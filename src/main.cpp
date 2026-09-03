/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <config.h>
#include <kconfig.h> // TMP IN ALPHA 1

#include "application.h"
#include "backup.h"
#include "bnpview.h"
#include "global.h"
#include "mainwindow.h"
#ifdef DEBUG_PIPE
#include "debugwindow.h"
#endif
#include "settings.h"

int main(int argc, char *argv[])
{
    const char *argv0 = (argc >= 1 ? argv[0] : "");

    Application app(argc, argv);

    KCrash::initialize();

    QCommandLineParser opts;
    opts.addOption(QCommandLineOption(QStringList() << QStringLiteral("d") << QStringLiteral("debug"), i18n("Show the debug window")));
    opts.addOption(QCommandLineOption(QStringList() << QStringLiteral("f") << QStringLiteral("data-folder"),
                                      i18n("Custom folder to load and save baskets and other application data."),
                                      i18nc("Command line help: --data-folder <FOLDER>", "folder")));
    opts.addOption(QCommandLineOption(QStringLiteral("start-hidden"),
                                      i18n("Automatically hide the main window in the system tray on startup."))); //

    opts.addPositionalArgument(QStringLiteral("file"), i18n("Open a basket archive or template."));
    KAboutData::applicationData().setupCommandLine(&opts); //--author, --license
    opts.process(app);
    KAboutData::applicationData().processCommandLine(&opts); // show author, license information and exit
    KDBusService service(KDBusService::Unique);
    QObject::connect(&service, &KDBusService::activateRequested, &app, &Application::onActivateRequested);
    // Custom data folder;
    // the own block is to to not keep variables live for the whole application lifetime
    {
        const QString customDataFolder = opts.value(QStringLiteral("data-folder"));
        if (!customDataFolder.isEmpty()) {
            Global::setCustomSavesFolder(customDataFolder);
        }
    }
    app.tryLoadFile(opts.positionalArguments(), QDir::currentPath());

    // Initialize the config file
    Global::basketConfig = KSharedConfig::openConfig(QStringLiteral("basketrc"));

    Backup::figureOutBinaryPath(argv0, app);

    /* Main Window */
    auto *win = new MainWindow();
    app.setMainWindow(win);
    /* Debug mode */
    if (opts.isSet(QStringLiteral("debug")))
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
