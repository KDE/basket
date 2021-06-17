/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <KCModule>
#include <KMainWindow> //For Global::mainWindow()
#include <KCMultiDialog>

#include <QAction> //For UseSysTray
#include <QtCore/QDate>

#include "basket_export.h"
#include "bnpview.h"

class KComboBox;

class QString;
class QCheckBox;
class QPushButton;
class QPoint;
class QSize;
class QSpinBox;

class LinkLook;
class LinkLookEditWidget;
class RunCommandRequester;

class BASKET_EXPORT GeneralPage : public KCModule
{
    Q_OBJECT
public:
    explicit GeneralPage(QWidget *parent = nullptr, const QVariantList &args = {});
    ~GeneralPage() override
    {
    }

    void load() override;
    void save() override;
    void defaults() override;

private:
    // General
    KComboBox *m_treeOnLeft;
    KComboBox *m_filterOnTop;
};

class BASKET_EXPORT BasketsPage : public KCModule
{
    Q_OBJECT
public:
    explicit BasketsPage(QWidget *parent = nullptr, const QVariantList &args = {});

    void load() override;
    void save() override;
    void defaults() override;

private:
    // Appearance
    QCheckBox *m_playAnimations;
    QCheckBox *m_showNotesToolTip;
    QCheckBox *m_bigNotes;

    // Behavior
    QCheckBox *m_autoBullet;
    QCheckBox *m_confirmNoteDeletion;
    QCheckBox *m_exportTextTags;
    QCheckBox *m_detectTextTags;
    QWidget *m_groupOnInsertionLineWidget;
    QCheckBox *m_groupOnInsertionLine;
    KComboBox *m_middleAction;
    QCheckBox *m_pasteAsPlainText;

    // Protection
    QCheckBox *m_useGnuPGAgent;
    QCheckBox *m_enableReLockTimeoutMinutes;
    QSpinBox *m_reLockTimeoutMinutes;
};

class BASKET_EXPORT NewNotesPage : public KCModule
{
    Q_OBJECT
public:
    explicit NewNotesPage(QWidget *parent = nullptr, const QVariantList &args = {});

    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void visualize();

private:
    // Notes Image Size
    QSpinBox *m_imgSizeX;
    QSpinBox *m_imgSizeY;
    QPushButton *m_pushVisualize;

    // Note Addition
    KComboBox *m_newNotesPlace;
    QCheckBox *m_viewTextFileContent;
    QCheckBox *m_viewHtmlFileContent;
    QCheckBox *m_viewImageFileContent;
    QCheckBox *m_viewSoundFileContent;
};

class BASKET_EXPORT NotesAppearancePage : public KCModule
{
    Q_OBJECT
public:
    explicit NotesAppearancePage(QWidget *parent = nullptr, const QVariantList &args = {});

    void load() override;
    void save() override;
    void defaults() override;

private:
    // Link Looks
    LinkLookEditWidget *m_soundLook;
    LinkLookEditWidget *m_fileLook;
    LinkLookEditWidget *m_localLinkLook;
    LinkLookEditWidget *m_networkLinkLook;
    LinkLookEditWidget *m_launcherLook;
    LinkLookEditWidget *m_crossReferenceLook;
};

class BASKET_EXPORT ApplicationsPage : public KCModule
{
    Q_OBJECT
public:
    explicit ApplicationsPage(QWidget *parent = nullptr, const QVariantList &args = {});

    void load() override;
    void save() override;
    void defaults() override;

private:
    // Applications
    QCheckBox *m_htmlUseProg;
    QCheckBox *m_imageUseProg;
    QCheckBox *m_animationUseProg;
    QCheckBox *m_soundUseProg;
    RunCommandRequester *m_htmlProg;
    RunCommandRequester *m_imageProg;
    RunCommandRequester *m_animationProg;
    RunCommandRequester *m_soundProg;
};

/**
 * KSettings::Dialog wrapper to properly calculate appropriate window size
 */
class BASKET_EXPORT SettingsDialog : public KCMultiDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;
    virtual int exec() override;

private Q_SLOTS:
    void adjustSize();
};

/** Handle all global variables (to avoid lot of extern declarations)
 * @author Sébastien Laoût
 */
class BASKET_EXPORT Settings // FIXME: Dispatch new config events ?
{
protected:
    /** Main window */
    static bool s_treeOnLeft;
    static bool s_filterOnTop;
    static bool s_showNotesToolTip;
    static bool s_confirmNoteDeletion;
    static bool s_bigNotes;
    static bool s_autoBullet;
    static bool s_pasteAsPlainText;
    static bool s_exportTextTags;
    static bool s_detectTextTags;
    static bool s_useGnuPGAgent;
    static int s_middleAction; // O:Nothing ; 1:Paste ; 2:Text ; 3:Html ; 4:Image ; 5:Link ; 6:Launcher ; 7:Color
    static bool s_groupOnInsertionLine;
    static bool s_spellCheckTextNotes;
    static int s_basketTreeWidth;
    static bool s_welcomeBasketsAdded;
    static QString s_dataFolder;
    static QDate s_lastBackup;
    static QPoint s_mainWindowPosition;
    static QSize s_mainWindowSize;
    static bool s_showEmptyBasketInfo;
    static bool s_blinkedFilter;
    static bool s_enableReLockTimeout;
    static int s_reLockTimeoutMinutes;
    /** Note Addition */
    static int s_newNotesPlace; // 0:OnTop ; 1:OnBottom ; 2:AtCurrentNote
    static int s_viewTextFileContent;
    static int s_viewHtmlFileContent;
    static int s_viewImageFileContent;
    static int s_viewSoundFileContent;
    /** Programs */
    static bool s_htmlUseProg;
    static bool s_imageUseProg;
    static bool s_animationUseProg;
    static bool s_soundUseProg;
    static QString s_htmlProg;
    static QString s_imageProg;
    static QString s_animationProg;
    static QString s_soundProg;
    /** Insert Note Default Values */
    static int s_defImageX;
    static int s_defImageY;
    static int s_defIconSize;
    /** Version Sync */
    static bool s_versionSyncEnabled;

public: /* And the following methods are just getter / setters */
    /** App settings GET */
    static inline bool treeOnLeft()
    {
        return s_treeOnLeft;
    }
    static inline bool filterOnTop()
    {
        return s_filterOnTop;
    }
    static inline bool showNotesToolTip()
    {
        return s_showNotesToolTip;
    }
    static inline bool confirmNoteDeletion()
    {
        return s_confirmNoteDeletion;
    }
    static inline bool bigNotes()
    {
        return s_bigNotes;
    }
    static inline bool autoBullet()
    {
        return s_autoBullet;
    }
    static inline bool pasteAsPlainText()
    {
        return s_pasteAsPlainText;
    }
    static inline bool exportTextTags()
    {
        return s_exportTextTags;
    }
    static inline bool detectTextTags()
    {
        return s_detectTextTags;
    }
    static inline bool useGnuPGAgent()
    {
        return s_useGnuPGAgent;
    }
    static inline bool blinkedFilter()
    {
        return s_blinkedFilter;
    }
    static inline bool enableReLockTimeout()
    {
        return s_enableReLockTimeout;
    }
    static inline int reLockTimeoutMinutes()
    {
        return s_reLockTimeoutMinutes;
    }
    static inline int middleAction()
    {
        return s_middleAction;
    }
    static inline bool groupOnInsertionLine()
    {
        return s_groupOnInsertionLine;
    }
    static inline bool spellCheckTextNotes()
    {
        return s_spellCheckTextNotes;
    }
    static inline int basketTreeWidth()
    {
        return s_basketTreeWidth;
    }
    static inline int dropTimeToShow()
    {
        return 7;
    } // TODO: 700 ; TODO: There is certainly a KGlobalConfig ???
    static inline bool welcomeBasketsAdded()
    {
        return s_welcomeBasketsAdded;
    }
    static inline QString dataFolder()
    {
        return s_dataFolder;
    }
    static inline QDate lastBackup()
    {
        return s_lastBackup;
    }
    static inline QPoint mainWindowPosition()
    {
        return s_mainWindowPosition;
    }
    static inline QSize mainWindowSize()
    {
        return s_mainWindowSize;
    }
    static inline bool showEmptyBasketInfo()
    {
        return s_showEmptyBasketInfo;
    }
    /** Programs */
    static inline bool isHtmlUseProg()
    {
        return s_htmlUseProg;
    }
    static inline bool isImageUseProg()
    {
        return s_imageUseProg;
    }
    static inline bool isAnimationUseProg()
    {
        return s_animationUseProg;
    }
    static inline bool isSoundUseProg()
    {
        return s_soundUseProg;
    }
    static inline QString htmlProg()
    {
        return s_htmlProg;
    }
    static inline QString imageProg()
    {
        return s_imageProg;
    }
    static inline QString animationProg()
    {
        return s_animationProg;
    }
    static inline QString soundProg()
    {
        return s_soundProg;
    }
    /** Insert Note Default Values */
    static inline int defImageX()
    {
        return s_defImageX;
    }
    static inline int defImageY()
    {
        return s_defImageY;
    }
    static inline int defIconSize()
    {
        return s_defIconSize;
    }
    /** Note Addition */
    static inline int newNotesPlace()
    {
        return s_newNotesPlace;
    }
    static inline int viewTextFileContent()
    {
        return s_viewTextFileContent;
    }
    static inline int viewHtmlFileContent()
    {
        return s_viewHtmlFileContent;
    }
    static inline int viewImageFileContent()
    {
        return s_viewImageFileContent;
    }
    static inline int viewSoundFileContent()
    {
        return s_viewSoundFileContent;
    }
    /** Version Sync */
    static inline bool versionSyncEnabled()
    {
        return s_versionSyncEnabled;
    }

    /** App settings SET */
    static void setTreeOnLeft(bool onLeft)
    {
        s_treeOnLeft = onLeft;
        if (Global::bnpView)
            Global::bnpView->setTreePlacement(onLeft);
    }
    static void setFilterOnTop(bool onTop)
    {
        if (s_filterOnTop != onTop) {
            s_filterOnTop = onTop;
            if (Global::bnpView)
                Global::bnpView->filterPlacementChanged(onTop);
        }
    }
    static void setShowNotesToolTip(bool show)
    {
        s_showNotesToolTip = show;
    }
    static inline void setConfirmNoteDeletion(bool confirm)
    {
        s_confirmNoteDeletion = confirm;
    }
    static inline void setPasteAsPlainText(bool yes)
    {
        s_pasteAsPlainText = yes;
    }
    static void setBigNotes(bool big);
    static void setAutoBullet(bool yes);
    static inline void setExportTextTags(bool yes)
    {
        s_exportTextTags = yes;
    }
    static inline void setDetectTextTags(bool yes)
    {
        s_detectTextTags = yes;
    }
    static inline void setUseGnuPGAgent(bool yes)
    {
        s_useGnuPGAgent = yes;
    }
    static inline void setBlinkedFilter(bool blinked)
    {
        s_blinkedFilter = blinked;
    }
    static inline void setEnableReLockTimeout(bool yes)
    {
        s_enableReLockTimeout = yes;
    }
    static inline void setReLockTimeoutMinutes(int minutes)
    {
        s_reLockTimeoutMinutes = minutes;
    }
    static inline void setMiddleAction(int action)
    {
        s_middleAction = action;
    }
    static inline void setGroupOnInsertionLine(bool yes)
    {
        s_groupOnInsertionLine = yes;
    }
    static inline void setSpellCheckTextNotes(bool yes)
    {
        s_spellCheckTextNotes = yes;
    }
    static inline void setBasketTreeWidth(int width)
    {
        s_basketTreeWidth = width;
    }
    static inline void setWelcomeBasketsAdded(bool added)
    {
        s_welcomeBasketsAdded = added;
    }
    static inline void setDataFolder(const QString &folder)
    {
        s_dataFolder = folder;
    }
    static inline void setLastBackup(const QDate &date)
    {
        s_lastBackup = date;
    }
    static inline void setMainWindowPosition(const QPoint &pos)
    {
        s_mainWindowPosition = pos;
    }
    static inline void setMainWindowSize(const QSize &size)
    {
        s_mainWindowSize = size;
    }
    static inline void setShowEmptyBasketInfo(bool show)
    {
        s_showEmptyBasketInfo = show;
    }
    // Programs :
    static inline void setIsHtmlUseProg(bool useProg)
    {
        s_htmlUseProg = useProg;
    }
    static inline void setIsImageUseProg(bool useProg)
    {
        s_imageUseProg = useProg;
    }
    static inline void setIsAnimationUseProg(bool useProg)
    {
        s_animationUseProg = useProg;
    }
    static inline void setIsSoundUseProg(bool useProg)
    {
        s_soundUseProg = useProg;
    }
    static inline void setHtmlProg(const QString &prog)
    {
        s_htmlProg = prog;
    }
    static inline void setImageProg(const QString &prog)
    {
        s_imageProg = prog;
    }
    static inline void setAnimationProg(const QString &prog)
    {
        s_animationProg = prog;
    }
    static inline void setSoundProg(const QString &prog)
    {
        s_soundProg = prog;
    }
    // Insert Note Default Values :
    static inline void setDefImageX(int val)
    {
        s_defImageX = val;
    }
    static inline void setDefImageY(int val)
    {
        s_defImageY = val;
    }
    static inline void setDefIconSize(int val)
    {
        s_defIconSize = val;
    }
    // Note Addition
    static inline void setNewNotesPlace(int val)
    {
        s_newNotesPlace = val;
    }
    static inline void setViewTextFileContent(bool view)
    {
        s_viewTextFileContent = view;
    }
    static inline void setViewHtmlFileContent(bool view)
    {
        s_viewHtmlFileContent = view;
    }
    static inline void setViewImageFileContent(bool view)
    {
        s_viewImageFileContent = view;
    }
    static inline void setViewSoundFileContent(bool view)
    {
        s_viewSoundFileContent = view;
    }
    // Version Sync
    static inline void setVersionSyncEnabled(bool enable)
    {
        s_versionSyncEnabled = enable;
    }

public:
    /* Save and load config */
    static void loadConfig();
    static void saveConfig();

protected:
    static void loadLinkLook(LinkLook *look, const QString &name, const LinkLook &defaultLook);
    static void saveLinkLook(LinkLook *look, const QString &name);
};

#endif // SETTINGS_H
