/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "global.h"

#include <KConfig>
#include <KMainWindow>

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QString>

#include "bnpview.h"
#include "gitwrapper.h"
#include "settings.h"

/** Define initial values for global variables : */

QString Global::s_customSavesFolder;
DebugWindow *Global::debugWindow = nullptr;
BackgroundManager *Global::backgroundManager = nullptr;
BNPView *Global::bnpView = nullptr;
KSharedConfig::Ptr Global::basketConfig;
QCommandLineParser *Global::commandLineOpts = nullptr;
MainWindow *Global::mainWnd = nullptr;

void Global::setCustomSavesFolder(const QString &folder)
{
    s_customSavesFolder = folder;
}

QString Global::savesFolder()
{
    static QString *folder = nullptr; // Memorize the folder to do not have to re-compute it each time it's needed

    if (folder == nullptr) { // Initialize it if not yet done
        if (!s_customSavesFolder.isEmpty()) { // Passed by command line (for development & debug purpose)
            QDir dir;
            dir.mkdir(s_customSavesFolder);
            folder = new QString(s_customSavesFolder.endsWith(QStringLiteral("/")) ? s_customSavesFolder : s_customSavesFolder + QLatin1Char('/'));
        } else if (!Settings::dataFolder().isEmpty()) { // Set by config option (in Basket -> Backup & Restore)
            folder = new QString(Settings::dataFolder().endsWith(QStringLiteral("/")) ? Settings::dataFolder() : Settings::dataFolder() + QLatin1Char('/'));
        } else { // The default path (should be that for most computers)
            folder = new QString(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + QStringLiteral("basket/"));
            initializeGitIfNeeded(*folder);
        }
    }
    return *folder;
}

QString Global::basketsFolder()
{
    return savesFolder() + QStringLiteral("baskets/");
}
QString Global::backgroundsFolder()
{
    return savesFolder() + QStringLiteral("backgrounds/");
}
QString Global::templatesFolder()
{
    return savesFolder() + QStringLiteral("templates/");
}
QString Global::tempCutFolder()
{
    return savesFolder() + QStringLiteral("temp-cut/");
}
QString Global::gitFolder()
{
    return savesFolder() + QStringLiteral(".git/");
}

void Global::initializeGitIfNeeded(QString savesFolder)
{
    if (!QDir(savesFolder + QStringLiteral(".git/")).exists()) {
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
    return nullptr;
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
