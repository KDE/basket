/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKET_OPTIONS_H
#define BASKET_OPTIONS_H

#include <QApplication>
#include <QCommandLineParser>

#include <KLocalizedString>

#include "aboutdata.h"
#include "global.h"

void setupCmdLineOptions(QCommandLineParser *opts)
{
    opts->addOption(QCommandLineOption(QStringList() << QStringLiteral("d") << QStringLiteral("debug"), i18n("Show the debug window")));
    opts->addOption(QCommandLineOption(QStringList() << QStringLiteral("f") << QStringLiteral("data-folder"),
                                       i18n("Custom folder to load and save baskets and other application data."),
                                       i18nc("Command line help: --data-folder <FOLDER>", "folder")));
    opts->addOption(QCommandLineOption(QStringLiteral("start-hidden"),
                                       i18n("Automatically hide the main window in the system tray on startup."))); //
    opts->addOption(QCommandLineOption(QStringList() << QStringLiteral("k") << QStringLiteral("use-drkonqi"),
                                       i18n("On crash, use the standard KDE crash handler rather than send an email.")));

    opts->addPositionalArgument(QStringLiteral("file"), i18n("Open a basket archive or template."));
}

#endif // BASKET_OPTIONS_H
