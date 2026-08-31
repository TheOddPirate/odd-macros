// SPDX-FileCopyrightText: 2025 Odd Østlie <theoddpirate@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file main.cpp
 * @brief Entry point for the odd-macros daemon and command-line tool.
 *
 * Provides both a background daemon that registers global shortcuts
 * (via KGlobalAccel) and a small command-line utility for testing the virtual
 * keyboard by sending a single keycode or typing a full text string.
 *
 * @author Odd Østlie &lt;theoddpirate@gmail.com&gt;
 * @since 0.1
 *
 * @par Exit codes
 * - @c 0 success
 * - @c 1 a failure during argument validation, device initialisation or sending
 *
 * @defgroup odd-macros-core Core modules
 * @brief The core building blocks of the odd-macros daemon.
 *
 * Groups the main header documentation of the platform helper, the virtual
 * keyboard device, the shortcut manager and the logging infrastructure.
 */

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <iostream>

#include "virtualkeyboarddevice.h"
#include "shortcutmanager.h"
#include "platformhelper.h"
#include "messagehandler.h"

#include <QLoggingCategory>


/**
 * @brief Application entry point.
 *
 * Initialises the QGuiApplication (required for KGlobalAccel under Wayland),
 * sets up logging, parses the command line, verifies uinput access and then
 * either runs one of the CLI modes (list keys, send keycode, type text) or
 * falls back to the daemon mode by starting the ShortcutManager and the event
 * loop.
 *
 * @param argc number of command line arguments.
 * @param argv the command line argument vector.
 * @return the application exit code.
 */
int main(int argc, char *argv[])
{
    // Must be QGuiApplication under Wayland for KGlobalAccel to work!
    QGuiApplication app(argc, argv);

    app.setApplicationName(QStringLiteral(PROJECT_NAME));
    app.setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    app.setOrganizationName(QStringLiteral(PROJECT_NAME));
    QString domain = PlatformHelper::resolveOrganizationDomain(QStringLiteral(PROJECT_DOMAIN));
    app.setOrganizationDomain(domain);

    initLogging();

    // Set up CLI Argument Parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(PROJECT_DESCRIPTION));
    parser.addHelpOption();
    parser.addVersionOption();

    // Define arguments/options
    QCommandLineOption sendCodeOption(QStringList() << "k" << "send-code","Send a single Linux uinput keycode (e.g., 40 for 'a', 28 for Enter).", "keycode");
    parser.addOption(sendCodeOption);

    QCommandLineOption typeOption(QStringList() << "t" << "type","Send a full text string directly through the virtual keyboard.","text");
    parser.addOption(typeOption);

    QCommandLineOption listKeysOption(QStringList() << "l" << "list-keys","Print an overview of all available keycodes and their names.");
    parser.addOption(listKeysOption);

    QCommandLineOption delayOption(QStringList() << "d" << "delay","Delay in milliseconds between keypresses (Default: 12ms).","ms","12");
    parser.addOption(delayOption);


    parser.process(app);

    // 1. Check access to /dev/uinput before doing anything else
    if (!VirtualKeyboardDevice::hasUinputAccess()) {
        qCCritical(mainlog) << "Error: Access to /dev/uinput denied.";
        qCCritical(mainlog) << "Ensure your user is a member of the 'input' group and has correct udev rules configured.";
        return 1;
    }

    // 2. Initialize the virtual keyboard device
    VirtualKeyboardDevice vdev(VirtualKeyboardDevice::EnableKey);
    if (!vdev.isReady()) {
        qCCritical(mainlog) << "Failed to create virtual keyboard device:" << vdev.lastError();
        return 1;
    }

    // --- CLI MODE 1: Print Keycodes ---
    if (parser.isSet(listKeysOption)) {
        vdev.printRegisteredKeys();
        return 0;
    }

    int delayMs = parser.value(delayOption).toInt();
    if (delayMs <= 0) delayMs = 12;

    // --- CLI MODE 2: Send Single Keycode ---
    if (parser.isSet(sendCodeOption)) {
        bool ok = false;
        int keycode = parser.value(sendCodeOption).toInt(&ok);
        if (!ok || keycode <= 0) {
            qCCritical(mainlog) << "Invalid keycode specified. Must be a positive integer.";
            return 1;
        }

        qInfo() << QString("Sending keycode %1 (delay: %2ms)...").arg(keycode).arg(delayMs);
        if (vdev.sendKeycode(keycode, delayMs)) {
            qCInfo(mainlog) << "Keycode sent successfully!";
            return 0;
        } else {
            qCCritical(mainlog) << "Failed to send keycode.";
            return 1;
        }
    }

    // --- CLI MODE 3: Type Text String ---
    if (parser.isSet(typeOption)) {
        QString textToType = parser.value(typeOption);
        qCInfo(mainlog) << QString("Typing text: \"%1\" (delay: %2ms)...").arg(textToType).arg(delayMs);
        if (vdev.typeString(textToType, delayMs)) {
            qCInfo(mainlog) << "Text sent successfully!";
            return 0;
        } else {
            qCCritical(mainlog) << "Failed to send text string.";
            return 1;
        }
    }

    // --- DAEMON MODE: Executed when no CLI flags are set ---
    ShortcutManager manager(&vdev);
    manager.loadAndRegisterShortcuts();


    qCInfo(mainlog) << "Press Ctrl+C to terminate.";

    return app.exec();
}


