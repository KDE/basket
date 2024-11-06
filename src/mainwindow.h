/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CONTAINER_H
#define CONTAINER_H

#include <KXmlGuiWindow>

class QResizeEvent;
class QVBoxLayout;
class QMoveEvent;
class QWidget;
class QAction;
class KToggleAction;
class BNPView;
class SettingsDialog;

/** The window that contain baskets, organized by tabs.
 * @author Sébastien Laoût
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /** Constructor, initializer and destructor */
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupActions();
public Q_SLOTS:
    bool askForQuit();
    /** Settings **/
    SettingsDialog *settings();
    //  void toggleToolBar();
    void toggleStatusBar();
    void showShortcutsSettingsDialog();
    void configureToolbars() override;
    void configureNotifications();
    void showSettingsDialog();
    void minimizeRestore();
    void quit();
    void slotNewToolbarConfig();

protected:
    bool queryExit();
    bool queryClose() override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;

public:
    void ensurePolished();

private:
    // Settings actions :
    //  KToggleAction *m_actShowToolbar;
    KToggleAction *m_actShowStatusbar;
    QAction *actQuit;
    QAction *actAppConfig;
    QList<QAction *> actBasketsList;

private:
    QVBoxLayout *m_layout;
    BNPView *m_baskets;
    bool m_startDocked;
    SettingsDialog *m_settings;
    bool m_quit;
};

#endif // CONTAINER_H
