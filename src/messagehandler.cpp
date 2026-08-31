// SPDX-FileCopyrightText: 2025 Odd Østlie <theoddpirate@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

/**
 * @file messagehandler.cpp
 * @brief Implementation of the custom Qt message handler and logging setup.
 *
 * @ingroup odd-macros-core
 */

#include "messagehandler.h"
#include "platformhelper.h"
#include <QSystemTrayIcon>
#include <QDateTime>
#include <QStandardPaths>
#include <cstdio>

/**
 * @brief Definition of the main application-wide logging category.
 *
 * The category string is derived from @c LOG_CAT(Main), which expands to
 * @c PROJECT_NAME.Main (for example @c odd-macros.Main).
 */
Q_LOGGING_CATEGORY(mainlog, LOG_CAT(Main))

/**
 * @brief The previously installed Qt message handler.
 *
 * Populated by @ref initLogging() when the custom handler is installed.
 */
QtMessageHandler originalHandler = nullptr;

/**
 * @brief Install the application-wide custom message handler.
 *
 * @sa messageHandler()
 */
void initLogging()
{
    originalHandler = qInstallMessageHandler(messageHandler);
}

/**
 * @brief Format and emit log messages to stderr.
 *
 * Builds a line of the form <tt>[timestamp] [level] [category] message</tt>
 * and writes it to stderr with an ANSI colour chosen from the message type.
 * Lines are only written if the originating category contains the project
 * name, which keeps unrelated library output out of the logs.
 *
 * @param type    the message type
 * @param context the logging context (file, line, function, category)
 * @param msg     the message body
 */
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    QString level;
    const char *color = "";
    const char *reset = "\033[0m";

    switch (type) {
    case QtDebugMsg:
        color = "\033[90m"; // bright black (gray)
        level = "DEBUG";
        break;
    case QtInfoMsg:
        color = "\033[32m"; // green
        level = "INFO";
        break;
    case QtWarningMsg:
        color = "\033[33m"; // yellow
        level = "WARN";
        break;
    case QtCriticalMsg:
        color = "\033[31m"; // red
        level = "ERROR";
        break;
    case QtFatalMsg:
        color = "\033[1;31m"; // bold red
        level = "FATAL";
        break;
    }

    QString line = QString("[%1] [%2] [%3] %4").arg(timestamp, level, context.category, msg);
    // 1. Print to the terminal
    if(QString(context.category).contains(QString(PROJECT_NAME)))
        fprintf(stderr, "%s%s%s\n", color, line.toUtf8().constData(), reset);

    if (type == QtFatalMsg) {
     //   abort();
    }
}
