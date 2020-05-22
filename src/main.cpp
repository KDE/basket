/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basket_options.h"
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <config.h>
#include <kconfig.h> // TMP IN ALPHA 1

#include "application.h"
#include "backup.h"
#include "global.h"
#include "kde4_migration.h"
#include "mainwindow.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    const char *argv0 = (argc >= 1 ? argv[0] : "");

    // enable high dpi support
    Application::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    Application::setAttribute(Qt::AA_EnableHighDpiScaling, true);

    Global::commandLineOpts = new QCommandLineParser();
    Application app(argc, argv);

    QCommandLineParser *opts = Global::commandLineOpts;
    KAboutData::applicationData().setupCommandLine(opts); //--author, --license
    setupCmdLineOptions(opts);
    opts->process(app);
    KAboutData::applicationData().processCommandLine(opts); // show author, license information and exit

    {
        Kde4Migrator migrator;
        if (migrator.migrateKde4Data())
            migrator.showPostMigrationDialog();
    }

    app.tryLoadFile(opts->positionalArguments(), QDir::currentPath());

    // Initialize the config file
    Global::basketConfig = KSharedConfig::openConfig("basketrc");

    Backup::figureOutBinaryPath(argv0, app);

    /* Main Window */
    MainWindow *win = new MainWindow();
    Global::mainWnd = win;
    Global::bnpView->handleCommandLine();
    app.setActiveWindow(win);

    if (Settings::useSystray()) {
        // The user wanted to not show the window (but it is already hidden by default, so we do nothing):
        if (opts->isSet(QCommandLineOption("start-hidden")))
            ;
        // When the application is restored by the desktop session, restore its state:
        else if (app.isSessionRestored()) {
            if (!Settings::startDocked())
                win->show();
        }
        // Else, the application has been launched explicitly by the user (QMenu, keyboard shortcut...), so he need it, we show it:
        else
            win->show();
    } else
        // No system tray icon: always show:
        win->show();

    // Self-test of the presence of basketui.rc (the only required file after basket executable)
    if (Global::bnpView->popupMenu("basket") == nullptr)
        // An error message will be show by BNPView::popupMenu()
        return 1;

    /* Go */
    int result = app.exec();
    exit(result); // Do not clean up memory to not crash while deleting the QApplication, or do not hang up on session exit
}
