// SPDX-FileCopyrightText: 2025 Odd Østlie <theoddpirate@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file virtualkeyboarddevice.cpp
 * @brief Implementation of the virtual keyboard device using uinput.
 *
 * @ingroup odd-macros-core
 */

#include "virtualkeyboarddevice.h"
#include "platformhelper.h"

#include <QMap>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <xkbcommon/xkbcommon.h>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(vkblogs)
Q_LOGGING_CATEGORY(vkblogs, LOG_CAT(VirtualKeyboardDevice))

/**
 * @brief A single entry of the built-in key table.
 */
struct KeyInfo {
    int code;         ///< The Linux input keycode value.
    const char* name; ///< The symbolic key name (e.g. @c "KEY_ENTER").
};

/**
 * @brief The built-in table of all keycodes supported by the daemon.
 *
 * Includes the standard letter, number and symbol keys, modifier keys,
 * function keys, navigation keys, the numeric keypad, media/system keys and
 * a set of mouse buttons (so button presses can also be injected).
 */
static const KeyInfo ALL_KEYS[] = {
    {KEY_ESC, "KEY_ESC"}, {KEY_1, "KEY_1"}, {KEY_2, "KEY_2"}, {KEY_3, "KEY_3"}, {KEY_4, "KEY_4"},
    {KEY_5, "KEY_5"}, {KEY_6, "KEY_6"}, {KEY_7, "KEY_7"}, {KEY_8, "KEY_8"}, {KEY_9, "KEY_9"},
    {KEY_0, "KEY_0"}, {KEY_MINUS, "KEY_MINUS"}, {KEY_EQUAL, "KEY_EQUAL"}, {KEY_BACKSPACE, "KEY_BACKSPACE"},
    {KEY_TAB, "KEY_TAB"}, {KEY_Q, "KEY_Q"}, {KEY_W, "KEY_W"}, {KEY_E, "KEY_E"}, {KEY_R, "KEY_R"},
    {KEY_T, "KEY_T"}, {KEY_Y, "KEY_Y"}, {KEY_U, "KEY_U"}, {KEY_I, "KEY_I"}, {KEY_O, "KEY_O"},
    {KEY_P, "KEY_P"}, {KEY_LEFTBRACE, "KEY_LEFTBRACE"}, {KEY_RIGHTBRACE, "KEY_RIGHTBRACE"}, {KEY_ENTER, "KEY_ENTER"},
    {KEY_LEFTCTRL, "KEY_LEFTCTRL"}, {KEY_A, "KEY_A"}, {KEY_S, "KEY_S"}, {KEY_D, "KEY_D"}, {KEY_F, "KEY_F"},
    {KEY_G, "KEY_G"}, {KEY_H, "KEY_H"}, {KEY_J, "KEY_J"}, {KEY_K, "KEY_K"}, {KEY_L, "KEY_L"},
    {KEY_SEMICOLON, "KEY_SEMICOLON"}, {KEY_APOSTROPHE, "KEY_APOSTROPHE"}, {KEY_GRAVE, "KEY_GRAVE"},
    {KEY_LEFTSHIFT, "KEY_LEFTSHIFT"}, {KEY_BACKSLASH, "KEY_BACKSLASH"}, {KEY_Z, "KEY_Z"}, {KEY_X, "KEY_X"},
    {KEY_C, "KEY_C"}, {KEY_V, "KEY_V"}, {KEY_B, "KEY_B"}, {KEY_N, "KEY_N"}, {KEY_M, "KEY_M"},
    {KEY_COMMA, "KEY_COMMA"}, {KEY_DOT, "KEY_DOT"}, {KEY_SLASH, "KEY_SLASH"}, {KEY_RIGHTSHIFT, "KEY_RIGHTSHIFT"},
    {KEY_102ND, "KEY_102ND"}, 
    {KEY_SPACE, "KEY_SPACE"}, {KEY_CAPSLOCK, "KEY_CAPSLOCK"},

    {KEY_LEFTALT, "KEY_LEFTALT"}, {KEY_RIGHTALT, "KEY_RIGHTALT"},
    {KEY_LEFTMETA, "KEY_LEFTMETA"}, {KEY_RIGHTMETA, "KEY_RIGHTMETA"},
    {KEY_RIGHTCTRL, "KEY_RIGHTCTRL"}, {KEY_COMPOSE, "KEY_COMPOSE"},


    {KEY_F1, "KEY_F1"}, {KEY_F2, "KEY_F2"}, {KEY_F3, "KEY_F3"}, {KEY_F4, "KEY_F4"},
    {KEY_F5, "KEY_F5"}, {KEY_F6, "KEY_F6"}, {KEY_F7, "KEY_F7"}, {KEY_F8, "KEY_F8"},
    {KEY_F9, "KEY_F9"}, {KEY_F10, "KEY_F10"}, {KEY_F11, "KEY_F11"}, {KEY_F12, "KEY_F12"},
    {KEY_F13, "KEY_F13"}, {KEY_F14, "KEY_F14"}, {KEY_F15, "KEY_F15"}, {KEY_F16, "KEY_F16"},
    {KEY_F17, "KEY_F17"}, {KEY_F18, "KEY_F18"}, {KEY_F19, "KEY_F19"}, {KEY_F20, "KEY_F20"},

    {KEY_SYSRQ, "KEY_SYSRQ"}, {KEY_SCROLLLOCK, "KEY_SCROLLLOCK"}, {KEY_PAUSE, "KEY_PAUSE"},
    {KEY_INSERT, "KEY_INSERT"}, {KEY_HOME, "KEY_HOME"}, {KEY_PAGEUP, "KEY_PAGEUP"},
    {KEY_DELETE, "KEY_DELETE"}, {KEY_END, "KEY_END"}, {KEY_PAGEDOWN, "KEY_PAGEDOWN"},
    {KEY_RIGHT, "KEY_RIGHT"}, {KEY_LEFT, "KEY_LEFT"}, {KEY_DOWN, "KEY_DOWN"}, {KEY_UP, "KEY_UP"},

    // Numeric
    {KEY_NUMLOCK, "KEY_NUMLOCK"}, {KEY_KPSLASH, "KEY_KPSLASH"}, {KEY_KPASTERISK, "KEY_KPASTERISK"},
    {KEY_KPMINUS, "KEY_KPMINUS"}, {KEY_KPPLUS, "KEY_KPPLUS"}, {KEY_KPENTER, "KEY_KPENTER"},
    {KEY_KPDOT, "KEY_KPDOT"}, {KEY_KPEQUAL, "KEY_KPEQUAL"},
    {KEY_KP0, "KEY_KP0"}, {KEY_KP1, "KEY_KP1"}, {KEY_KP2, "KEY_KP2"}, {KEY_KP3, "KEY_KP3"},
    {KEY_KP4, "KEY_KP4"}, {KEY_KP5, "KEY_KP5"}, {KEY_KP6, "KEY_KP6"}, {KEY_KP7, "KEY_KP7"},
    {KEY_KP8, "KEY_KP8"}, {KEY_KP9, "KEY_KP9"},

    // Media & System
    {KEY_MUTE, "KEY_MUTE"}, {KEY_VOLUMEDOWN, "KEY_VOLUMEDOWN"}, {KEY_VOLUMEUP, "KEY_VOLUMEUP"},
    {KEY_POWER, "KEY_POWER"}, {KEY_SLEEP, "KEY_SLEEP"}, {KEY_WAKEUP, "KEY_WAKEUP"},
    {KEY_PLAYPAUSE, "KEY_PLAYPAUSE"}, {KEY_NEXTSONG, "KEY_NEXTSONG"}, {KEY_PREVIOUSSONG, "KEY_PREVIOUSSONG"},
    {KEY_STOPCD, "KEY_STOPCD"}, {KEY_CALC, "KEY_CALC"}, {KEY_MAIL, "KEY_MAIL"},

    // Mouse
    {BTN_LEFT, "BTN_LEFT"}, {BTN_RIGHT, "BTN_RIGHT"}, {BTN_MIDDLE, "BTN_MIDDLE"}
};

/**
 * @brief Check whether the process can write to @c /dev/uinput.
 *
 * @return @c true if write access is available.
 */
bool VirtualKeyboardDevice::hasUinputAccess() {
    return (access("/dev/uinput", W_OK) == 0);
}

/**
 * @brief Construct a virtual keyboard device.
 *
 * Opens @c /dev/uinput, enables the requested event capabilities, configures
 * the device with a fixed vendor/product pair and name, and creates the device
 * through the uinput ioctls. A short sleep allows the device to settle.
 *
 * If anything fails, @ref m_lastError is populated and the device is left in a
 * non-ready state (see @ref isReady()).
 *
 * @param options the event capabilities to enable.
 * @param parent  optional QObject parent.
 */
VirtualKeyboardDevice::VirtualKeyboardDevice(SetupOptions options, QObject *parent)
: QObject(parent)
{
    if (!hasUinputAccess()) {
        m_lastError = "No write access to /dev/uinput. Is the user in the 'input' group?";
        qCWarning(vkblogs) << m_lastError;
        return;
    }

    m_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        m_lastError = QString("Failed to open /dev/uinput: %1").arg(strerror(errno));
        qCWarning(vkblogs) << m_lastError;
        return;
    }

    setupBits(options);

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor = 0x2333;
    usetup.id.product = 0x6666;
    usetup.id.version = 1;
    std::strncpy(usetup.name, QStringLiteral(PROJECT_NAME).toUtf8() + " Virtual Keyboard Device", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &usetup) < 0) {
        m_lastError = QString("UI_DEV_SETUP ioctl failed: %1").arg(strerror(errno));
        qCWarning(vkblogs) << m_lastError;
        close(m_fd);
        m_fd = -1;
        return;
    }

    if (ioctl(m_fd, UI_DEV_CREATE) < 0) {
        m_lastError = QString("UI_DEV_CREATE ioctl failed: %1").arg(strerror(errno));
        qCWarning(vkblogs) << m_lastError;
        close(m_fd);
        m_fd = -1;
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

/**
 * @brief Destroy the virtual keyboard device.
 *
 * Destroys and closes the underlying uinput device if it is open.
 */
VirtualKeyboardDevice::~VirtualKeyboardDevice() {
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        close(m_fd);
    }
}

/**
 * @brief Emit a single raw uinput event.
 *
 * @param type  the event type (e.g. @c EV_KEY, @c EV_SYN).
 * @param code  the event code.
 * @param value the event value.
 * @return @c true on a full write, otherwise @c false.
 */
bool VirtualKeyboardDevice::emitEvent(uint16_t type, uint16_t code, int32_t value) {
    if (m_fd < 0) return false;

    struct input_event ie;
    std::memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = value;

    return (write(m_fd, &ie, sizeof(ie)) == sizeof(ie));
}

/**
 * @brief Emit a key event followed by a sync report.
 *
 * @param keycode the Linux input keycode.
 * @param value   the event value (1 press, 0 release).
 * @return @c true if both writes succeed, otherwise @c false.
 */
bool VirtualKeyboardDevice::emitKey(int keycode, int32_t value) {
    bool ok = emitEvent(EV_KEY, keycode, value);
    ok &= emitEvent(EV_SYN, SYN_REPORT, 0);
    return ok;
}

/**
 * @brief Press (hold down) a single key.
 *
 * @param keycode the Linux input keycode.
 * @return @c true on success, @c false if the device is invalid or the
 *         keycode is not positive.
 */
bool VirtualKeyboardDevice::pressKeycode(int keycode) {
    if (m_fd < 0 || keycode <= 0) return false;
    return emitKey(keycode, 1); // 1 = Press
}

/**
 * @brief Release a key.
 *
 * @param keycode the Linux input keycode.
 * @return @c true on success, @c false if the device is invalid or the
 *         keycode is not positive.
 */
bool VirtualKeyboardDevice::releaseKeycode(int keycode) {
    if (m_fd < 0 || keycode <= 0) return false;
    return emitKey(keycode, 0); // 0 = Release
}

/**
 * @brief Send a complete key press+release cycle.
 *
 * Reuses @ref pressKeycode() and @ref releaseKeycode().
 *
 * @param keycode the Linux input keycode to send.
 * @param delayMs delay in milliseconds between press and release.
 * @return @c true if both the press and release succeeded.
 */
bool VirtualKeyboardDevice::sendKeycode(int keycode, int delayMs) {
    bool ok = pressKeycode(keycode);
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    ok &= releaseKeycode(keycode);
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    return ok;
}

/**
 * @brief Type out a full text string character by character.
 *
 * @param text    the text to type.
 * @param delayMs delay in milliseconds between keystrokes.
 * @return @c true once the whole string has been processed.
 */
bool VirtualKeyboardDevice::typeString(const QString &text, int delayMs) {
    if (m_fd < 0) return false;

    for (const QChar &ch : text) {
        bool needsShift = false;
        int code = stringToKeycode(ch, needsShift);

        if (code <= 0) {
            qCWarning(vkblogs) << "Missing keycode mapping for character:" << ch;
            continue;
        }

        if (needsShift) {
            emitKey(KEY_LEFTSHIFT, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        sendKeycode(code, delayMs);

        if (needsShift) {
            emitKey(KEY_LEFTSHIFT, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    return true;
}

/**
 * @brief Print the list of all registered keycodes and their names.
 */
void VirtualKeyboardDevice::printRegisteredKeys() const {
    qCInfo(vkblogs) << "=== Registered Keys in VirtualKeyboardDevice ===";
    size_t count = sizeof(ALL_KEYS) / sizeof(KeyInfo);
    for (size_t i = 0; i < count; ++i) {
        qCInfo(vkblogs).noquote() << QString("Keycode: %1 \t -> %2").arg(ALL_KEYS[i].code, 4).arg(ALL_KEYS[i].name);
    }
    qCInfo(vkblogs) << "================================================";
}

/**
 * @brief Configure the event capability bits on the device.
 *
 * @param options the flags of the event groups to enable.
 */
void VirtualKeyboardDevice::setupBits(SetupOptions options) {
    if (options.testFlag(EnableKey)) {
        ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
        ioctl(m_fd, UI_SET_EVBIT, EV_SYN);
        registerDefaultKeys();
    }

    if (options.testFlag(EnableRel)) {
        ioctl(m_fd, UI_SET_EVBIT, EV_REL);
        static const int rel_list[] = {REL_X, REL_Y, REL_Z, REL_WHEEL, REL_HWHEEL};
        for (int rel : rel_list) {
            ioctl(m_fd, UI_SET_RELBIT, rel);
        }
    }

    if (options.testFlag(EnableAbs)) {
        ioctl(m_fd, UI_SET_EVBIT, EV_ABS);
        static const int abs_list[] = {ABS_X, ABS_Y, ABS_MT_SLOT, ABS_MT_TRACKING_ID,
            ABS_MT_POSITION_X, ABS_MT_POSITION_Y, ABS_PRESSURE, ABS_MT_PRESSURE};
            for (int abs : abs_list) {
                ioctl(m_fd, UI_SET_ABSBIT, abs);
            }
    }
}

/**
 * @brief Register all default keycodes from the built-in key table.
 */
void VirtualKeyboardDevice::registerDefaultKeys() {
    size_t count = sizeof(ALL_KEYS) / sizeof(KeyInfo);
    for (size_t i = 0; i < count; ++i) {
        ioctl(m_fd, UI_SET_KEYBIT, ALL_KEYS[i].code);
    }
}

/**
 * @brief Translate a character into a Linux keycode using libxkbcommon.
 *
 * Iterates over all possible xkb keycodes and looks for a match either without
 * any modifiers or with the Shift modifier active. This makes the mapping
 * layout-aware under Wayland.
 *
 * @param ch          the character to map.
 * @param needsShift  on return set to @c true if the character requires Shift.
 * @return the Linux input keycode, or a non-positive value when unmapped.
 */
int VirtualKeyboardDevice::stringToKeycode(QChar ch, bool &needsShift) const {
    needsShift = false;
    uint32_t targetUtf32 = ch.unicode();

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) return -1;

    struct xkb_rule_names names = {
        .rules = nullptr,
        .model = nullptr,
        .layout = nullptr,
        .variant = nullptr,
        .options = nullptr
    };

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        xkb_context_unref(ctx);
        return -1;
    }

    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) {
        xkb_keymap_unref(keymap);
        xkb_context_unref(ctx);
        return -1;
    }

    // Hent den faktiske indeksen for Shift-modifikatoren i gjeldende layout
    xkb_mod_index_t shiftIdx = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
    xkb_mod_mask_t shiftMask = (shiftIdx != XKB_MOD_INVALID) ? (1u << shiftIdx) : 0;

    int foundKeycode = -1;

    for (xkb_keycode_t xkb_code = 8; xkb_code < 255; ++xkb_code) {

        // 1. Sjekk uten modifikatorer
        xkb_state_update_mask(state, 0, 0, 0, 0, 0, 0);
        uint32_t utf32_unstyled = xkb_state_key_get_utf32(state, xkb_code);
        if (utf32_unstyled == targetUtf32) {
            foundKeycode = xkb_code - 8;
            needsShift = false;
            break;
        }

        // 2. Sjekk med Shift-modifikator aktivert
        if (shiftMask != 0) {
            xkb_state_update_mask(state, shiftMask, 0, 0, 0, 0, 0);
            uint32_t utf32_shifted = xkb_state_key_get_utf32(state, xkb_code);
            if (utf32_shifted == targetUtf32) {
                foundKeycode = xkb_code - 8;
                needsShift = true;
                break;
            }
        }
    }

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return foundKeycode;
}
