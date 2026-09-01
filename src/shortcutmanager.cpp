// SPDX-FileCopyrightText: 2025 David Edmundson <davidedmundson@kde.org>
// SPDX-License-Identifier: LGPL-2.1-or-later
// Modified for this project by the oddpirate, check out kiot for original https://github.com/davidedmundson/kiot

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
#include <QThread>

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
 * - If a @c Sequence entry is present, an advanced multi-step action is created.
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
        qCInfo(shortcutlogs) << "\n\n[Shortcuts][next_sibling]\nName=Goto Next Sibling folder\nKeycodes=KEY_BACKSPACE+KEY_RIGHT+KEY_ENTER";
        return;
    }

    VirtualKeyboardDevice *vdev = m_device;
    auto shortcutConfigToplevel = KSharedConfig::openConfig()->group(QStringLiteral("Shortcuts"));
    const QStringList shortcutIds = shortcutConfigToplevel.groupList();

    for (const QString &shortcutId : shortcutIds) {
        auto shortcutConfig = shortcutConfigToplevel.group(shortcutId);
        const QString name = shortcutConfig.readEntry("Name", shortcutId);

        if (shortcutConfig.hasKey("Keycode")) {
            QString entry = shortcutConfig.readEntry("Keycode", "");
            int targetKeycode = vdev->keycodeFromName(entry);

            if (targetKeycode <= 0) {
                qCWarning(shortcutlogs) << "Invalid Keycode entry:" << entry << "for shortcut:" << shortcutId;
                continue;
            }

            QAction *action = new QAction(name, this);
            action->setObjectName(shortcutId);
            KGlobalAccel::self()->setShortcut(action, {});

            connect(action, &QAction::triggered, action, [vdev, targetKeycode, name]() {
                qCInfo(shortcutlogs) << "Triggered:" << name << "-> Keycode:" << targetKeycode;

                QTimer::singleShot(200, vdev, [vdev, targetKeycode]() {
                    vdev->sendKeycode(targetKeycode);
                });
            });

        } else if (shortcutConfig.hasKey("Keycodes")) {
            QString preSplit = shortcutConfig.readEntry("Keycodes", "");
            if (preSplit.isEmpty()) {
                continue;
            }

            QStringList parts = preSplit.split('+', Qt::SkipEmptyParts);
            QList<int> targetKeyCodes;
            for (const QString &part : parts) {
                int code = vdev->keycodeFromName(part.trimmed());
                if (code > 0) {
                    targetKeyCodes.append(code);
                } else {
                    qCWarning(shortcutlogs) << "Invalid key name/code in Keycodes entry:" << part;
                }
            }

            if (targetKeyCodes.isEmpty()) {
                continue;
            }

            QAction *action = new QAction(name, this);
            action->setObjectName(shortcutId);
            KGlobalAccel::self()->setShortcut(action, {});

            connect(action, &QAction::triggered, action, [vdev, targetKeyCodes, name]() {
                qCInfo(shortcutlogs) << "Triggered:" << name << "-> Legacy Keycodes sequence";

                QTimer::singleShot(200, vdev, [vdev, targetKeyCodes]() {
                    for (int keycode : targetKeyCodes) {
                        qCInfo(shortcutlogs) << "Macro sending keycode:" << keycode;
                        vdev->sendKeycode(keycode);
                    }
                });
            });

        } else if (shortcutConfig.hasKey("Sequence")) {
            QString sequence = shortcutConfig.readEntry("Sequence", "");
            if (sequence.isEmpty()) {
                continue;
            }

            QAction *action = new QAction(name, this);
            action->setObjectName(shortcutId);
            KGlobalAccel::self()->setShortcut(action, {});

            connect(action, &QAction::triggered, action, [this, vdev, sequence, name]() {
                qCInfo(shortcutlogs) << "Triggered:" << name << "-> Full Macro Sequence";

                QTimer::singleShot(200, vdev, [this, sequence]() {
                    executeSequence(sequence);
                });
            });
        }

        qCInfo(shortcutlogs) << "KGlobalAccel registered:" << shortcutId << "(" << name << ")";
    }

    qCInfo(shortcutlogs) << QStringLiteral(PROJECT_NAME) << "daemon is running in the background under Wayland/KDE.";
}

/**
 * @brief Parse and execute an advanced multi-step macro sequence string.
 *
 * @param sequenceStr formatted sequence string (e.g. "down:KEY_LEFTCTRL, click:KEY_A, up:KEY_LEFTCTRL")
 */
void ShortcutManager::executeSequence(const QString &sequenceStr) {
    if (!m_device) return;

    QStringList steps = sequenceStr.split(',', Qt::SkipEmptyParts);

    for (const QString &rawStep : steps) {
        QString step = rawStep.trimmed();
        int colonIdx = step.indexOf(':');

        if (colonIdx == -1) {
            // Backward compatibility: single key click without prefix
            int code = m_device->keycodeFromName(step);
            if (code > 0) {
                m_device->sendKeycode(code);
            }
            continue;
        }

        QString cmd = step.left(colonIdx).toLower().trimmed();
        QString val = step.mid(colonIdx + 1).trimmed();

        if (cmd == "down" || cmd == "press") {
            int code = m_device->keycodeFromName(val);
            if (code > 0) m_device->pressKeycode(code);

        } else if (cmd == "up" || cmd == "release") {
            int code = m_device->keycodeFromName(val);
            if (code > 0) m_device->releaseKeycode(code);

        } else if (cmd == "click") {
            int code = m_device->keycodeFromName(val);
            if (code > 0) m_device->sendKeycode(code);

        } else if (cmd == "delay") {
            bool ok = false;
            int ms = val.toInt(&ok);
            if (ok && ms > 0) {
                QThread::msleep(ms);
            }

        } else if (cmd == "type") {
            m_device->typeString(val);

        } else if (cmd == "mouse_move") {
            QStringList coords = val.split(';');
            if (coords.size() == 2) {
                int x = coords[0].toInt();
                int y = coords[1].toInt();
                m_device->moveMouse(x, y);
            }

        } else if (cmd == "scroll") {
            int steps = val.toInt();
            m_device->scrollWheel(steps);
        }

        // Micro-pause between steps to avoid event pacing issues in uinput/kernel
        QThread::msleep(10);
    }
}