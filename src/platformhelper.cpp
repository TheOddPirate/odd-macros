// SPDX-FileCopyrightText: 2025 Odd Østlie <theoddpirate@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file platformhelper.cpp
 * @brief Implementation of the platform and process detection helpers.
 *
 * @ingroup odd-macros-core
 */

#include "platformhelper.h"
#include <QProcessEnvironment>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include <QApplication>
#include <QUrl>
#include <QString>
#include <QLoggingCategory>
#include <QSettings>

/**
 * @brief Logging category used by the platform helper.
 */
Q_DECLARE_LOGGING_CATEGORY(helper)
Q_LOGGING_CATEGORY(helper,LOG_CAT(Helper))

/**
 * @brief Detect the current platform using compile-time macros.
 *
 * @return the detected @ref PlatformHelper::Platform value.
 */
PlatformHelper::Platform PlatformHelper::currentPlatform()
{
    #if defined(Q_OS_WIN)
        return Platform::Windows;
    #elif defined(Q_OS_LINUX)
        return Platform::Linux;
    #elif defined(Q_OS_MACOS)
        return Platform::macOS;
    #elif defined(Q_OS_ANDROID)
        return Platform::Android;
    #elif defined(Q_OS_IOS)
        return Platform::iOS;
    #else
        return Platform::Unknown;
    #endif
}

/**
 * @brief Detect the current CPU architecture.
 *
 * @return the detected @ref PlatformHelper::Architecture value.
 */
PlatformHelper::Architecture PlatformHelper::currentArchitecture()
{
    QString arch = QSysInfo::currentCpuArchitecture();
    
    if (arch == "x86") return Architecture::X86;
    if (arch == "x86_64") return Architecture::X86_64;
    if (arch == "arm") return Architecture::ARM;
    if (arch == "arm64") return Architecture::ARM64;
    
    return Architecture::UnknownArch;
}

/**
 * @brief Check whether the application runs inside a Flatpak sandbox.
 *
 * The result is computed once and then cached for the lifetime of the process.
 *
 * @return @c true if running inside a Flatpak sandbox, otherwise @c false.
 */
bool PlatformHelper::isFlatpak()
{
    // Sjekk standard Flatpak miljøvariabler
    static bool cached = false;
    static bool isFlatpakValue = false;
    
    if (!cached) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        isFlatpakValue = env.contains("FLATPAK_ID") || 
                         env.value("container") == "flatpak" ||
                         env.contains("FLATPAK_SANDBOX_DIR");
        cached = true;
    }
    
    return isFlatpakValue;
}

/**
 * @brief Check whether the application runs inside a Snap confinement.
 *
 * @return @c true if the @c SNAP environment variable is set.
 */
bool PlatformHelper::isSnap()
{
    return !qEnvironmentVariableIsEmpty("SNAP");
}

/**
 * @brief Returns the path to the autostart directory for the current platform.
 *
 * @return the autostart directory, or an empty QString if unsupported.
 */
QString PlatformHelper::autostartPath()
{
    Platform platform = currentPlatform();
    
    switch (platform) {
    case Platform::Linux:
        if (isFlatpak()) {
            return QDir::homePath() + "/.config/autostart";
        }
        return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
        
    case Platform::Windows:
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
               + "/Microsoft/Windows/Start Menu/Programs/Startup";
               
    case Platform::macOS:
        return QDir::homePath() + "/Library/LaunchAgents";
        
    default:
        return QString();
    }
}

/**
 * @brief Human-readable name of the current platform.
 *
 * @return e.g. @c "Windows", @c "Linux", @c "macOS", @c "Android", @c "iOS"
 *         or @c "Unknown".
 */
QString PlatformHelper::platformName()
{
    switch (currentPlatform()) {
    case Platform::Windows: return "Windows";
    case Platform::Linux: return "Linux";
    case Platform::macOS: return "macOS";
    case Platform::Android: return "Android";
    case Platform::iOS: return "iOS";
    default: return "Unknown";
    }
}

/**
 * @brief Generate a reverse-DNS service name from the organisation domain.
 *
 * @return the generated service name.
 */
QString PlatformHelper::generateServiceName()
{

    QString domain = resolveOrganizationDomain(QString(PROJECT_DOMAIN));
    if (domain.isEmpty())
        domain = "local";

    QStringList parts = domain.split('.', Qt::SkipEmptyParts);
    QString reversed;
    for (const auto &p : parts)
        reversed.prepend(p + ".");
    return reversed + QString(PROJECT_NAME);
}

/**
 * @brief Normalise a raw domain string into a lower-cased host name.
 *
 * If no scheme is present the input is interpreted as a host and prefixed
 * with @c https:// so that @c QUrl can parse it as such.
 *
 * @param input the raw domain string.
 * @return the lower-cased host name, or @c "theoddpirate.com" when invalid.
 */
QString PlatformHelper::resolveOrganizationDomain(const QString &input)
{
    if (input.trimmed().isEmpty())
        return QStringLiteral("theoddpirate.com");

    QUrl url(input);

    // Hvis ingen scheme er satt, prøv å tolke det som en host
    if (!url.isValid() || url.scheme().isEmpty())
        url = QUrl(QStringLiteral("https://") + input);

    QString host = url.host();

    // QUrl kan feile stille, så dobbeltsjekk
    if (host.isEmpty())
        return QStringLiteral("theoddpirate.com");

    return host.toLower();
}

/**
 * @brief Path of this application's configuration file.
 *
 * @return e.g. <tt>~/.config/odd-macrosrc</tt> on Linux.
 */
QString PlatformHelper::configFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + QStringLiteral(PROJECT_NAME) + "rc";
}

/**
 * @brief The full build ABI of the Qt runtime.
 *
 * @return value from @c QSysInfo::buildAbi().
 */
QString PlatformHelper::buildAbi()
{
    return QSysInfo::buildAbi();
}

/**
 * @brief The version string of the running kernel.
 *
 * @return value from @c QSysInfo::kernelVersion().
 */
QString PlatformHelper::kernelVersion()
{
    return QSysInfo::kernelVersion();
}

/**
 * @brief A human-readable product name of the operating system.
 *
 * @return value from @c QSysInfo::prettyProductName().
 */
QString PlatformHelper::productName()
{
    return QSysInfo::prettyProductName();
}


/**
 * @brief A helper function to detect the Desktop Environment
 *
 * Checks @c XDG_CURRENT_DESKTOP, @c DESKTOP_SESSION and
 * @c XDG_SESSION_DESKTOP in order and returns a normalised, lower-cased name.
 *
 * @return @c "kde", @c "gnome", the raw desktop name, or @c "unknown".
 */
QString PlatformHelper::detectDesktopEnvironment()
{
    // 1. Check XDG_CURRENT_DESKTOP first
    QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (!desktop.isEmpty()) {
        QString lowerDesktop = desktop.toLower();

        // Moderne KDE Plasma setter vanligvis "KDE" eller "Plasma"
        if (lowerDesktop.contains("kde") || lowerDesktop.contains("plasma")) {
            return "kde";
        }

        // GNOME og derivater
        if (lowerDesktop.contains("gnome") || lowerDesktop.contains("ubuntu") || lowerDesktop.contains("pop") || lowerDesktop.contains("cosmic")) {
            return "gnome";
        }

        return lowerDesktop;
    }

    // 2. Check DESKTOP_SESSION (For older systems)
    desktop = qEnvironmentVariable("DESKTOP_SESSION");
    if (!desktop.isEmpty()) {
        QString lowerDesktop = desktop.toLower();
        if (lowerDesktop.contains("plasma") || lowerDesktop.contains("kde")) {
            return "kde";
        }
        if (lowerDesktop.contains("gnome") || lowerDesktop.contains("ubuntu")) {
            return "gnome";
        }
        return lowerDesktop;
    }
    // 3. Sjekk XDG_SESSION_DESKTOP (nyere standard)
    desktop = qEnvironmentVariable("XDG_SESSION_DESKTOP");
    if (!desktop.isEmpty()) {
        QString lowerDesktop = desktop.toLower();
        if (lowerDesktop.contains("kde") || lowerDesktop.contains("plasma")) {
            return "kde";
        }
        if (lowerDesktop.contains("gnome")) {
            return "gnome";
        }
        return lowerDesktop;
    }

    return "unknown";
}

// ======================= QProcess helper functions for sandbox escapes ==========================/ 

/**
 * @brief Build a command line that runs on the host system.
 *
 * @param program   the program to run.
 * @param arguments optional program arguments.
 * @return the host-ready command line.
 */
QStringList PlatformHelper::makeHostContext(const QString &program, const QStringList &arguments)
{
    QStringList result;
    if (isFlatpak()) {
        // Bruk flatpak-spawn for å kjøre kommandoer på vertssystemet
        result << "flatpak-spawn" << "--host" << program;
        result << arguments;
    } else {
        result << program;
        result << arguments;
    }
    return result;
}

/**
 * @brief Check whether the running Flatpak grants @c flatpak-spawn talk access.
 *
 * @return @c true if the sandbox configuration allows host process spawning.
 */
bool checkHasFlatpakSpawnPrivileges()
{
    QFile f(QStringLiteral("/.flatpak-info"));
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }

    return f.readAll().contains("\norg.freedesktop.Flatpak=talk\n");
}

/**
 * @brief Build a host-ready @ref PlatformHelper::ProcessContext from a QProcess.
 *
 * @param process the source process definition.
 * @return the resulting host context.
 */
PlatformHelper::ProcessContext PlatformHelper::makeHostContext(QProcess &process)
{
    if (!isFlatpak()) {
        return {process.program(), process.arguments()};
    }
    static const bool hasFlatpakSpawnPrivileges = checkHasFlatpakSpawnPrivileges();
    if (!hasFlatpakSpawnPrivileges) {
        qCWarning(helper) << "Process execution expects 'org.freedesktop.Flatpak=talk'" << process.program();
        return {process.program(), process.arguments()};
    }
    QStringList args{QStringLiteral("--watch-bus"), QStringLiteral("--host"), QStringLiteral("--forward-fd=1"), QStringLiteral("--forward-fd=2")};
    if (!process.workingDirectory().isEmpty()) {
        args << QStringLiteral("--directory=%1").arg(process.workingDirectory());
    }
    const auto systemEnvironment = QProcessEnvironment::systemEnvironment().toStringList();
    const auto processEnvironment = process.processEnvironment().toStringList();
    for (const auto &variable : processEnvironment) {
        if (systemEnvironment.contains(variable)) {
            continue;
        }
        args << QStringLiteral("--env=%1").arg(variable);
    }
    if (!process.program().isEmpty()) { // some callers are cheeky and pass no program but put it into the arguments (e.g. konsole)
        args << process.program();
    }
    args += process.arguments();
    return {QStringLiteral("/usr/bin/flatpak-spawn"), args};

}

/**
 * @brief Start a process on the host system.
 *
 * @param process the process to start.
 * @param mode    whether to start synchronously or detached.
 */
void PlatformHelper::startHostProcess(QProcess &process, PlatformHelper::ProcessMode mode)
{
    const auto context = makeHostContext(process);
    switch (mode) {
        case PlatformHelper::ProcessMode::start:
            process.start(context.program, context.arguments);
            return;
        case PlatformHelper::ProcessMode::startDetached:
            process.startDetached(context.program, context.arguments);
            return;
       
    }


}
