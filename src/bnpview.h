/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BNPVIEW_H
#define BNPVIEW_H

#include <QSplitter>
#include <QXmlStreamWriter>
#include <QtCore/QList>
#include <QtGui/QClipboard>

#include <KXMLGUIClient>

#include "basket_export.h"
#include "global.h"

class QDomElement;

class QStackedWidget;
class QPixmap;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoStack;

class QEvent;
class QHideEvent;
class QShowEvent;

class QAction;
class KToggleAction;
class QMenu;
class KTar;

class DesktopColorPicker;
class RegionGrabber;

class BasketScene;
class DecoratedBasket;
class BasketListView;
class BasketListViewItem;
class BasketTreeListView;
class NoteSelection;
class BasketStatusBar;
class Tag;
class State;
class Note;
class KMainWindow;

class BASKET_EXPORT BNPView : public QSplitter
{
    Q_OBJECT
    Q_CLASSINFO("D Bus Interface", "org.kde.basket.dbus");

public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    BNPView(QWidget *parent, const char *name, KXMLGUIClient *aGUIClient, KActionCollection *actionCollection, BasketStatusBar *bar);
    ~BNPView() override;
    /// MANAGE CONFIGURATION EVENTS!:
    void setTreePlacement(bool onLeft);
    void relayoutAllBaskets();
    void recomputeAllStyles();
    void removedStates(const QList<State *> &deletedStates);
    void linkLookChanged();
    void filterPlacementChanged(bool onTop);
    /// MANAGE BASKETS:
    BasketListViewItem *listViewItemForBasket(BasketScene *basket);
    BasketScene *currentBasket();
    BasketScene *parentBasketOf(BasketScene *basket);
    void setCurrentBasket(BasketScene *basket);
    void setCurrentBasketInHistory(BasketScene *basket);
    void removeBasket(BasketScene *basket);
    /// For NewBasketDialog (and later some other classes):
    int topLevelItemCount();
    ///
    BasketListViewItem *topLevelItem(int i);
    int basketCount(QTreeWidgetItem *parent = nullptr);
    bool canFold();
    bool canExpand();
    void enableActions();

private:
    //! Create <basket> element with <properties>
    void writeBasketElement(QTreeWidgetItem *item, QXmlStreamWriter &steam);
public slots:
    void countsChanged(BasketScene *basket);
    void notesStateChanged();
    bool convertTexts();

    void updateBasketListViewItem(BasketScene *basket);
    void save();
    void save(QTreeWidget *listView, QTreeWidgetItem *firstItem, QXmlStreamWriter &stream);
    void saveSubHierarchy(QTreeWidgetItem *item, QXmlStreamWriter &stream, bool recursive);
    void load();
    void load(QTreeWidgetItem *item, const QDomElement &baskets);
    void loadNewBasket(const QString &folderName, const QDomElement &properties, BasketScene *parent);
    void goToPreviousBasket();
    void goToNextBasket();
    void foldBasket();
    void expandBasket();
    void closeAllEditors();
    ///
    void toggleFilterAllBaskets(bool doFilter);
    void newFilter();
    void newFilterFromFilterBar();
    bool isFilteringAllBaskets();
    // From main window
    void importKNotes();
    void importKJots();
    void importKnowIt();
    void importTuxCards();
    void importStickyNotes();
    void importTomboy();
    void importJreepadFile();
    void importTextFile();
    void backupRestore();
    void checkCleanup();

    /** Note */
    void activatedTagShortcut();
    void exportToHTML();
    void editNote();
    void cutNote();
    void copyNote();
    void delNote();
    void openNote();
    void openNoteWith();
    void saveNoteAs();
    void noteGroup();
    void noteUngroup();
    void moveOnTop();
    void moveOnBottom();
    void moveNoteUp();
    void moveNoteDown();
    void slotSelectAll();
    void slotUnselectAll();
    void slotInvertSelection();
    void slotResetFilter();

    void slotColorFromScreen(bool global = false);
    void slotColorFromScreenGlobal();
    void colorPicked(const QColor &color);
    void colorPickingCanceled();
    void slotConvertTexts();

    /** Global shortcuts */
    void addNoteText();
    void addNoteHtml();
    void addNoteImage();
    void addNoteLink();
    void addNoteCrossReference();
    void addNoteColor();
    /** Passive Popups for Global Actions */
    void showPassiveDropped(const QString &title);
    void showPassiveDroppedDelayed(); // Do showPassiveDropped(), but delayed
    void showPassiveContent(bool forceShow = false);
    void showPassiveContentForced();
    void showPassiveImpossible(const QString &message);
    void showPassiveLoading(BasketScene *basket);
    // For GUI :
    void setFiltering(bool filtering);
    /** Edit */
    void undo();
    void redo();
    void globalPasteInCurrentBasket();
    void pasteInCurrentBasket();
    void pasteSelInCurrentBasket();
    void pasteToBasket(int index, QClipboard::Mode mode = QClipboard::Clipboard);
    void showHideFilterBar(bool show, bool switchFocus = true);
    /** Insert **/
    void insertEmpty(int type);
    void insertWizard(int type);
    void grabScreenshot(bool global = false);
    void grabScreenshotGlobal();
    void screenshotGrabbed(const QPixmap &pixmap);
    /** BasketScene */
    void askNewBasket();
    void askNewBasket(BasketScene *parent, BasketScene *pickProperties);
    void askNewSubBasket();
    void askNewSiblingBasket();
    void aboutToHideNewBasketPopup();
    void setNewBasketPopup();
    void cancelNewBasketPopup();
    void propBasket();
    void delBasket();
    void doBasketDeletion(BasketScene *basket);
    void password();
    void saveAsArchive();
    void openArchive();
    void delayedOpenArchive();
    void delayedOpenBasket();
    void lockBasket();
    void hideOnEscape();

    void changedSelectedNotes();
    void timeoutTryHide();
    void timeoutHide();

    void loadCrossReference(QString link);
    QString folderFromBasketNameLink(QStringList pages, QTreeWidgetItem *parent = nullptr);

    void sortChildrenAsc();
    void sortChildrenDesc();
    void sortSiblingsAsc();
    void sortSiblingsDesc();

public:
    static QString s_fileToOpen;
    static QString s_basketToOpen;

public slots:
    void addWelcomeBaskets();
private slots:
    void updateNotesActions();
    void slotBasketChanged();
    void canUndoRedoChanged();
    void currentBasketChanged();
    void isLockedChanged();
    void lateInit();
    void onFirstShow();

public:
    QAction *m_actEditNote;
    QAction *m_actOpenNote;
    QAction *m_actPaste;
    QAction *m_actGrabScreenshot;
    QAction *m_actColorPicker;
    QAction *m_actLockBasket;
    QAction *m_actPassBasket;
    QAction *actNewBasket;
    QAction *actNewSubBasket;
    QAction *actNewSiblingBasket;
    QAction *m_actHideWindow;
    QAction *m_actExportToHtml;
    QAction *m_actPropBasket;
    QAction *m_actSortChildrenAsc;
    QAction *m_actSortChildrenDesc;
    QAction *m_actSortSiblingsAsc;
    QAction *m_actSortSiblingsDesc;
    QAction *m_actDelBasket;
    KToggleAction *m_actFilterAllBaskets;

private:
    // Basket actions:
    QAction *m_actSaveAsArchive;
    QAction *m_actOpenArchive;
    // Notes actions :
    QAction *m_actOpenNoteWith;
    QAction *m_actSaveNoteAs;
    QAction *m_actGroup;
    QAction *m_actUngroup;
    QAction *m_actMoveOnTop;
    QAction *m_actMoveNoteUp;
    QAction *m_actMoveNoteDown;
    QAction *m_actMoveOnBottom;
    // Edit actions :
    QAction *m_actUndo;
    QAction *m_actRedo;
    QAction *m_actCutNote;
    QAction *m_actCopyNote;
    QAction *m_actDelNote;
    QAction *m_actSelectAll;
    QAction *m_actUnselectAll;
    QAction *m_actInvertSelection;
    // Insert actions :
    //      QAction *m_actInsertText;
    QAction *m_actInsertHtml;
    QAction *m_actInsertLink;
    QAction *m_actInsertCrossReference;
    QAction *m_actInsertImage;
    QAction *m_actInsertColor;
    QAction *m_actImportKMenu;
    QAction *m_actInsertLauncher;
    QAction *m_actImportIcon;
    QAction *m_actLoadFile;
    QList<QAction *> m_insertActions;
    // Basket actions :
    KToggleAction *m_actShowFilter;
    QAction *m_actResetFilter;
    // Go actions :
    QAction *m_actPreviousBasket;
    QAction *m_actNextBasket;
    QAction *m_actFoldBasket;
    QAction *m_actExpandBasket;
    //      QAction *m_convertTexts; // FOR_BETA_PURPOSE

    void setupActions();
    void setupGlobalShortcuts();
    DecoratedBasket *currentDecoratedBasket();

public:
    BasketScene *loadBasket(const QString &folderName);                                 // Public only for class Archive
    BasketListViewItem *appendBasket(BasketScene *basket, QTreeWidgetItem *parentItem); // Public only for class Archive

    BasketScene *basketForFolderName(const QString &folderName);
    Note *noteForFileName(const QString &fileName, BasketScene &basket, Note *note = nullptr);
    QMenu *popupMenu(const QString &menuName);
    bool isPart();
    bool isMainWindowActive();
    void showMainWindow();

    // TODO: dcop calls -- dbus these
public Q_SLOTS:
    Q_SCRIPTABLE void newBasket();
    Q_SCRIPTABLE void handleCommandLine();
    Q_SCRIPTABLE void reloadBasket(const QString &folderName);
    Q_SCRIPTABLE bool createNoteHtml(const QString content, const QString basket);
    Q_SCRIPTABLE QStringList listBaskets();
    Q_SCRIPTABLE bool createNoteFromFile(const QString url, const QString basket);
    Q_SCRIPTABLE bool changeNoteHtml(const QString content, const QString basket, const QString noteName);

public slots:
    void setWindowTitle(QString s);
    void updateStatusBarHint();
    void setSelectionStatus(QString s);
    void setLockStatus(bool isLocked);
    void postStatusbarMessage(const QString &);
    void setStatusBarHint(const QString &);
    void setUnsavedStatus(bool isUnsaved);
    void setActive(bool active = true);
    KActionCollection *actionCollection()
    {
        return m_actionCollection;
    };

    void populateTagsMenu();
    void populateTagsMenu(QMenu &menu, Note *referenceNote);
    void connectTagsMenu();
    void disconnectTagsMenu();
    void disconnectTagsMenuDelayed();

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;

private:
    QMenu *m_lastOpenedTagsMenu;

private slots:
    void slotPressed(QTreeWidgetItem *item, int column);
    void needSave(QTreeWidgetItem *);
    void slotContextMenu(const QPoint &pos);
    void slotShowProperties(QTreeWidgetItem *item);
    void initialize();

signals:
    void basketChanged();
    void setWindowCaption(const QString &s);
    void showPart();

protected:
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

protected:
    void hideMainWindow();

private:
    BasketTreeListView *m_tree;
    QStackedWidget *m_stack;
    bool m_loading;
    bool m_newBasketPopup;
    bool m_firstShow;
    DesktopColorPicker *m_colorPicker;
    bool m_colorPickWasShown;
    bool m_colorPickWasGlobal;
    RegionGrabber *m_regionGrabber;
    QString m_passiveDroppedTitle;
    NoteSelection *m_passiveDroppedSelection;
    static const int c_delayTooltipTime;
    KActionCollection *m_actionCollection;
    KXMLGUIClient *m_guiClient;
    BasketStatusBar *m_statusbar;
    QTimer *m_tryHideTimer;
    QTimer *m_hideTimer;

    QUndoStack *m_history;
    KMainWindow *m_HiddenMainWindow;
};

#endif // BNPVIEW_H
