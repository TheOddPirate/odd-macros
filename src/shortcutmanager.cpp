// SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later
// Modified for this project by the oddpirate, check out kiot for originale https://github.com/davidedmundson/kiot

/**
 * @file shortcutmanager.cpp
 * @brief Implementation of the global shortcut registration.
 *
 * @ingroup odd-macros-core
 *
 * This translation unit is derived from David Edmundson's kiot project and
 * adapted to register macros defined in the odd-macros configuration file.
 */

#include "shortcutmanager.h"
#include "platformhelper.h"
#include <QCoreApplication>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <QAction>
#include <QTimer>
#include <QString>
#include <QList>
#include <QFileInfo>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(shortcutlogs)

/**
 * @brief Logging category used by the shortcut manager.
 */
Q_LOGGING_CATEGORY(shortcutlogs, LOG_CAT(ShortcutManager))

/**
 * @brief Construct a shortcut manager.
 *
 * @param device the virtual keyboard device used to execute macros.
 * @param parent optional QObject parent.
 */
ShortcutManager::ShortcutManager(VirtualKeyboardDevice *device, QObject *parent)
: QObject(parent), m_device(device) {}

/**
 * @brief Load configured shortcuts and register them with KGlobalAccel.
 *
 * Opens the application's real configuration file (not the in-memory one),
 * and for every shortcut ID under the @b [Shortcuts] group:
 *
 * - If a @c Keycode entry is present, a single-key macro action is created.
 * - If a @c Keycodes entry is present, a sequence macro action is created.
 *
 * Each action registers an empty default with KGlobalAccel, which lets Plasma
 * fill in the shortcut assigned by the user in System Settings. A short timer
 * delay before playback gives the physical modifier keys time to be released.
 */
void ShortcutManager::loadAndRegisterShortcuts() {
    if (!m_device) {
        qCWarning(shortcutlogs) << "ShortcutManager failed: VirtualKeyboardDevice is null!";
        return;
    }

    QString configFile = PlatformHelper::configFilePath();
    if (!QFileInfo::exists(configFile)) {
        qCWarning(shortcutlogs) << "Config file does not exist, aborting ShortcutManager";
        qCInfo(shortcutlogs) << "Config file should be saved as:" << configFile;
        qCInfo(shortcutlogs) << "And look like this:";
        qCInfo(shortcutlogs) << "\n\n[Shortcuts][next_sibling]\nName=Goto Next Sibling folder\nKeycodes=14+106+28";
        return;
    }

    VirtualKeyboardDevice *vdev = m_device;
    auto shortcutConfigToplevel = KSharedConfig::openConfig()->group(QStringLiteral("Shortcuts"));
    const QStringList shortcutIds = shortcutConfigToplevel.groupList();

    for (const QString &shortcutId : shortcutIds) {
        auto shortcutConfig = shortcutConfigToplevel.group(shortcutId);

        const QString name = shortcutConfig.readEntry("Name", shortcutId);

        if (shortcutConfig.hasKey("Keycode")) {
            int targetKeycode = shortcutConfig.readEntry("Keycode", 0);

            if (targetKeycode == 0) {
                continue;
            }

            // 1. Important: Pass 'this' as parent so the action isn't destroyed or loses context
            QAction *action = new QAction(name, this);
            action->setObjectName(shortcutId);

            // 2. Register empty default - KGlobalAccel retrieves the saved shortcut from Plasma config!
            KGlobalAccel::self()->setShortcut(action, {});

            // 3. Connect with a timer delay so physical modifier keys can be released
            connect(action, &QAction::triggered, action, [vdev, targetKeycode, name]() {
                qCInfo(shortcutlogs) << "Triggered:" << name << "-> Keycode:" << targetKeycode;

                QTimer::singleShot(800, vdev, [vdev, targetKeycode]() {
                    vdev->sendKeycode(targetKeycode);
                });
            });
        } else if (shortcutConfig.hasKey("Keycodes")) {
            QString preSplit = shortcutConfig.readEntry("Keycodes", "Nope");

            if (preSplit == "Nope") {
                continue;
            }

            QStringList parts = preSplit.split('+', Qt::SkipEmptyParts);
            QList<int> targetKeyCodes;
            for (const QString &part : parts) {
                bool ok;
                int value = part.toInt(&ok);
                if (ok) {
                    targetKeyCodes.append(value);
                } else {
                    qCInfo(shortcutlogs) << "Invalid number:" << part;
                }
            }

            // 1. Important: Pass 'this' as parent so the action isn't destroyed or loses context
            QAction *action = new QAction(name, this);
            action->setObjectName(shortcutId);

            // 2. Register empty default - KGlobalAccel retrieves the saved shortcut from Plasma config!
            KGlobalAccel::self()->setShortcut(action, {});

            // 3. Connect with a timer delay so physical modifier keys can be released
            connect(action, &QAction::triggered, action, [vdev, targetKeyCodes, name]() {

                QTimer::singleShot(800, vdev, [vdev, targetKeyCodes]() {
                    for (int keycode : targetKeyCodes) {
                        qCInfo(shortcutlogs) << "Macro sending keycode:" << keycode;

                        // Since sendKeycode uses std::this_thread::sleep_for internally,
                        // this line blocks automatically until the key is pressed AND released!
                        vdev->sendKeycode(keycode);
                    }
                });

            });
        }
        qCInfo(shortcutlogs) << "KGlobalAccel registered:" << shortcutId << "(" << name << ")";
    }

    qCInfo(shortcutlogs) << QStringLiteral(PROJECT_NAME) << "daemon is running in the background under Wayland/KDE.";
}
