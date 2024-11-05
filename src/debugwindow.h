/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QLatin1String>
#include <QStringLiteral>

class QVBoxLayout;
class QTextBrowser;
class QString;
class QCloseEvent;

/**A simple window that display text through debugging messages.
 *@author Sébastien Laoût
 */

class DebugWindow : public QWidget
{
    Q_OBJECT
public:
    /** Constructor and destructor */
    explicit DebugWindow(QWidget *parent = nullptr);
    ~DebugWindow() override;
    /** Methods to post a message to the debug window */
    Q_INVOKABLE void postMessage(const QString msg);
    DebugWindow &operator<<(const QString msg);
    void insertHLine();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QVBoxLayout *layout;
    QTextBrowser *textBrowser;
};

#ifdef DEBUG_PIPE
void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
#endif

#define DEBUG_WIN                                                                                                                                                                                                                              \
    if (Global::debugWindow)                                                                                                                                                                                                                   \
    *Global::debugWindow

#endif // DEBUGWINDOW_H
