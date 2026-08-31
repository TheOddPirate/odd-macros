#ifndef VIRTUALKEYBOARDDEVICE_H
#define VIRTUALKEYBOARDDEVICE_H

#include <QObject>
#include <QString>
#include <QFlags>
#include <QMap>

/**
 * @file virtualkeyboarddevice.h
 * @brief Virtual keyboard device abstraction backed by the Linux uinput subsystem.
 *
 * This class creates and drives a virtual input device under @c /dev/uinput,
 * allowing the daemon to inject keyboard events (single keycodes, press/release
 * pairs and full text strings) into the system as if they came from a physical
 * keyboard.
 *
 * The class is non-copyable and owns a single file descriptor for the lifetime
 * of the instance.
 *
 * @ingroup odd-macros-core
 * @author Odd Østlie &lt;theoddpirate@gmail.com&gt;
 * @since 0.1
 */

/**
 * @brief Manages a virtual Linux keyboard device created via @c /dev/uinput.
 *
 * A default constructed instance attempts to open the uinput device in the
 * constructor. Check @ref isReady() afterwards before using any of the other
 * public methods; on failure @ref lastError() holds a human-readable reason.
 */
class VirtualKeyboardDevice : public QObject {
Q_OBJECT
Q_DISABLE_COPY(VirtualKeyboardDevice)

public:
    /**
     * @brief Event capability groups that can be enabled on the device.
     */
    enum SetupOption {
        EnableKey = 0x01,  ///< Enable key (EV_KEY) and sync (EV_SYN) events.
        EnableRel = 0x02,  ///< Enable relative (EV_REL) events.
        EnableAbs = 0x04   ///< Enable absolute (EV_ABS) events.
    };
    /**
     * @brief A combination of @ref SetupOption flags.
     */
    Q_DECLARE_FLAGS(SetupOptions, SetupOption)

    /**
     * @brief Construct a virtual keyboard device.
     *
     * Opens @c /dev/uinput, configures the requested event capabilities, sets
     * up the device with a vendor/product identifier and creates the device.
     *
     * @param options the event capabilities to enable, defaulting to
     *                @ref EnableKey only.
     * @param parent  optional QObject parent.
     */
    explicit VirtualKeyboardDevice(SetupOptions options = EnableKey, QObject *parent = nullptr);

    /**
     * @brief Destroy the virtual keyboard device.
     *
     * Optionally destroys and closes the underlying uinput device.
     */
    ~VirtualKeyboardDevice() override;

    /**
     * @brief Check whether the underlying uinput device is open and ready.
     *
     * @return @c true when the file descriptor is valid, otherwise @c false.
     */
    bool isReady() const { return m_fd >= 0; }

    /**
     * @brief The last error message, if any.
     *
     * @return a human-readable error string, or an empty string if none.
     */
    QString lastError() const { return m_lastError; }

    /**
     * @brief Emit a single raw uinput event.
     *
     * @param type  the event type (e.g. @c EV_KEY, @c EV_SYN).
     * @param code  the event code (e.g. a keycode).
     * @param value the event value (e.g. 1 for press, 0 for release).
     * @return @c true on successful write, otherwise @c false.
     */
    bool emitEvent(uint16_t type, uint16_t code, int32_t value);

    /**
     * @brief Emit a key event followed by a sync report.
     *
     * @param keycode the Linux input keycode.
     * @param value   the event value (1 press, 0 release).
     * @return @c true if both writes succeed, otherwise @c false.
     */
    bool emitKey(int keycode, int32_t value);

    /**
     * @brief Send a complete key press+release cycle.
     *
     * Presses @p keycode, waits @p delayMs, releases it and waits @p delayMs
     * again. The call blocks for roughly @c 2*delayMs milliseconds.
     *
     * @param keycode the Linux input keycode to send.
     * @param delayMs delay in milliseconds between press and release.
     * @return @c true if both the press and release succeeded.
     */
    bool sendKeycode(int keycode, int delayMs = 12);

    /**
     * @brief Press (hold down) a single key.
     *
     * @param keycode the Linux input keycode.
     * @return @c true on success, @c false if the device is invalid or the
     *         keycode is not positive.
     */
    bool pressKeycode(int keycode);

    /**
     * @brief Release a previously pressed key.
     *
     * @param keycode the Linux input keycode.
     * @return @c true on success, @c false if the device is invalid or the
     *         keycode is not positive.
     */
    bool releaseKeycode(int keycode);

    /**
     * @brief Type out a full text string character by character.
     *
     * Each character is translated to a keycode (and an implicit Shift press
     * for upper-case characters) via libxkbcommon, and emitted in sequence with
     * the given delay between keystrokes. Unmapped characters are skipped with a
     * warning.
     *
     * @param text    the text to type.
     * @param delayMs delay in milliseconds between keystrokes.
     * @return @c true once the whole string has been processed.
     */
    bool typeString(const QString &text, int delayMs = 12);

    /**
     * @brief Print the list of all supported keycodes and their names.
     *
     * Writes the registered key table to the console log for reference.
     */
    void printRegisteredKeys() const;

    /**
     * @brief Check whether the current process can write to @c /dev/uinput.
     *
     * @return @c true if write access is available.
     */
    static bool hasUinputAccess();

private:
    /**
     * @brief File descriptor of the open uinput device, or -1 when invalid.
     */
    int m_fd{ -1 };

    /**
     * @brief The most recent error message, if any.
     */
    QString m_lastError;

    /**
     * @brief Configure the event capability bits on the device.
     *
     * @param options the flags of the event groups to enable.
     */
    void setupBits(SetupOptions options);

    /**
     * @brief Register all default keycodes from the built-in key table.
     */
    void registerDefaultKeys();

    /**
     * @brief Translate a character into a Linux keycode using libxkbcommon.
     *
     * @param ch          the character to map.
     * @param needsShift  on return set to @c true if the character requires
     *                    the Shift modifier, otherwise @c false.
     * @return the Linux input keycode, or a non-positive value when unmapped.
     */
    int stringToKeycode(QChar ch, bool &needsShift) const;
};

/**
 * @brief Support for combining @ref VirtualKeyboardDevice::SetupOption values.
 */
Q_DECLARE_OPERATORS_FOR_FLAGS(VirtualKeyboardDevice::SetupOptions)

#endif // VIRTUALKEYBOARDDEVICE_H
