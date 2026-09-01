#pragma once

#include <QObject>
#include "virtualkeyboarddevice.h"

/**
 * @file shortcutmanager.h
 * @brief Registration of global shortcuts from the configuration file.
 *
 * The ShortcutManager reads the application configuration and registers each
 * entry with KDE's KGlobalAccel, so that a user-defined global hotkey is
 * translated into a macro (either a single keycode or a sequence of them)
 * injected through the virtual keyboard device.
 *
 * @ingroup odd-macros-core
 * @author Odd Østlie &lt;theoddpirate@gmail.com&gt;
 * @since 0.1
 */

/**
 * @brief Reads configured shortcuts and registers them with KGlobalAccel.
 *
 * The manager parses the top-level @b [Shortcuts] group of the configuration
 * file. Each sub-group represents one shortcut and supports either a
 * @c Keycode entry (single key) or a @c Keycodes entry (a @c '+'-separated
 * sequence of keycodes).
 */
class ShortcutManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Construct a shortcut manager.
     *
     * @param device the virtual keyboard device used to run the macros.
     * @param parent optional QObject parent.
     */
    explicit ShortcutManager(VirtualKeyboardDevice *device, QObject *parent = nullptr);

    /**
     * @brief Load configured shortcuts and register them with KGlobalAccel.
     *
     * Reads the current KSharedConfig configuration, iterates over every
     * shortcut ID under the @b [Shortcuts] group, creates a KGlobalAccel action
     * for each and connects its trigger to the corresponding macro playback.
     *
     * If the device is null, or the configuration file is missing, an empty
     * set is registered and a warning is logged.
     */
    void loadAndRegisterShortcuts();
    void executeSequence(const QString &sequenceStr);

private:
    /**
     * @brief The virtual keyboard device used to execute macros.
     */
    VirtualKeyboardDevice *m_device = nullptr;
};
