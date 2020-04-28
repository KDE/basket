/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "global.h"

#include <KConfig>
#include <KMainWindow>

#include <QApplication>
#include <QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QString>

#include "bnpview.h"
#include "gitwrapper.h"
#include "settings.h"

/** Define initial values for global variables : */

QString Global::s_customSavesFolder;
LikeBack *Global::likeBack = 0L;
DebugWindow *Global::debugWindow = 0L;
BackgroundManager *Global::backgroundManager = 0L;
SystemTray *Global::systemTray = 0L;
BNPView *Global::bnpView = 0L;
KSharedConfig::Ptr Global::basketConfig;
QCommandLineParser *Global::commandLineOpts = NULL;
MainWindow *Global::mainWnd = NULL;

void Global::setCustomSavesFolder(const QString &folder)
{
    s_customSavesFolder = folder;
}

QString Global::savesFolder()
{
    static QString *folder = 0L; // Memorize the folder to do not have to re-compute it each time it's needed

    if (folder == 0L) {                       // Initialize it if not yet done
        if (!s_customSavesFolder.isEmpty()) { // Passed by command line (for development & debug purpose)
            QDir dir;
            dir.mkdir(s_customSavesFolder);
            folder = new QString(s_customSavesFolder.endsWith("/") ? s_customSavesFolder : s_customSavesFolder + '/');
        } else if (!Settings::dataFolder().isEmpty()) { // Set by config option (in Basket -> Backup & Restore)
            folder = new QString(Settings::dataFolder().endsWith("/") ? Settings::dataFolder() : Settings::dataFolder() + '/');
        } else { // The default path (should be that for most computers)
            folder = new QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "basket/");
            initializeGitIfNeeded(*folder);
        }
    }
    return *folder;
}

QString Global::basketsFolder()
{
    return savesFolder() + "baskets/";
}
QString Global::backgroundsFolder()
{
    return savesFolder() + "backgrounds/";
}
QString Global::templatesFolder()
{
    return savesFolder() + "templates/";
}
QString Global::tempCutFolder()
{
    return savesFolder() + "temp-cut/";
}
QString Global::gitFolder()
{
    return savesFolder() + ".git/";
}

void Global::initializeGitIfNeeded(QString savesFolder)
{
    if (!QDir(savesFolder + ".git/").exists()) {
        GitWrapper::initializeGitRepository(savesFolder);
    }
}

QString Global::openNoteIcon() // FIXME: Now an edit icon
{
    return QVariant(Global::bnpView->m_actEditNote->icon()).toString();
}

KMainWindow *Global::activeMainWindow()
{
    QWidget *res = qApp->activeWindow();

    if (res && res->inherits("KMainWindow")) {
        return static_cast<KMainWindow *>(res);
    }
    return 0;
}

MainWindow *Global::mainWindow()
{
    return mainWnd;
}

KConfig *Global::config()
{
    // The correct solution is to go and replace all KConfig* with KSharedConfig::Ptr, but that seems awfully annoying to do right now
    return Global::basketConfig.data();
}
