/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QLocale>
#include <QStatusBar>
#include <QtGui/QMoveEvent>
#include <QtGui/QResizeEvent>

#include <KAboutData>
#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KEditToolBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShortcutsDialog>
#include <KToggleAction>

#include "basketstatusbar.h"
#include "bnpview.h"
#include "global.h"
#include "settings.h"

/** Container */

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_settings(nullptr)
    , m_quit(false)
{
    BasketStatusBar *bar = new BasketStatusBar(statusBar());
    m_baskets = new BNPView(this, "BNPViewApp", this, actionCollection(), bar);
    setCentralWidget(m_baskets);

    setupActions();
    statusBar()->show();
    statusBar()->setSizeGripEnabled(true);

    setAutoSaveSettings(/*groupName=*/QString::fromLatin1("MainWindow"), /*saveWindowSize=*//*FIXME:false:Why was it false??*/ true);

    //  m_actShowToolbar->setChecked(   toolBar()->isVisible()   );
    m_actShowStatusbar->setChecked(statusBar()->isVisible());
    connect(m_baskets, &BNPView::setWindowCaption, this, &MainWindow::setWindowTitle);

    //  InlineEditors::instance()->richTextToolBar();
    setStandardToolBarMenuEnabled(true);

    createGUI("basketui.rc");
    KConfigGroup group = KSharedConfig::openConfig()->group(autoSaveGroup());
    applyMainWindowSettings(group);
}

MainWindow::~MainWindow()
{
    KConfigGroup group = KSharedConfig::openConfig()->group(autoSaveGroup());
    saveMainWindowSettings(group);
    delete m_settings;
    delete m_baskets;
}

void MainWindow::setupActions()
{
    actQuit = KStandardAction::quit(this, SLOT(quit()), actionCollection());
    QAction *a = nullptr;
    a = actionCollection()->addAction("minimizeRestore", this, SLOT(minimizeRestore()));
    a->setText(i18n("Minimize"));
    a->setIcon(QIcon::fromTheme(QString()));
    a->setShortcut(0);

    /** Settings : ************************************************************/
    //  m_actShowToolbar   = KStandardAction::showToolbar(   this, SLOT(toggleToolBar()),   actionCollection());
    m_actShowStatusbar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection());

    //  m_actShowToolbar->setCheckedState( KGuiItem(i18n("Hide &Toolbar")) );

    (void)KStandardAction::keyBindings(this, SLOT(showShortcutsSettingsDialog()), actionCollection());

    (void)KStandardAction::configureToolbars(this, SLOT(configureToolbars()), actionCollection());

    // QAction *actCfgNotifs = KStandardAction::configureNotifications(this, SLOT(configureNotifications()), actionCollection() );
    // actCfgNotifs->setEnabled(false); // Not yet implemented !

    actAppConfig = KStandardAction::preferences(this, SLOT(showSettingsDialog()), actionCollection());
}

/*void MainWindow::toggleToolBar()
{
    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();

    saveMainWindowSettings( KSharedConfig::openConfig(), autoSaveGroup() );
}*/

void MainWindow::toggleStatusBar()
{
    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();

    KConfigGroup group = KSharedConfig::openConfig()->group(autoSaveGroup());
    saveMainWindowSettings(group);
}

void MainWindow::configureToolbars()
{
    KConfigGroup group = KSharedConfig::openConfig()->group(autoSaveGroup());
    saveMainWindowSettings(group);

    KEditToolBar dlg(actionCollection());
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &MainWindow::slotNewToolbarConfig);
    dlg.exec();
}

void MainWindow::configureNotifications()
{
    // TODO
    // KNotifyDialog *dialog = new KNotifyDialog(this, "KNotifyDialog", false);
    // dialog->show();
}

void MainWindow::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    // ...if you use any action list, use plugActionList on each here...
    createGUI("basketui.rc"); // TODO: Reconnect tags menu aboutToShow() ??
    // TODO: Does this do anything?
    plugActionList(QString::fromLatin1("go_baskets_list"), actBasketsList);
    KConfigGroup group = KSharedConfig::openConfig()->group(autoSaveGroup());
    applyMainWindowSettings(group);
}

void MainWindow::showSettingsDialog()
{
    if (!m_settings)
        m_settings = new SettingsDialog (qApp->activeWindow());

    if (Global::activeMainWindow()) {
        m_settings->exec();
        return;
    }

    m_settings->show();
}

void MainWindow::showShortcutsSettingsDialog()
{
    KShortcutsDialog::configure(actionCollection());
    //.setWindowTitle(..)
    // actionCollection()->writeSettings();
}

void MainWindow::ensurePolished()
{
    bool shouldSave = false;

    // If position and size has never been set, set nice ones:
    //  - Set size to sizeHint()
    //  - Keep the window manager placing the window where it want and save this
    if (Settings::mainWindowSize().isEmpty()) {
        //      qDebug() << "Main Window Position: Initial Set in show()";
        int defaultWidth = qApp->desktop()->width() * 5 / 6;
        int defaultHeight = qApp->desktop()->height() * 5 / 6;
        resize(defaultWidth, defaultHeight); // sizeHint() is bad (too small) and we want the user to have a good default area size
        shouldSave = true;
    } else {
        //      qDebug() << "Main Window Position: Recall in show(x="
        //                << Settings::mainWindowPosition().x() << ", y=" << Settings::mainWindowPosition().y()
        //                << ", width=" << Settings::mainWindowSize().width() << ", height=" << Settings::mainWindowSize().height()
        //                << ")";
        // move(Settings::mainWindowPosition());
        // resize(Settings::mainWindowSize());
    }

    KXmlGuiWindow::ensurePolished();

    if (shouldSave) {
        //      qDebug() << "Main Window Position: Save size and position in show(x="
        //                << pos().x() << ", y=" << pos().y()
        //                << ", width=" << size().width() << ", height=" << size().height()
        //                << ")";
        Settings::setMainWindowPosition(pos());
        Settings::setMainWindowSize(size());
        Settings::saveConfig();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    //  qDebug() << "Main Window Position: Save size in resizeEvent(width=" << size().width() << ", height=" << size().height() << ") ; isMaximized="
    //            << (isMaximized() ? "true" : "false");
    Settings::setMainWindowSize(size());
    Settings::saveConfig();

    // Added to make it work (previous lines do not work):
    // saveMainWindowSettings( KSharedConfig::openConfig(), autoSaveGroup() );
    KXmlGuiWindow::resizeEvent(event);
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    //  qDebug() << "Main Window Position: Save position in moveEvent(x=" << pos().x() << ", y=" << pos().y() << ")";
    Settings::setMainWindowPosition(pos());
    Settings::saveConfig();

    // Added to make it work (previous lines do not work):
    // saveMainWindowSettings( KSharedConfig::openConfig(), autoSaveGroup() );
    KXmlGuiWindow::moveEvent(event);
}

bool MainWindow::queryExit()
{
    hide();
    return true;
}

void MainWindow::quit()
{
    m_quit = true;
    close();
}

bool MainWindow::queryClose()
{
    /*  if (m_shuttingDown) // Set in askForQuit(): we don't have to ask again
        return true;*/

    if (qApp->isSavingSession()) {
        Settings::saveConfig();
        return true;
    }

    return askForQuit();
}

bool MainWindow::askForQuit()
{
    QString message = i18n("<p>Do you really want to quit %1?</p>", QGuiApplication::applicationDisplayName());

    int really = KMessageBox::warningContinueCancel(this, message, i18n("Quit Confirm"), KStandardGuiItem::quit(), KStandardGuiItem::cancel(), "confirmQuitAsking");

    if (really == KMessageBox::Cancel) {
        m_quit = false;
        return false;
    }

    return true;
}

void MainWindow::minimizeRestore()
{
    if (isVisible())
        hide();
    else
        show();
}
