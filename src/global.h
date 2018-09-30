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

#ifndef GLOBAL_H
#define GLOBAL_H

#include "basket_export.h"
#include <KSharedConfig>

class QString;

class KMainWindow;
class KAboutData;

class AboutData;
class LikeBack;
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
    static LikeBack          *likeBack;
    static DebugWindow       *debugWindow;
    static BackgroundManager *backgroundManager;
    static SystemTray        *systemTray;
    static BNPView           *bnpView;
    static KSharedConfig::Ptr basketConfig;
    static AboutData          basketAbout;
    static QCommandLineParser* commandLineOpts;
    static MainWindow*        mainWnd;



    // Application Folders:
    static void setCustomSavesFolder(const QString &folder);
    static QString savesFolder();       /// << @return e.g. "/home/username/.local/share/basket/".
    static QString basketsFolder();     /// << @return e.g. "/home/username/.local/share/basket/baskets/".
    static QString backgroundsFolder(); /// << @return e.g. "/home/username/.local/share/basket/backgrounds/".
    static QString templatesFolder();   /// << @return e.g. "/home/username/.local/share/basket/templates/".
    static QString tempCutFolder();     /// << @return e.g. "/home/username/.local/share/basket/temp-cut/".   (was ".tmp/")
    static QString gitFolder();         /// << @return e.g. "/home/username/.local/share/basket/.git/".

    // Various Things:
    /** Initialize git repository if Version sync is enabled
        @param savesFolder Path returned by savesFolder()  */
    static void initializeGitIfNeeded(QString savesFolder);
    static QString openNoteIcon();      /// << @return the icon used for the "Open" action on notes.
    static KMainWindow* activeMainWindow(); /// << @returns Main window if it has focus (is active), otherwise NULL
    static MainWindow* mainWindow(); /// << @returns Main window (always not NULL after it has been actually created)
    static KConfig* config();
    static KAboutData* about();
};

#endif // GLOBAL_H
