/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "debugwindow.h"

#include <QCloseEvent>
#include <QString>
#include <QVBoxLayout>

#include <QLocale>
#include <QTextBrowser>

#include <KLocalizedString>

#include "global.h"

DebugWindow::DebugWindow(QWidget *parent)
    : QWidget(parent)
{
    Global::debugWindow = this;
    setWindowTitle(i18n("Debug Window"));

    layout = new QVBoxLayout(this);
    textBrowser = new QTextBrowser(this);

    textBrowser->setWordWrapMode(QTextOption::NoWrap);

    layout->addWidget(textBrowser);
    textBrowser->show();
}

DebugWindow::~DebugWindow()
{
    delete textBrowser;
    delete layout;
}

void DebugWindow::postMessage(const QString msg)
{
    textBrowser->append(msg);
}

DebugWindow &DebugWindow::operator<<(const QString msg)
{
    // This can be used from a different thread
    QMetaObject::invokeMethod(this, "postMessage", Qt::QueuedConnection, Q_ARG(QString, msg));
    return *this;
}

void DebugWindow::insertHLine()
{
    textBrowser->append(QStringLiteral("<hr>"));
}

void DebugWindow::closeEvent(QCloseEvent *event)
{
    Global::debugWindow = nullptr;
    QWidget::closeEvent(event);
}

#ifdef DEBUG_PIPE
void debugMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile file(QStringLiteral("/tmp/basket_debug_pipe"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
        out << msg << Qt::endl;
        file.close();
    }
}
#endif

#include "moc_debugwindow.cpp"
