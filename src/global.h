/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include "basket_export.h"
#include <KSharedConfig>

class QString;

class KMainWindow;

class DebugWindow;
class BackgroundManager;
class SystemTray;
class BNPView;
class QCommandLineParser;

class MainWindow;

/** Handle all global variables of the application.
 * This file only declare classes : developer should include
 * the .h files of variables he use.
 * @author Sébastien Laoût
 */
class BASKET_EXPORT Global
{
private:
    static QString s_customSavesFolder;

    static void initializeGitRepository(QString folder);

public:
    // Global Variables:
    static DebugWindow *debugWindow;
    static BackgroundManager *backgroundManager;
    static BNPView *bnpView;
    static KSharedConfig::Ptr basketConfig;
    static QCommandLineParser *commandLineOpts;
    static MainWindow *mainWnd;

    // Application Folders:
    static void setCustomSavesFolder(const QString &folder);
    static QString savesFolder(); /// << @return e.g. "/home/username/.local/share/basket/".
    static QString basketsFolder(); /// << @return e.g. "/home/username/.local/share/basket/baskets/".
    static QString backgroundsFolder(); /// << @return e.g. "/home/username/.local/share/basket/backgrounds/".
    static QString templatesFolder(); /// << @return e.g. "/home/username/.local/share/basket/templates/".
    static QString tempCutFolder(); /// << @return e.g. "/home/username/.local/share/basket/temp-cut/".   (was ".tmp/")
    static QString gitFolder(); /// << @return e.g. "/home/username/.local/share/basket/.git/".

    // Various Things:
    /** Initialize git repository if Version sync is enabled
        @param savesFolder Path returned by savesFolder()  */
    static void initializeGitIfNeeded(QString savesFolder);
    static QString openNoteIcon(); /// << @return the icon used for the "Open" action on notes.
    static KMainWindow *activeMainWindow(); /// << @returns Main window if it has focus (is active), otherwise NULL
    static MainWindow *mainWindow(); /// << @returns Main window (always not NULL after it has been actually created)
    static KConfig *config();
};

#endif // GLOBAL_H
