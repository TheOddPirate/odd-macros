#ifndef KIOT_MESSAGEHANDLER_H
#define KIOT_MESSAGEHANDLER_H

#include <QLoggingCategory>
#include <QtGlobal>

/**
 * @file messagehandler.h
 * @brief Custom Qt message handler and logging configuration.
 *
 * This header declares the application-wide logging categories and the custom
 * message handler used to format and route log output. It is the single place
 * where the top-level logging category (@c mainlog) is defined.
 *
 * The custom handler stamps every message with a timestamp, a human-readable
 * log level and the originating category, and emits it to stderr with
 * ANSI colouring. Fatal messages additionally terminate the application.
 *
 * @ingroup odd-macros-core
 * @author Odd Østlie &lt;theoddpirate@gmail.com&gt;
 * @since 0.1
 */

/**
 * @brief The main application-wide logging category.
 *
 * All log output produced by the daemon core should use this category so that
 * it carries a consistent category string and can be filtered uniformly with
 * the Qt logging facility.
 *
 * This category is declared here but defined in @c messagehandler.cpp.
 */
Q_DECLARE_LOGGING_CATEGORY(mainlog)

/**
 * @brief The previously installed Qt message handler.
 *
 * Saved by @ref initLogging() before installing the custom handler. It is kept
 * so that the original handler can be restored or chained if ever needed.
 */
extern QtMessageHandler originalHandler;

/**
 * @brief Initialise the logging configuration.
 *
 * Installs the application-wide custom message handler and stores the previous
 * one in @ref originalHandler. This must be called early in @c main(), before
 * any logging is performed, so that no message is emitted through the default
 * handler first.
 */
void initLogging();

/**
 * @brief Custom Qt message handler for the odd-macros application.
 *
 * Formats a log message with a UTC ISO timestamp, the log level, the logging
 * category and the message body, then writes it to stderr. Non-fatal messages
 * are colourised with ANSI escape sequences. For @c QtFatalMsg the application
 * is terminated with @c abort() after the message has been written.
 *
 * @param type    the message type (@c QtDebugMsg, @c QtInfoMsg, ...)
 * @param context the logging context holding file, line, function and category
 * @param msg     the actual message text
 */
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif // KIOT_MESSAGEHANDLER_H
