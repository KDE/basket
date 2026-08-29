/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basket_options.h"
#include <KCrash>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <config.h>
#include <kconfig.h> // TMP IN ALPHA 1

#include "application.h"
#include "backup.h"
#include "global.h"
#include "mainwindow.h"
#ifdef DEBUG_PIPE
#include "debugwindow.h"
#endif
#include "settings.h"

int main(int argc, char *argv[])
{
    const char *argv0 = (argc >= 1 ? argv[0] : "");

    Global::commandLineOpts = new QCommandLineParser();
    Application app(argc, argv);

    KCrash::initialize();

    QCommandLineParser *opts = Global::commandLineOpts;
    KAboutData::applicationData().setupCommandLine(opts); //--author, --license
    setupCmdLineOptions(opts);
    opts->process(app);
    KAboutData::applicationData().processCommandLine(opts); // show author, license information and exit
    // Custom data folder;
    // the own block is to to not keep variables live for the whole application lifetime
    {
        const QString customDataFolder = opts->value(QStringLiteral("data-folder"));
        if (!customDataFolder.isEmpty()) {
            Global::setCustomSavesFolder(customDataFolder);
        }
    }
    app.tryLoadFile(opts->positionalArguments(), QDir::currentPath());

    // Initialize the config file
    Global::basketConfig = KSharedConfig::openConfig(QStringLiteral("basketrc"));

    Backup::figureOutBinaryPath(argv0, app);

    /* Main Window */
    auto *win = new MainWindow();
    app.setMainWindow(win);
    /* Debug mode */
    if (opts->isSet(QStringLiteral("debug")))
        Global::bnpView->enableDebugMode();
    app.setActiveWindow(win);

    win->show();

    // Self-test of the presence of basketui.rc (the only required file after basket executable)
    if (Global::bnpView->popupMenu(QStringLiteral("basket")) == nullptr)
        // An error message will be show by BNPView::popupMenu()
        return 1;

#ifdef DEBUG_PIPE
    // Install the debug message handler for external usage
    qInstallMessageHandler(debugMessageHandler);
#endif

    /* Go */
    int result = app.exec();
    app.setMainWindow(nullptr);
    exit(result); // Do not clean up memory to not crash while deleting the QApplication, or do not hang up on session exit
}
