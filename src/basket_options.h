/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef BASKET_OPTIONS_H
#define BASKET_OPTIONS_H

#include <QCommandLineParser>
#include <QApplication>

#include <KLocalizedString>

#include "global.h"
#include "aboutdata.h"



void setupCmdLineOptions(QCommandLineParser *opts)
{
    opts->addHelpOption();
    opts->addVersionOption();
    Global::basketAbout.setupCommandLine(opts); //--author, --license

    opts->addOption(QCommandLineOption(QStringList() << "d" << "debug",
                                       i18n("Show the debug window")));
    opts->addOption(QCommandLineOption(QStringList() << "f" << "data-folder",
                                       i18n("Custom folder to load and save baskets and other application data."),
                                       i18nc("Command line help: --data-folder <FOLDER>", "folder")));
    opts->addOption(QCommandLineOption("start-hidden",
                                       i18n("Automatically hide the main window in the system tray on startup."))); //
    opts->addOption(QCommandLineOption(QStringList() << "k" << "use-drkonqi",
                                       i18n("On crash, use the standard KDE crash handler rather than send an email.")));

    opts->addPositionalArgument("file", i18n("Open a basket archive or template."));
}

#endif // BASKET_OPTIONS_H
