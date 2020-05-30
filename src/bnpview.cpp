/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bnpview.h"
#include "common.h"

#include <QAction>
#include <QApplication>
#include <QCommandLineParser>
#include <QGraphicsView>
#include <QMenu>
#include <QProgressDialog>
#include <QStackedWidget>
#include <QUndoStack>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QRegExp>
#include <QtGui/QHideEvent>
#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPixmap>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtXml/QDomDocument>

#include <KAboutData>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPassivePopup>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KWindowSystem>
#include <KXMLGUIFactory>

#ifndef BASKET_USE_DRKONQI
#include <KCrash>
#endif // BASKET_USE_DRKONQI

#include <cstdlib>
#include <unistd.h> // usleep

#include "archive.h"
#include "backgroundmanager.h"
#include "backup.h"
#include "basketfactory.h"
#include "basketlistview.h"
#include "basketproperties.h"
#include "basketscene.h"
#include "basketstatusbar.h"
#include "colorpicker.h"
#include "debugwindow.h"
#include "decoratedbasket.h"
#include "formatimporter.h"
#include "gitwrapper.h"
#include "history.h"
#include "htmlexporter.h"
#include "icon_names.h"
#include "newbasketdialog.h"
#include "notedrag.h"
#include "noteedit.h" // To launch InlineEditors::initToolBars()
#include "notefactory.h"
#include "password.h"
#include "regiongrabber.h"
#include "settings.h"
#include "softwareimporters.h"
#include "tools.h"
#include "xmlwork.h"

#include <QFileDialog>
#include <QResource>
#include <QStandardPaths>
#include <qdbusconnection.h>

//#include "bnpviewadaptor.h"

/** class BNPView: */

const int BNPView::c_delayTooltipTime = 275;

BNPView::BNPView(QWidget *parent, const char *name, KXMLGUIClient *aGUIClient, KActionCollection *actionCollection, BasketStatusBar *bar)
    : QSplitter(Qt::Horizontal, parent)
    , m_actLockBasket(nullptr)
    , m_actPassBasket(nullptr)
    , m_loading(true)
    , m_newBasketPopup(false)
    , m_firstShow(true)
    , m_colorPicker(new DesktopColorPicker())
    , m_regionGrabber(nullptr)
    , m_passiveDroppedSelection(nullptr)
    , m_actionCollection(actionCollection)
    , m_guiClient(aGUIClient)
    , m_statusbar(bar)
    , m_tryHideTimer(nullptr)
    , m_hideTimer(nullptr)
{
    // new BNPViewAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/BNPView", this);

    setObjectName(name);

    /* Settings */
    Settings::loadConfig();

    Global::bnpView = this;

    // Needed when loading the baskets:
    Global::backgroundManager = new BackgroundManager();

    setupGlobalShortcuts();
    m_history = new QUndoStack(this);
    initialize();
    QTimer::singleShot(0, this, SLOT(lateInit()));
}

BNPView::~BNPView()
{
    int treeWidth = Global::bnpView->sizes()[Settings::treeOnLeft() ? 0 : 1];

    Settings::setBasketTreeWidth(treeWidth);

    if (currentBasket() && currentBasket()->isDuringEdit())
        currentBasket()->closeEditor();

    Settings::saveConfig();

    Global::bnpView = nullptr;

    delete Global::systemTray;
    Global::systemTray = nullptr;
    delete m_statusbar;
    delete m_history;
    m_history = nullptr;

    NoteDrag::createAndEmptyCuttingTmpFolder(); // Clean the temporary folder we used
}

void BNPView::lateInit()
{
    /*
        InlineEditors* instance = InlineEditors::instance();

        if(instance)
        {
            KToolBar* toolbar = instance->richTextToolBar();

            if(toolbar)
                toolbar->hide();
        }
    */

    // If the main window is hidden when session is saved, Container::queryClose()
    //  isn't called and the last value would be kept
    Settings::setStartDocked(true);
    Settings::saveConfig();

    /* System tray icon */
    Global::systemTray = new SystemTray(Global::activeMainWindow());
    Global::systemTray->setIconByName(":/images/22-apps-basket");
    connect(Global::systemTray, &SystemTray::showPart, this, &BNPView::showPart);
    /*if (Settings::useSystray())
        Global::systemTray->show();*/

    // Load baskets
    DEBUG_WIN << "Baskets are loaded from " + Global::basketsFolder();

    NoteDrag::createAndEmptyCuttingTmpFolder(); // If last exec hasn't done it: clean the temporary folder we will use
    Tag::loadTags();                            // Tags should be ready before loading baskets, but tags need the mainContainer to be ready to create KActions!
    load();

    // If no basket has been found, try to import from an older version,
    if (topLevelItemCount() <= 0) {
        QDir dir;
        dir.mkdir(Global::basketsFolder());
        if (FormatImporter::shouldImportBaskets()) {
            FormatImporter::importBaskets();
            load();
        }
        if (topLevelItemCount() <= 0) {
            // Create first basket:
            BasketFactory::newBasket(QString(), i18n("General"));
            GitWrapper::commitBasket(currentBasket());
            GitWrapper::commitTagsXml();
        }
    }

    // Load the Welcome Baskets if it is the First Time:
    if (!Settings::welcomeBasketsAdded()) {
        addWelcomeBaskets();
        Settings::setWelcomeBasketsAdded(true);
        Settings::saveConfig();
    }

    m_tryHideTimer = new QTimer(this);
    m_hideTimer = new QTimer(this);
    connect(m_tryHideTimer, &QTimer::timeout, this, &BNPView::timeoutTryHide);
    connect(m_hideTimer, &QTimer::timeout, this, &BNPView::timeoutHide);

    // Preload every baskets for instant filtering:
    /*StopWatch::start(100);
        QListViewItemIterator it(m_tree);
        while (it.current()) {
            BasketListViewItem *item = ((BasketListViewItem*)it.current());
            item->basket()->load();
            qApp->processEvents();
            ++it;
        }
    StopWatch::check(100);*/
}

void BNPView::addWelcomeBaskets()
{
    // Possible paths where to find the welcome basket archive, trying the translated one, and falling back to the English one:
    QStringList possiblePaths;
    if (QString(Tools::systemCodeset()) == QString("UTF-8")) { // Welcome baskets are encoded in UTF-8. If the system is not, then use the English version:
        QString lang = QLocale().languageToString(QLocale().language());
        possiblePaths.append(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "basket/welcome/Welcome_" + lang + ".baskets"));
        possiblePaths.append(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "basket/welcome/Welcome_" + lang.split('_')[0] + ".baskets"));
    }
    possiblePaths.append(QStandardPaths::locate(QStandardPaths::GenericDataLocation, "basket/welcome/Welcome_en_US.baskets"));

    // Take the first EXISTING basket archive found:
    QDir dir;
    QString path;
    for (QStringList::Iterator it = possiblePaths.begin(); it != possiblePaths.end(); ++it) {
        if (dir.exists(*it)) {
            path = *it;
            break;
        }
    }

    // Extract:
    if (!path.isEmpty())
        Archive::open(path);
}

void BNPView::onFirstShow()
{
    // In late init, because we need qApp->mainWidget() to be set!
    connectTagsMenu();

    m_statusbar->setupStatusBar();

    int treeWidth = Settings::basketTreeWidth();
    if (treeWidth < 0)
        treeWidth = m_tree->fontMetrics().maxWidth() * 11;
    QList<int> splitterSizes;
    splitterSizes.append(treeWidth);
    setSizes(splitterSizes);
}

void BNPView::setupGlobalShortcuts()
{
    KActionCollection *ac = new KActionCollection(this);
    QAction *a = nullptr;

    // Ctrl+Shift+W only works when started standalone:
    QWidget *basketMainWindow = qobject_cast<KMainWindow *>(Global::bnpView->parent());

    int modifier = Qt::CTRL + Qt::ALT + Qt::SHIFT;

    if (basketMainWindow) {
        a = ac->addAction("global_show_hide_main_window", Global::systemTray, SLOT(toggleActive()));
        a->setText(i18n("Show/hide main window"));
        a->setStatusTip(
            i18n("Allows you to show main Window if it is hidden, and to hide "
                 "it if it is shown."));
        KGlobalAccel::self()->setGlobalShortcut(a, (QKeySequence(modifier + Qt::Key_W)));
    }

    a = ac->addAction("global_paste", Global::bnpView, SLOT(globalPasteInCurrentBasket()));
    a->setText(i18n("Paste clipboard contents in current basket"));
    a->setStatusTip(
        i18n("Allows you to paste clipboard contents in the current basket "
             "without having to open the main window."));
    KGlobalAccel::self()->setGlobalShortcut(a, QKeySequence(modifier + Qt::Key_V));

    a = ac->addAction("global_show_current_basket", Global::bnpView, SLOT(showPassiveContentForced()));
    a->setText(i18n("Show current basket name"));
    a->setStatusTip(
        i18n("Allows you to know basket is current without opening "
             "the main window."));

    a = ac->addAction("global_paste_selection", Global::bnpView, SLOT(pasteSelInCurrentBasket()));
    a->setText(i18n("Paste selection in current basket"));
    a->setStatusTip(
        i18n("Allows you to paste clipboard selection in the current basket "
             "without having to open the main window."));
    KGlobalAccel::self()->setGlobalShortcut(a, (QKeySequence(Qt::CTRL + Qt::ALT + Qt::SHIFT + Qt::Key_S)));

    a = ac->addAction("global_new_basket", Global::bnpView, SLOT(askNewBasket()));
    a->setText(i18n("Create a new basket"));
    a->setStatusTip(
        i18n("Allows you to create a new basket without having to open the "
             "main window (you then can use the other global shortcuts to add "
             "a note, paste clipboard or paste selection in this new basket)."));

    a = ac->addAction("global_previous_basket", Global::bnpView, SLOT(goToPreviousBasket()));
    a->setText(i18n("Go to previous basket"));
    a->setStatusTip(
        i18n("Allows you to change current basket to the previous one without "
             "having to open the main window."));

    a = ac->addAction("global_next_basket", Global::bnpView, SLOT(goToNextBasket()));
    a->setText(i18n("Go to next basket"));
    a->setStatusTip(
        i18n("Allows you to change current basket to the next one "
             "without having to open the main window."));

    a = ac->addAction("global_note_add_html", Global::bnpView, SLOT(addNoteHtml()));
    a->setText(i18n("Insert text note"));
    a->setStatusTip(
        i18n("Add a text note to the current basket without having to open "
             "the main window."));
    KGlobalAccel::self()->setGlobalShortcut(a, (QKeySequence(modifier + Qt::Key_T)));

    a = ac->addAction("global_note_add_image", Global::bnpView, SLOT(addNoteImage()));
    a->setText(i18n("Insert image note"));
    a->setStatusTip(
        i18n("Add an image note to the current basket without having to open "
             "the main window."));

    a = ac->addAction("global_note_add_link", Global::bnpView, SLOT(addNoteLink()));
    a->setText(i18n("Insert link note"));
    a->setStatusTip(
        i18n("Add a link note to the current basket without having "
             "to open the main window."));

    a = ac->addAction("global_note_add_color", Global::bnpView, SLOT(addNoteColor()));
    a->setText(i18n("Insert color note"));
    a->setStatusTip(
        i18n("Add a color note to the current basket without having to open "
             "the main window."));

    a = ac->addAction("global_note_pick_color", Global::bnpView, SLOT(slotColorFromScreenGlobal()));
    a->setText(i18n("Pick color from screen"));
    a->setStatusTip(
        i18n("Add a color note picked from one pixel on screen to the current "
             "basket without "
             "having to open the main window."));

    a = ac->addAction("global_note_grab_screenshot", Global::bnpView, SLOT(grabScreenshotGlobal()));
    a->setText(i18n("Grab screen zone"));
    a->setStatusTip(
        i18n("Grab a screen zone as an image in the current basket without "
             "having to open the main window."));

#if 0
    a = ac->addAction("global_note_add_text", Global::bnpView,
                      SLOT(addNoteText()));
    a->setText(i18n("Insert plain text note"));
    a->setStatusTip(
        i18n("Add a plain text note to the current basket without having to "
             "open the main window."));
#endif
}

void BNPView::initialize()
{
    /// Configure the List View Columns:
    m_tree = new BasketTreeListView(this);
    m_tree->setHeaderLabel(i18n("Baskets"));
    m_tree->setSortingEnabled(false /*Disabled*/);
    m_tree->setRootIsDecorated(true);
    m_tree->setLineWidth(1);
    m_tree->setMidLineWidth(0);
    m_tree->setFocusPolicy(Qt::NoFocus);

    /// Configure the List View Drag and Drop:
    m_tree->setDragEnabled(true);
    m_tree->setDragDropMode(QAbstractItemView::DragDrop);
    m_tree->setAcceptDrops(true);
    m_tree->viewport()->setAcceptDrops(true);

    /// Configure the Splitter:
    m_stack = new QStackedWidget(this);

    setOpaqueResize(true);

    setCollapsible(indexOf(m_tree), true);
    setCollapsible(indexOf(m_stack), false);
    setStretchFactor(indexOf(m_tree), 0);
    setStretchFactor(indexOf(m_stack), 1);

    /// Configure the List View Signals:
    connect(m_tree, &BasketTreeListView::itemActivated, this, &BNPView::slotPressed);
    connect(m_tree, &BasketTreeListView::itemPressed, this, &BNPView::slotPressed);
    connect(m_tree, &BasketTreeListView::itemClicked, this, &BNPView::slotPressed);

    connect(m_tree, &BasketTreeListView::itemExpanded, this, &BNPView::needSave);
    connect(m_tree, &BasketTreeListView::itemCollapsed, this, &BNPView::needSave);
    connect(m_tree, &BasketTreeListView::contextMenuRequested, this, &BNPView::slotContextMenu);
    connect(m_tree, &BasketTreeListView::itemDoubleClicked, this, &BNPView::slotShowProperties);

    connect(m_tree, &BasketTreeListView::itemExpanded, this, &BNPView::basketChanged);
    connect(m_tree, &BasketTreeListView::itemCollapsed, this, &BNPView::basketChanged);

    connect(this, &BNPView::basketChanged, this, &BNPView::slotBasketChanged);

    connect(m_history, &QUndoStack::canRedoChanged, this, &BNPView::canUndoRedoChanged);
    connect(m_history, &QUndoStack::canUndoChanged, this, &BNPView::canUndoRedoChanged);

    setupActions();

    /// What's This Help for the tree:
    m_tree->setWhatsThis(
        i18n("<h2>Basket Tree</h2>"
             "Here is the list of your baskets. "
             "You can organize your data by putting them in different baskets. "
             "You can group baskets by subject by creating new baskets inside others. "
             "You can browse between them by clicking a basket to open it, or reorganize them using drag and drop."));

    setTreePlacement(Settings::treeOnLeft());
}

void BNPView::setupActions()
{
    QAction *a = nullptr;
    KActionCollection *ac = actionCollection();

    a = ac->addAction("basket_export_basket_archive", this, SLOT(saveAsArchive()));
    a->setText(i18n("&Basket Archive..."));
    a->setIcon(QIcon::fromTheme("baskets"));
    a->setShortcut(0);
    m_actSaveAsArchive = a;

    a = ac->addAction("basket_import_basket_archive", this, SLOT(openArchive()));
    a->setText(i18n("&Basket Archive..."));
    a->setIcon(QIcon::fromTheme("baskets"));
    a->setShortcut(0);
    m_actOpenArchive = a;

    a = ac->addAction("window_hide", this, SLOT(hideOnEscape()));
    a->setText(i18n("&Hide Window"));
    m_actionCollection->setDefaultShortcut(a, KStandardShortcut::Close);
    m_actHideWindow = a;

    m_actHideWindow->setEnabled(Settings::useSystray()); // Init here !

    a = ac->addAction("basket_export_html", this, SLOT(exportToHTML()));
    a->setText(i18n("&HTML Web Page..."));
    a->setIcon(QIcon::fromTheme("text-html"));
    a->setShortcut(0);
    m_actExportToHtml = a;

    a = ac->addAction("basket_import_text_file", this, &BNPView::importTextFile);
    a->setText(i18n("Text &File..."));
    a->setIcon(QIcon::fromTheme("text-plain"));
    a->setShortcut(0);

    a = ac->addAction("basket_backup_restore", this, SLOT(backupRestore()));
    a->setText(i18n("&Backup && Restore..."));
    a->setShortcut(0);

    a = ac->addAction("check_cleanup", this, SLOT(checkCleanup()));
    a->setText(i18n("&Check && Cleanup..."));
    a->setShortcut(0);
    if (Global::commandLineOpts->isSet("debug")) {
        a->setEnabled(true);
    } else {
        a->setEnabled(false);
    }

    /** Note : ****************************************************************/

    a = ac->addAction("edit_delete", this, SLOT(delNote()));
    a->setText(i18n("D&elete"));
    a->setIcon(QIcon::fromTheme("edit-delete"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Delete"));
    m_actDelNote = a;

    m_actCutNote = ac->addAction(KStandardAction::Cut, this, SLOT(cutNote()));
    m_actCopyNote = ac->addAction(KStandardAction::Copy, this, SLOT(copyNote()));

    m_actSelectAll = ac->addAction(KStandardAction::SelectAll, this, SLOT(slotSelectAll()));
    m_actSelectAll->setStatusTip(i18n("Selects all notes"));

    a = ac->addAction("edit_unselect_all", this, SLOT(slotUnselectAll()));
    a->setText(i18n("U&nselect All"));
    m_actUnselectAll = a;
    m_actUnselectAll->setStatusTip(i18n("Unselects all selected notes"));

    a = ac->addAction("edit_invert_selection", this, SLOT(slotInvertSelection()));
    a->setText(i18n("&Invert Selection"));
    m_actionCollection->setDefaultShortcut(a, Qt::CTRL + Qt::Key_Asterisk);
    m_actInvertSelection = a;

    m_actInvertSelection->setStatusTip(i18n("Inverts the current selection of notes"));

    a = ac->addAction("note_edit", this, SLOT(editNote()));
    a->setText(i18nc("Verb; not Menu", "&Edit..."));
    // a->setIcon(QIcon::fromTheme("edit"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Return"));
    m_actEditNote = a;

    m_actOpenNote = ac->addAction(KStandardAction::Open, "note_open", this, SLOT(openNote()));
    m_actOpenNote->setIcon(QIcon::fromTheme("window-new"));
    m_actOpenNote->setText(i18n("&Open"));
    m_actionCollection->setDefaultShortcut(m_actOpenNote, QKeySequence("F9"));

    a = ac->addAction("note_open_with", this, SLOT(openNoteWith()));
    a->setText(i18n("Open &With..."));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Shift+F9"));
    m_actOpenNoteWith = a;

    m_actSaveNoteAs = ac->addAction(KStandardAction::SaveAs, "note_save_to_file", this, SLOT(saveNoteAs()));
    m_actSaveNoteAs->setText(i18n("&Save to File..."));
    m_actionCollection->setDefaultShortcut(m_actSaveNoteAs, QKeySequence("F10"));

    a = ac->addAction("note_group", this, SLOT(noteGroup()));
    a->setText(i18n("&Group"));
    a->setIcon(QIcon::fromTheme("mail-attachment"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+G"));
    m_actGroup = a;

    a = ac->addAction("note_ungroup", this, SLOT(noteUngroup()));
    a->setText(i18n("U&ngroup"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+G"));
    m_actUngroup = a;

    a = ac->addAction("note_move_top", this, SLOT(moveOnTop()));
    a->setText(i18n("Move on &Top"));
    a->setIcon(QIcon::fromTheme("arrow-up-double"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+Home"));
    m_actMoveOnTop = a;

    a = ac->addAction("note_move_up", this, SLOT(moveNoteUp()));
    a->setText(i18n("Move &Up"));
    a->setIcon(QIcon::fromTheme("arrow-up"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+Up"));
    m_actMoveNoteUp = a;

    a = ac->addAction("note_move_down", this, SLOT(moveNoteDown()));
    a->setText(i18n("Move &Down"));
    a->setIcon(QIcon::fromTheme("arrow-down"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+Down"));
    m_actMoveNoteDown = a;

    a = ac->addAction("note_move_bottom", this, SLOT(moveOnBottom()));
    a->setText(i18n("Move on &Bottom"));
    a->setIcon(QIcon::fromTheme("arrow-down-double"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+End"));
    m_actMoveOnBottom = a;

    m_actPaste = ac->addAction(KStandardAction::Paste, this, SLOT(pasteInCurrentBasket()));

    /** Insert : **************************************************************/

#if 0
    a = ac->addAction("insert_text");
    a->setText(i18n("Plai&n Text"));
    a->setIcon(QIcon::fromTheme("text"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+T"));
    m_actInsertText = a;
#endif

    a = ac->addAction("insert_html");
    a->setText(i18n("&Text"));
    a->setIcon(QIcon::fromTheme("text-html"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Insert"));
    m_actInsertHtml = a;

    a = ac->addAction("insert_link");
    a->setText(i18n("&Link"));
    a->setIcon(QIcon::fromTheme(IconNames::LINK));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Y"));
    m_actInsertLink = a;

    a = ac->addAction("insert_cross_reference");
    a->setText(i18n("Cross &Reference"));
    a->setIcon(QIcon::fromTheme(IconNames::CROSS_REF));
    m_actInsertCrossReference = a;

    a = ac->addAction("insert_image");
    a->setText(i18n("&Image"));
    a->setIcon(QIcon::fromTheme(IconNames::IMAGE));
    m_actInsertImage = a;

    a = ac->addAction("insert_color");
    a->setText(i18n("&Color"));
    a->setIcon(QIcon::fromTheme(IconNames::COLOR));
    m_actInsertColor = a;

    a = ac->addAction("insert_launcher");
    a->setText(i18n("L&auncher"));
    a->setIcon(QIcon::fromTheme(IconNames::LAUNCH));
    m_actInsertLauncher = a;

    a = ac->addAction("insert_kmenu");
    a->setText(i18n("Import Launcher for &desktop application..."));
    a->setIcon(QIcon::fromTheme(IconNames::KMENU));
    m_actImportKMenu = a;

    a = ac->addAction("insert_icon");
    a->setText(i18n("Im&port Icon..."));
    a->setIcon(QIcon::fromTheme(IconNames::ICONS));
    m_actImportIcon = a;

    a = ac->addAction("insert_from_file");
    a->setText(i18n("Load From &File..."));
    a->setIcon(QIcon::fromTheme(IconNames::DOCUMENT_IMPORT));
    m_actLoadFile = a;

    //  connect( m_actInsertText, QAction::triggered, this, [this] () { insertEmpty(NoteType::Text); });
    connect(m_actInsertHtml, &QAction::triggered, this, [this] () { insertEmpty(NoteType::Html); });
    connect(m_actInsertImage, &QAction::triggered, this, [this] () { insertEmpty(NoteType::Image); });
    connect(m_actInsertLink, &QAction::triggered, this, [this] () { insertEmpty(NoteType::Link); });
    connect(m_actInsertCrossReference, &QAction::triggered, this, [this] () { insertEmpty(NoteType::CrossReference); });
    connect(m_actInsertColor, &QAction::triggered, this, [this] () { insertEmpty(NoteType::Color); });
    connect(m_actInsertLauncher, &QAction::triggered, this, [this] () { insertEmpty(NoteType::Launcher); });

    connect(m_actImportKMenu, &QAction::triggered, this, [this] () { insertWizard(1); });
    connect(m_actImportIcon, &QAction::triggered, this, [this] () { insertWizard(2); });
    connect(m_actLoadFile, &QAction::triggered, this, [this] () { insertWizard(3); });

    a = ac->addAction("insert_screen_color", this, &BNPView::slotColorFromScreen);
    a->setText(i18n("C&olor from Screen"));
    a->setIcon(QIcon::fromTheme("kcolorchooser"));
    m_actColorPicker = a;

    connect(m_colorPicker.get(), &DesktopColorPicker::pickedColor, this, &BNPView::colorPicked);

    a = ac->addAction("insert_screen_capture", this, SLOT(grabScreenshot()));
    a->setText(i18n("Grab Screen &Zone"));
    a->setIcon(QIcon::fromTheme("ksnapshot"));
    m_actGrabScreenshot = a;

    //connect(m_actGrabScreenshot, SIGNAL(regionGrabbed(const QPixmap&)), this, SLOT(screenshotGrabbed(const QPixmap&)));
    //connect(m_colorPicker, SIGNAL(canceledPick()), this, SLOT(colorPickingCanceled()));

    //  m_insertActions.append( m_actInsertText     );
    m_insertActions.append(m_actInsertHtml);
    m_insertActions.append(m_actInsertLink);
    m_insertActions.append(m_actInsertCrossReference);
    m_insertActions.append(m_actInsertImage);
    m_insertActions.append(m_actInsertColor);
    m_insertActions.append(m_actImportKMenu);
    m_insertActions.append(m_actInsertLauncher);
    m_insertActions.append(m_actImportIcon);
    m_insertActions.append(m_actLoadFile);
    m_insertActions.append(m_actColorPicker);
    m_insertActions.append(m_actGrabScreenshot);

    /** Basket : **************************************************************/

    // At this stage, main.cpp has not set qApp->mainWidget(), so Global::runInsideKontact()
    // returns true. We do it ourself:
    bool runInsideKontact = true;
    QWidget *parentWidget = (QWidget *)parent();
    while (parentWidget) {
        if (parentWidget->inherits("MainWindow"))
            runInsideKontact = false;
        parentWidget = (QWidget *)parentWidget->parent();
    }

    // Use the "basket" icon in Kontact so it is consistent with the Kontact "New..." icon

    a = ac->addAction("basket_new", this, SLOT(askNewBasket()));
    a->setText(i18n("&New Basket..."));
    a->setIcon(QIcon::fromTheme((runInsideKontact ? "basket" : "document-new")));
    m_actionCollection->setDefaultShortcuts(a, KStandardShortcut::shortcut(KStandardShortcut::New));
    actNewBasket = a;

    a = ac->addAction("basket_new_sub", this, SLOT(askNewSubBasket()));
    a->setText(i18n("New &Sub-Basket..."));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+Shift+N"));
    actNewSubBasket = a;

    a = ac->addAction("basket_new_sibling", this, SLOT(askNewSiblingBasket()));
    a->setText(i18n("New Si&bling Basket..."));
    actNewSiblingBasket = a;

    KActionMenu *newBasketMenu = new KActionMenu(i18n("&New"), ac);
    newBasketMenu->setIcon(QIcon::fromTheme("document-new"));
    ac->addAction("basket_new_menu", newBasketMenu);

    newBasketMenu->addAction(actNewBasket);
    newBasketMenu->addAction(actNewSubBasket);
    newBasketMenu->addAction(actNewSiblingBasket);
    connect(newBasketMenu, SIGNAL(triggered()), this, SLOT(askNewBasket()));

    a = ac->addAction("basket_properties", this, SLOT(propBasket()));
    a->setText(i18n("&Properties..."));
    a->setIcon(QIcon::fromTheme("document-properties"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("F2"));
    m_actPropBasket = a;

    a = ac->addAction("basket_sort_children_asc", this, SLOT(sortChildrenAsc()));
    a->setText(i18n("Sort Children Ascending"));
    a->setIcon(QIcon::fromTheme("view-sort-ascending"));
    m_actSortChildrenAsc = a;

    a = ac->addAction("basket_sort_children_desc", this, SLOT(sortChildrenDesc()));
    a->setText(i18n("Sort Children Descending"));
    a->setIcon(QIcon::fromTheme("view-sort-descending"));
    m_actSortChildrenDesc = a;

    a = ac->addAction("basket_sort_siblings_asc", this, SLOT(sortSiblingsAsc()));
    a->setText(i18n("Sort Siblings Ascending"));
    a->setIcon(QIcon::fromTheme("view-sort-ascending"));
    m_actSortSiblingsAsc = a;

    a = ac->addAction("basket_sort_siblings_desc", this, SLOT(sortSiblingsDesc()));
    a->setText(i18n("Sort Siblings Descending"));
    a->setIcon(QIcon::fromTheme("view-sort-descending"));
    m_actSortSiblingsDesc = a;

    a = ac->addAction("basket_remove", this, SLOT(delBasket()));
    a->setText(i18nc("Remove Basket", "&Remove"));
    a->setShortcut(0);
    m_actDelBasket = a;

#ifdef HAVE_LIBGPGME
    a = ac->addAction("basket_password", this, SLOT(password()));
    a->setText(i18nc("Password protection", "Pass&word..."));
    a->setShortcut(0);
    m_actPassBasket = a;

    a = ac->addAction("basket_lock", this, SLOT(lockBasket()));
    a->setText(i18nc("Lock Basket", "&Lock"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+L"));
    m_actLockBasket = a;
#endif

    /** Edit : ****************************************************************/

    // m_actUndo     = KStandardAction::undo(  this, SLOT(undo()),                 actionCollection() );
    // m_actUndo->setEnabled(false); // Not yet implemented !
    // m_actRedo     = KStandardAction::redo(  this, SLOT(redo()),                 actionCollection() );
    // m_actRedo->setEnabled(false); // Not yet implemented !

    KToggleAction *toggleAct = nullptr;
    toggleAct = new KToggleAction(i18n("&Filter"), ac);
    ac->addAction("edit_filter", toggleAct);
    toggleAct->setIcon(QIcon::fromTheme("view-filter"));
    m_actionCollection->setDefaultShortcuts(toggleAct, KStandardShortcut::shortcut(KStandardShortcut::Find));
    m_actShowFilter = toggleAct;

    connect(m_actShowFilter, SIGNAL(toggled(bool)), this, SLOT(showHideFilterBar(bool)));

    toggleAct = new KToggleAction(ac);
    ac->addAction("edit_filter_all_baskets", toggleAct);
    toggleAct->setText(i18n("&Search All"));
    toggleAct->setIcon(QIcon::fromTheme("edit-find"));
    m_actionCollection->setDefaultShortcut(toggleAct, QKeySequence("Ctrl+Shift+F"));
    m_actFilterAllBaskets = toggleAct;

    connect(m_actFilterAllBaskets, &KToggleAction::toggled, this, &BNPView::toggleFilterAllBaskets);

    a = ac->addAction("edit_filter_reset", this, SLOT(slotResetFilter()));
    a->setText(i18n("&Reset Filter"));
    a->setIcon(QIcon::fromTheme("edit-clear-locationbar-rtl"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+R"));
    m_actResetFilter = a;

    /** Go : ******************************************************************/

    a = ac->addAction("go_basket_previous", this, SLOT(goToPreviousBasket()));
    a->setText(i18n("&Previous Basket"));
    a->setIcon(QIcon::fromTheme("go-previous"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Alt+Left"));
    m_actPreviousBasket = a;

    a = ac->addAction("go_basket_next", this, SLOT(goToNextBasket()));
    a->setText(i18n("&Next Basket"));
    a->setIcon(QIcon::fromTheme("go-next"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Alt+Right"));
    m_actNextBasket = a;

    a = ac->addAction("go_basket_fold", this, SLOT(foldBasket()));
    a->setText(i18n("&Fold Basket"));
    a->setIcon(QIcon::fromTheme("go-up"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Alt+Up"));
    m_actFoldBasket = a;

    a = ac->addAction("go_basket_expand", this, SLOT(expandBasket()));
    a->setText(i18n("&Expand Basket"));
    a->setIcon(QIcon::fromTheme("go-down"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Alt+Down"));
    m_actExpandBasket = a;

#if 0
    // FOR_BETA_PURPOSE:
    a = ac->addAction("beta_convert_texts", this, SLOT(convertTexts()));
    a->setText(i18n("Convert text notes to rich text notes"));
    a->setIcon(QIcon::fromTheme("run-build-file"));
    m_convertTexts = a;
#endif

    InlineEditors::instance()->initToolBars(actionCollection());
    /** Help : ****************************************************************/

    a = ac->addAction("help_welcome_baskets", this, SLOT(addWelcomeBaskets()));
    a->setText(i18n("&Welcome Baskets"));
}

BasketListViewItem *BNPView::topLevelItem(int i)
{
    return (BasketListViewItem *)m_tree->topLevelItem(i);
}

void BNPView::slotShowProperties(QTreeWidgetItem *item)
{
    if (item)
        propBasket();
}

void BNPView::slotContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item;
    item = m_tree->itemAt(pos);
    QString menuName;
    if (item) {
        BasketScene *basket = ((BasketListViewItem *)item)->basket();

        setCurrentBasket(basket);
        menuName = "basket_popup";
    } else {
        menuName = "tab_bar_popup";
        /*
         * "File -> New" create a new basket with the same parent basket as the current one.
         * But when invoked when right-clicking the empty area at the bottom of the basket tree,
         * it is obvious the user want to create a new basket at the bottom of the tree (with no parent).
         * So we set a temporary variable during the time the popup menu is shown,
         * so the slot askNewBasket() will do the right thing:
         */
        setNewBasketPopup();
    }

    QMenu *menu = popupMenu(menuName);
    connect(menu, &QMenu::aboutToHide, this, &BNPView::aboutToHideNewBasketPopup);
    menu->exec(m_tree->mapToGlobal(pos));
}

/* this happens every time we switch the basket (but not if we tell the user we save the stuff
 */
void BNPView::save()
{
    DEBUG_WIN << "Basket Tree: Saving...";

    QString data;
    QXmlStreamWriter stream(&data);
    XMLWork::setupXmlStream(stream, "basketTree");

    // Save Basket Tree:
    save(m_tree, nullptr, stream);

    stream.writeEndElement();
    stream.writeEndDocument();

    // Write to Disk:
    FileStorage::safelySaveToFile(Global::basketsFolder() + "baskets.xml", data);

    GitWrapper::commitBasketView();
}

void BNPView::save(QTreeWidget *listView, QTreeWidgetItem *item, QXmlStreamWriter &stream)
{
    if (item == nullptr) {
        if (listView == nullptr) {
            // This should not happen: we call either save(listView, 0) or save(0, item)
            DEBUG_WIN << "BNPView::save error: listView=NULL and item=NULL";
            return;
        }

        // For each basket:
        for (int i = 0; i < listView->topLevelItemCount(); i++) {
            item = listView->topLevelItem(i);
            save(nullptr, item, stream);
        }
    } else {
        saveSubHierarchy(item, stream, true);
    }
}

void BNPView::writeBasketElement(QTreeWidgetItem *item, QXmlStreamWriter &stream)
{
    BasketScene *basket = ((BasketListViewItem *)item)->basket();

    // Save Attributes:
    stream.writeAttribute("folderName", basket->folderName());
    if (item->childCount() >= 0) // If it can be expanded/folded:
        stream.writeAttribute("folded", XMLWork::trueOrFalse(!item->isExpanded()));

    if (((BasketListViewItem *)item)->isCurrentBasket())
        stream.writeAttribute("lastOpened", "true");

    basket->saveProperties(stream);
}

void BNPView::saveSubHierarchy(QTreeWidgetItem *item, QXmlStreamWriter &stream, bool recursive)
{
    stream.writeStartElement("basket");
    writeBasketElement(item, stream); // create root <basket>
    if (recursive) {
        for (int i = 0; i < item->childCount(); i++) {
            saveSubHierarchy(item->child(i), stream, true);
        }
    }
    stream.writeEndElement();
}

void BNPView::load()
{
    QScopedPointer<QDomDocument> doc(XMLWork::openFile("basketTree", Global::basketsFolder() + "baskets.xml"));
    // BEGIN Compatibility with 0.6.0 Pre-Alpha versions:
    if (!doc)
        doc.reset(XMLWork::openFile("basketsTree", Global::basketsFolder() + "baskets.xml"));
    // END
    if (doc != nullptr) {
        QDomElement docElem = doc->documentElement();
        load(nullptr, docElem);
    }
    m_loading = false;
}

void BNPView::load(QTreeWidgetItem *item, const QDomElement &baskets)
{
    QDomNode n = baskets.firstChild();
    while (!n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                BasketScene *basket = loadBasket(folderName);
                BasketListViewItem *basketItem = appendBasket(basket, item);
                basketItem->setExpanded(!XMLWork::trueOrFalse(element.attribute("folded", "false"), false));
                basket->loadProperties(XMLWork::getElement(element, "properties"));
                if (XMLWork::trueOrFalse(element.attribute("lastOpened", element.attribute("lastOpened", "false")), false)) // Compat with 0.6.0-Alphas
                    setCurrentBasket(basket);
                // Load Sub-baskets:
                load(basketItem, element);
            }
        }
        n = n.nextSibling();
    }
}

BasketScene *BNPView::loadBasket(const QString &folderName)
{
    if (folderName.isEmpty())
        return nullptr;

    DecoratedBasket *decoBasket = new DecoratedBasket(m_stack, folderName);
    BasketScene *basket = decoBasket->basket();
    m_stack->addWidget(decoBasket);

    connect(basket, &BasketScene::countsChanged, this, &BNPView::countsChanged);
    // Important: Create listViewItem and connect signal BEFORE loadProperties(), so we get the listViewItem updated without extra work:
    connect(basket, &BasketScene::propertiesChanged, this, &BNPView::updateBasketListViewItem);

    connect(basket->decoration()->filterBar(), &FilterBar::newFilter, this, &BNPView::newFilterFromFilterBar);
    connect(basket, &BasketScene::crossReference, this, &BNPView::loadCrossReference);

    return basket;
}

int BNPView::basketCount(QTreeWidgetItem *parent)
{
    int count = 1;
    if (parent == nullptr)
        return 0;

    for (int i = 0; i < parent->childCount(); i++) {
        count += basketCount(parent->child(i));
    }

    return count;
}

bool BNPView::canFold()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (!item)
        return false;
    return (item->childCount() > 0 && item->isExpanded());
}

bool BNPView::canExpand()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (!item)
        return false;
    return (item->childCount() > 0 && !item->isExpanded());
}

BasketListViewItem *BNPView::appendBasket(BasketScene *basket, QTreeWidgetItem *parentItem)
{
    BasketListViewItem *newBasketItem;
    if (parentItem)
        newBasketItem = new BasketListViewItem(parentItem, parentItem->child(parentItem->childCount() - 1), basket);
    else {
        newBasketItem = new BasketListViewItem(m_tree, m_tree->topLevelItem(m_tree->topLevelItemCount() - 1), basket);
    }

    return newBasketItem;
}

void BNPView::loadNewBasket(const QString &folderName, const QDomElement &properties, BasketScene *parent)
{
    BasketScene *basket = loadBasket(folderName);
    appendBasket(basket, (basket ? listViewItemForBasket(parent) : nullptr));
    basket->loadProperties(properties);
    setCurrentBasketInHistory(basket);
    //  save();
}

int BNPView::topLevelItemCount()
{
    return m_tree->topLevelItemCount();
}

void BNPView::goToPreviousBasket()
{
    if (m_history->canUndo())
        m_history->undo();
}

void BNPView::goToNextBasket()
{
    if (m_history->canRedo())
        m_history->redo();
}

void BNPView::foldBasket()
{
    BasketListViewItem *item = listViewItemForBasket(currentBasket());
    if (item && item->childCount() <= 0)
        item->setExpanded(false); // If Alt+Left is hit and there is nothing to close, make sure the focus will go to the parent basket

    QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, nullptr, nullptr);
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::expandBasket()
{
    QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, nullptr, nullptr);
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::closeAllEditors()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = (BasketListViewItem *)(*it);
        item->basket()->closeEditor();
        ++it;
    }
}

bool BNPView::convertTexts()
{
    bool convertedNotes = false;
    QProgressDialog dialog;
    dialog.setWindowTitle(i18n("Plain Text Notes Conversion"));
    dialog.setLabelText(i18n("Converting plain text notes to rich text ones..."));
    dialog.setModal(true);
    dialog.setRange(0, basketCount());
    dialog.show(); // setMinimumDuration(50/*ms*/);

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = (BasketListViewItem *)(*it);
        if (item->basket()->convertTexts())
            convertedNotes = true;

        dialog.setValue(dialog.value() + 1);

        if (dialog.wasCanceled())
            break;
        ++it;
    }

    return convertedNotes;
}

void BNPView::toggleFilterAllBaskets(bool doFilter)
{
    // If the filter isn't already showing, we make sure it does.
    if (doFilter)
        m_actShowFilter->setChecked(true);

    // currentBasket()->decoration()->filterBar()->setFilterAll(doFilter);

    if (doFilter)
        currentBasket()->decoration()->filterBar()->setEditFocus();

    // Filter every baskets:
    newFilter();
}

/** This function can be called recursively because we call qApp->processEvents().
 * If this function is called whereas another "instance" is running,
 * this new "instance" leave and set up a flag that is read by the first "instance"
 * to know it should re-begin the work.
 * PS: Yes, that's a very lame pseudo-threading but that works, and it's programmer-efforts cheap :-)
 */
void BNPView::newFilter()
{
    static bool alreadyEntered = false;
    static bool shouldRestart = false;

    if (alreadyEntered) {
        shouldRestart = true;
        return;
    }
    alreadyEntered = true;
    shouldRestart = false;

    BasketScene *current = currentBasket();
    const FilterData &filterData = current->decoration()->filterBar()->filterData();

    // Set the filter data for every other baskets, or reset the filter for every other baskets if we just disabled the filterInAllBaskets:
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        if (item->basket() != current) {
            if (isFilteringAllBaskets())
                item->basket()->decoration()->filterBar()->setFilterData(filterData); // Set the new FilterData for every other baskets
            else
                item->basket()->decoration()->filterBar()->setFilterData(FilterData()); // We just disabled the global filtering: remove the FilterData
        }
        ++it;
    }

    // Show/hide the "little filter icons" (during basket load)
    // or the "little numbers" (to show number of found notes in the baskets) is the tree:
    qApp->processEvents();

    // Load every baskets for filtering, if they are not already loaded, and if necessary:
    if (filterData.isFiltering) {
        BasketScene *current = currentBasket();
        QTreeWidgetItemIterator it(m_tree);
        while (*it) {
            BasketListViewItem *item = ((BasketListViewItem *)*it);
            if (item->basket() != current) {
                BasketScene *basket = item->basket();
                if (!basket->loadingLaunched() && !basket->isLocked())
                    basket->load();
                basket->filterAgain();
                qApp->processEvents();
                if (shouldRestart) {
                    alreadyEntered = false;
                    shouldRestart = false;
                    newFilter();
                    return;
                }
            }
            ++it;
        }
    }

    //  qApp->processEvents();
    m_tree->viewport()->update(); // to see the "little numbers"

    alreadyEntered = false;
    shouldRestart = false;
}

void BNPView::newFilterFromFilterBar()
{
    if (isFilteringAllBaskets())
        QTimer::singleShot(0, this, SLOT(newFilter())); // Keep time for the QLineEdit to display the filtered character and refresh correctly!
}

bool BNPView::isFilteringAllBaskets()
{
    return m_actFilterAllBaskets->isChecked();
}

BasketListViewItem *BNPView::listViewItemForBasket(BasketScene *basket)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        if (item->basket() == basket)
            return item;
        ++it;
    }
    return nullptr;
}

BasketScene *BNPView::currentBasket()
{
    DecoratedBasket *decoBasket = (DecoratedBasket *)m_stack->currentWidget();
    if (decoBasket)
        return decoBasket->basket();
    else
        return nullptr;
}

BasketScene *BNPView::parentBasketOf(BasketScene *basket)
{
    BasketListViewItem *item = (BasketListViewItem *)(listViewItemForBasket(basket)->parent());
    if (item)
        return item->basket();
    else
        return nullptr;
}

void BNPView::setCurrentBasketInHistory(BasketScene *basket)
{
    if (!basket)
        return;

    if (currentBasket() == basket)
        return;

    m_history->push(new HistorySetBasket(basket));
}

void BNPView::setCurrentBasket(BasketScene *basket)
{
    if (currentBasket() == basket)
        return;

    if (currentBasket())
        currentBasket()->closeBasket();

    if (basket)
        basket->aboutToBeActivated();

    BasketListViewItem *item = listViewItemForBasket(basket);
    if (item) {
        m_tree->setCurrentItem(item);
        item->ensureVisible();
        m_stack->setCurrentWidget(basket->decoration());
        // If the window has changed size, only the current basket receive the event,
        // the others will receive ony one just before they are shown.
        // But this triggers unwanted animations, so we eliminate it:
        basket->relayoutNotes();
        basket->openBasket();
        setWindowTitle(item->basket()->basketName());
        countsChanged(basket);
        updateStatusBarHint();
        if (Global::systemTray)
            Global::systemTray->updateDisplay();
        m_tree->scrollToItem(m_tree->currentItem());
        item->basket()->setFocus();
    }
    m_tree->viewport()->update();
    Q_EMIT basketChanged();
}

void BNPView::removeBasket(BasketScene *basket)
{
    if (basket->isDuringEdit())
        basket->closeEditor();

    // Find a new basket to switch to and select it.
    // Strategy: get the next sibling, or the previous one if not found.
    // If there is no such one, get the parent basket:
    BasketListViewItem *basketItem = listViewItemForBasket(basket);
    BasketListViewItem *nextBasketItem = (BasketListViewItem *)(m_tree->itemBelow(basketItem));
    if (!nextBasketItem)
        nextBasketItem = (BasketListViewItem *)m_tree->itemAbove(basketItem);
    if (!nextBasketItem)
        nextBasketItem = (BasketListViewItem *)(basketItem->parent());

    if (nextBasketItem)
        setCurrentBasketInHistory(nextBasketItem->basket());

    // Remove from the view:
    basket->unsubscribeBackgroundImages();
    m_stack->removeWidget(basket->decoration());
    //  delete basket->decoration();
    delete basketItem;
    //  delete basket;

    // If there is no basket anymore, add a new one:
    if (!nextBasketItem) {
        BasketFactory::newBasket(QString(), i18n("General"));
    } else {    // No need to save two times if we add a basket
        save();
    }
}

void BNPView::setTreePlacement(bool onLeft)
{
    if (onLeft)
        insertWidget(0, m_tree);
    else
        addWidget(m_tree);
    // updateGeometry();
    qApp->postEvent(this, new QResizeEvent(size(), size()));
}

void BNPView::relayoutAllBaskets()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        // item->basket()->unbufferizeAll();
        item->basket()->unsetNotesWidth();
        item->basket()->relayoutNotes();
        ++it;
    }
}

void BNPView::recomputeAllStyles()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        item->basket()->recomputeAllStyles();
        item->basket()->unsetNotesWidth();
        item->basket()->relayoutNotes();
        ++it;
    }
}

void BNPView::removedStates(const QList<State *> &deletedStates)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        item->basket()->removedStates(deletedStates);
        ++it;
    }
}

void BNPView::linkLookChanged()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        item->basket()->linkLookChanged();
        ++it;
    }
}

void BNPView::filterPlacementChanged(bool onTop)
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = static_cast<BasketListViewItem *>(*it);
        DecoratedBasket *decoration = static_cast<DecoratedBasket *>(item->basket()->parent());
        decoration->setFilterBarPosition(onTop);
        ++it;
    }
}

void BNPView::updateBasketListViewItem(BasketScene *basket)
{
    BasketListViewItem *item = listViewItemForBasket(basket);
    if (item)
        item->setup();

    if (basket == currentBasket()) {
        setWindowTitle(basket->basketName());
        if (Global::systemTray)
            Global::systemTray->updateDisplay();
    }

    // Don't save if we are loading!
    if (!m_loading)
        save();
}

void BNPView::needSave(QTreeWidgetItem *)
{
    if (!m_loading)
        // A basket has been collapsed/expanded or a new one is select: this is not urgent:
        QTimer::singleShot(500 /*ms*/, this, SLOT(save()));
}

void BNPView::slotPressed(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    BasketScene *basket = currentBasket();
    if (basket == nullptr)
        return;

    // Impossible to Select no Basket:
    if (!item)
        m_tree->setCurrentItem(listViewItemForBasket(basket), true);

    else if (dynamic_cast<BasketListViewItem *>(item) != nullptr && currentBasket() != ((BasketListViewItem *)item)->basket()) {
        setCurrentBasketInHistory(((BasketListViewItem *)item)->basket());
        needSave(nullptr);
    }
    basket->graphicsView()->viewport()->setFocus();
}

DecoratedBasket *BNPView::currentDecoratedBasket()
{
    if (currentBasket())
        return currentBasket()->decoration();
    else
        return nullptr;
}

// Redirected actions :

void BNPView::exportToHTML()
{
    HTMLExporter exporter(currentBasket());
}
void BNPView::editNote()
{
    currentBasket()->noteEdit();
}
void BNPView::cutNote()
{
    currentBasket()->noteCut();
}
void BNPView::copyNote()
{
    currentBasket()->noteCopy();
}
void BNPView::delNote()
{
    currentBasket()->noteDelete();
}
void BNPView::openNote()
{
    currentBasket()->noteOpen();
}
void BNPView::openNoteWith()
{
    currentBasket()->noteOpenWith();
}
void BNPView::saveNoteAs()
{
    currentBasket()->noteSaveAs();
}
void BNPView::noteGroup()
{
    currentBasket()->noteGroup();
}
void BNPView::noteUngroup()
{
    currentBasket()->noteUngroup();
}
void BNPView::moveOnTop()
{
    currentBasket()->noteMoveOnTop();
}
void BNPView::moveOnBottom()
{
    currentBasket()->noteMoveOnBottom();
}
void BNPView::moveNoteUp()
{
    currentBasket()->noteMoveNoteUp();
}
void BNPView::moveNoteDown()
{
    currentBasket()->noteMoveNoteDown();
}
void BNPView::slotSelectAll()
{
    currentBasket()->selectAll();
}
void BNPView::slotUnselectAll()
{
    currentBasket()->unselectAll();
}
void BNPView::slotInvertSelection()
{
    currentBasket()->invertSelection();
}
void BNPView::slotResetFilter()
{
    currentDecoratedBasket()->resetFilter();
}

void BNPView::importTextFile()
{
    SoftwareImporters::importTextFile();
}

void BNPView::backupRestore()
{
    BackupDialog dialog;
    dialog.exec();
}

void checkNote(Note *note, QList<QString> &fileList)
{
    while (note) {
        note->finishLazyLoad();
        if (note->isGroup()) {
            checkNote(note->firstChild(), fileList);
        } else if (note->content()->useFile()) {
            QString noteFileName = note->basket()->folderName() + note->content()->fileName();
            int basketFileIndex = fileList.indexOf(noteFileName);
            if (basketFileIndex < 0) {
                DEBUG_WIN << "<font color='red'>" + noteFileName + " NOT FOUND!</font>";
            } else {
                fileList.removeAt(basketFileIndex);
            }
        }
        note = note->next();
    }
}

void checkBasket(BasketListViewItem *item, QList<QString> &dirList, QList<QString> &fileList)
{
    BasketScene *basket = ((BasketListViewItem *)item)->basket();
    QString basketFolderName = basket->folderName();
    int basketFolderIndex = dirList.indexOf(basket->folderName());
    if (basketFolderIndex < 0) {
        DEBUG_WIN << "<font color='red'>" + basketFolderName + " NOT FOUND!</font>";
    } else {
        dirList.removeAt(basketFolderIndex);
    }
    int basketFileIndex = fileList.indexOf(basket->folderName() + ".basket");
    if (basketFileIndex < 0) {
        DEBUG_WIN << "<font color='red'>.basket file of " + basketFolderName + ".basket NOT FOUND!</font>";
    } else {
        fileList.removeAt(basketFileIndex);
    }
    if (!basket->loadingLaunched() && !basket->isLocked()) {
        basket->load();
    }
    DEBUG_WIN << "\t********************************************************************************";
    DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") loaded.";
    Note *note = basket->firstNote();
    if (!note) {
        DEBUG_WIN << "\tHas NO notes!";
    } else {
        checkNote(note, fileList);
    }
    basket->save();
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
    for (int i = 0; i < item->childCount(); i++) {
        checkBasket((BasketListViewItem *)item->child(i), dirList, fileList);
    }
    if (basket != Global::bnpView->currentBasket()) {
        DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") unloading...";
        DEBUG_WIN << "\t********************************************************************************";
        basket->unbufferizeAll();
    } else {
        DEBUG_WIN << basket->basketName() << "(" << basketFolderName << ") is the current basket, not unloading.";
        DEBUG_WIN << "\t********************************************************************************";
    }
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
}

void BNPView::checkCleanup()
{
    DEBUG_WIN << "Starting the check, cleanup and reindexing... (" + Global::basketsFolder() + ')';
    QList<QString> dirList;
    QList<QString> fileList;
    QString topDirEntry;
    QString subDirEntry;
    QFileInfo fileInfo;
    QDir topDir(Global::basketsFolder(), QString(), QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
    Q_FOREACH (topDirEntry, topDir.entryList()) {
        if (topDirEntry != QLatin1String(".") && topDirEntry != QLatin1String("..")) {
            fileInfo.setFile(Global::basketsFolder() + '/' + topDirEntry);
            if (fileInfo.isDir()) {
                dirList << topDirEntry + '/';
                QDir basketDir(Global::basketsFolder() + '/' + topDirEntry, QString(), QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
                Q_FOREACH (subDirEntry, basketDir.entryList()) {
                    if (subDirEntry != "." && subDirEntry != "..") {
                        fileList << topDirEntry + '/' + subDirEntry;
                    }
                }
            } else if (topDirEntry != "." && topDirEntry != ".." && topDirEntry != "baskets.xml") {
                fileList << topDirEntry;
            }
        }
    }
    DEBUG_WIN << "Directories found: " + QString::number(dirList.count());
    DEBUG_WIN << "Files found: " + QString::number(fileList.count());

    DEBUG_WIN << "Checking Baskets:";
    for (int i = 0; i < topLevelItemCount(); i++) {
        checkBasket(topLevelItem(i), dirList, fileList);
    }
    DEBUG_WIN << "Baskets checked.";
    DEBUG_WIN << "Directories remaining (not in any basket): " + QString::number(dirList.count());
    DEBUG_WIN << "Files remaining (not in any basket): " + QString::number(fileList.count());

    Q_FOREACH (topDirEntry, dirList) {
        DEBUG_WIN << "<font color='red'>" + topDirEntry + " does not belong to any basket!</font>";
        // Tools::deleteRecursively(Global::basketsFolder() + '/' + topDirEntry);
        // DEBUG_WIN << "<font color='red'>\t" + topDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + "/" + topDirEntry);
        DEBUG_WIN << "<font color='red'>\t" + topDirEntry + " trashed!</font>";
        Q_FOREACH (subDirEntry, fileList) {
            fileInfo.setFile(Global::basketsFolder() + '/' + subDirEntry);
            if (!fileInfo.isFile()) {
                fileList.removeAll(subDirEntry);
                DEBUG_WIN << "<font color='red'>\t\t" + subDirEntry + " already removed!</font>";
            }
        }
    }
    Q_FOREACH (subDirEntry, fileList) {
        DEBUG_WIN << "<font color='red'>" + subDirEntry + " does not belong to any note!</font>";
        // Tools::deleteRecursively(Global::basketsFolder() + '/' + subDirEntry);
        // DEBUG_WIN << "<font color='red'>\t" + subDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + '/' + subDirEntry);
        DEBUG_WIN << "<font color='red'>\t" + subDirEntry + " trashed!</font>";
    }
    DEBUG_WIN << "Check, cleanup and reindexing completed";
}

void BNPView::countsChanged(BasketScene *basket)
{
    if (basket == currentBasket())
        notesStateChanged();
}

void BNPView::notesStateChanged()
{
    BasketScene *basket = currentBasket();

    // Update statusbar message :
    if (currentBasket()->isLocked())
        setSelectionStatus(i18n("Locked"));
    else if (!basket->isLoaded())
        setSelectionStatus(i18n("Loading..."));
    else if (basket->count() == 0)
        setSelectionStatus(i18n("No notes"));
    else {
        QString count = i18np("%1 note", "%1 notes", basket->count());
        QString selecteds = i18np("%1 selected", "%1 selected", basket->countSelecteds());
        QString showns = (currentDecoratedBasket()->filterData().isFiltering ? i18n("all matches") : i18n("no filter"));
        if (basket->countFounds() != basket->count())
            showns = i18np("%1 match", "%1 matches", basket->countFounds());
        setSelectionStatus(i18nc("e.g. '18 notes, 10 matches, 5 selected'", "%1, %2, %3", count, showns, selecteds));
    }

    if (currentBasket()->redirectEditActions()) {
        m_actSelectAll->setEnabled(!currentBasket()->selectedAllTextInEditor());
        m_actUnselectAll->setEnabled(currentBasket()->hasSelectedTextInEditor());
    } else {
        m_actSelectAll->setEnabled(basket->countSelecteds() < basket->countFounds());
        m_actUnselectAll->setEnabled(basket->countSelecteds() > 0);
    }
    m_actInvertSelection->setEnabled(basket->countFounds() > 0);

    updateNotesActions();
}

void BNPView::updateNotesActions()
{
    bool isLocked = currentBasket()->isLocked();
    bool oneSelected = currentBasket()->countSelecteds() == 1;
    bool oneOrSeveralSelected = currentBasket()->countSelecteds() >= 1;
    bool severalSelected = currentBasket()->countSelecteds() >= 2;

    // FIXME: m_actCheckNotes is also modified in void BNPView::areSelectedNotesCheckedChanged(bool checked)
    //        bool BasketScene::areSelectedNotesChecked() should return false if bool BasketScene::showCheckBoxes() is false
    //  m_actCheckNotes->setChecked( oneOrSeveralSelected &&
    //                               currentBasket()->areSelectedNotesChecked() &&
    //                               currentBasket()->showCheckBoxes()             );

    Note *selectedGroup = (severalSelected ? currentBasket()->selectedGroup() : nullptr);

    m_actEditNote->setEnabled(!isLocked && oneSelected && !currentBasket()->isDuringEdit());
    if (currentBasket()->redirectEditActions()) {
        m_actCutNote->setEnabled(currentBasket()->hasSelectedTextInEditor());
        m_actCopyNote->setEnabled(currentBasket()->hasSelectedTextInEditor());
        m_actPaste->setEnabled(true);
        m_actDelNote->setEnabled(currentBasket()->hasSelectedTextInEditor());
    } else {
        m_actCutNote->setEnabled(!isLocked && oneOrSeveralSelected);
        m_actCopyNote->setEnabled(oneOrSeveralSelected);
        m_actPaste->setEnabled(!isLocked);
        m_actDelNote->setEnabled(!isLocked && oneOrSeveralSelected);
    }
    m_actOpenNote->setEnabled(oneOrSeveralSelected);
    m_actOpenNoteWith->setEnabled(oneSelected); // TODO: oneOrSeveralSelected IF SAME TYPE
    m_actSaveNoteAs->setEnabled(oneSelected);   // IDEM?
    m_actGroup->setEnabled(!isLocked && severalSelected && (!selectedGroup || selectedGroup->isColumn()));
    m_actUngroup->setEnabled(!isLocked && selectedGroup && !selectedGroup->isColumn());
    m_actMoveOnTop->setEnabled(!isLocked && oneOrSeveralSelected && !currentBasket()->isFreeLayout());
    m_actMoveNoteUp->setEnabled(!isLocked && oneOrSeveralSelected); // TODO: Disable when unavailable!
    m_actMoveNoteDown->setEnabled(!isLocked && oneOrSeveralSelected);
    m_actMoveOnBottom->setEnabled(!isLocked && oneOrSeveralSelected && !currentBasket()->isFreeLayout());

    for (QList<QAction *>::const_iterator action = m_insertActions.constBegin(); action != m_insertActions.constEnd(); ++action)
        (*action)->setEnabled(!isLocked);

    // From the old Note::contextMenuEvent(...) :
    /*  if (useFile() || m_type == Link) {
        m_type == Link ? i18n("&Open target")         : i18n("&Open")
        m_type == Link ? i18n("Open target &with...") : i18n("Open &with...")
        m_type == Link ? i18n("&Save target as...")   : i18n("&Save a copy as...")
            // If useFile() there is always a file to open / open with / save, but :
        if (m_type == Link) {
                if (url().toDisplayString().isEmpty() && runCommand().isEmpty())     // no URL nor runCommand :
        popupMenu->setItemEnabled(7, false);                       //  no possible Open !
                if (url().toDisplayString().isEmpty())                               // no URL :
        popupMenu->setItemEnabled(8, false);                       //  no possible Open with !
                if (url().toDisplayString().isEmpty() || url().path().endsWith("/")) // no URL or target a folder :
        popupMenu->setItemEnabled(9, false);                       //  not possible to save target file
    }
    } else if (m_type != Color) {
        popupMenu->insertSeparator();
        popupMenu->insertItem( QIcon::fromTheme("document-save-as"), i18n("&Save a copy as..."), this, SLOT(slotSaveAs()), 0, 10 );
    }*/
}

// BEGIN Color picker (code from KColorEdit):

/* Activate the mode
 */
void BNPView::slotColorFromScreen(bool global)
{
    m_colorPickWasGlobal = global;

    currentBasket()->saveInsertionData();
    m_colorPicker->pickColor();
}

void BNPView::slotColorFromScreenGlobal()
{
    slotColorFromScreen(true);
}

void BNPView::colorPicked(const QColor &color)
{
    if (!currentBasket()->isLoaded()) {
        showPassiveLoading(currentBasket());
        currentBasket()->load();
    }
    currentBasket()->insertColor(color);

    if (Settings::usePassivePopup()) {
        showPassiveDropped(i18n("Picked color to basket <i>%1</i>"));
    }
}

void BNPView::slotConvertTexts()
{
    /*
        int result = KMessageBox::questionYesNoCancel(
            this,
            i18n(
                "<p>This will convert every text notes into rich text notes.<br>"
                "The content of the notes will not change and you will be able to apply formatting to those notes.</p>"
                "<p>This process cannot be reverted back: you will not be able to convert the rich text notes to plain text ones later.</p>"
                "<p>As a beta-tester, you are strongly encouraged to do the convert process because it is to test if plain text notes are still needed.<br>"
                "If nobody complain about not having plain text notes anymore, then the final version is likely to not support plain text notes anymore.</p>"
                "<p><b>Which basket notes do you want to convert?</b></p>"
            ),
            i18n("Convert Text Notes"),
            KGuiItem(i18n("Only in the Current Basket")),
            KGuiItem(i18n("In Every Baskets"))
        );
        if (result == KMessageBox::Cancel)
            return;
    */

    bool conversionsDone;
    //  if (result == KMessageBox::Yes)
    //      conversionsDone = currentBasket()->convertTexts();
    //  else
    conversionsDone = convertTexts();

    if (conversionsDone)
        KMessageBox::information(this, i18n("The plain text notes have been converted to rich text."), i18n("Conversion Finished"));
    else
        KMessageBox::information(this, i18n("There are no plain text notes to convert."), i18n("Conversion Finished"));
}

QMenu *BNPView::popupMenu(const QString &menuName)
{
    QMenu *menu = nullptr;

    if (m_guiClient) {
        qDebug() << "m_guiClient";
        KXMLGUIFactory *factory = m_guiClient->factory();
        if (factory) {
            menu = (QMenu *)factory->container(menuName, m_guiClient);
        }
    }
    if (menu == nullptr) {
        QString basketDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/basket/";

        KMessageBox::error(this,
                           i18n("<p><b>The file basketui.rc seems to not exist or is too old.<br>"
                                "%1 cannot run without it and will stop.</b></p>"
                                "<p>Please check your installation of %2.</p>"
                                "<p>If you do not have administrator access to install the application "
                                "system wide, you can copy the file basketui.rc from the installation "
                                "archive to the folder <a href='file://%3'>%4</a>.</p>"
                                "<p>As last resort, if you are sure the application is correctly installed "
                                "but you had a preview version of it, try to remove the "
                                "file %5basketui.rc</p>",
                                QGuiApplication::applicationDisplayName(),
                                QGuiApplication::applicationDisplayName(),
                                basketDataPath,
                                basketDataPath,
                                basketDataPath),
                           i18n("Resource not Found"),
                           KMessageBox::AllowLink);
        exit(1); // We SHOULD exit right now and aboard everything because the caller except menu != 0 to not crash.
    }
    return menu;
}

void BNPView::showHideFilterBar(bool show, bool switchFocus)
{
    //  if (show != m_actShowFilter->isChecked())
    //      m_actShowFilter->setChecked(show);
    m_actShowFilter->setChecked(show);

    currentDecoratedBasket()->setFilterBarVisible(show, switchFocus);
    if (!show)
        currentDecoratedBasket()->resetFilter();
}

void BNPView::insertEmpty(int type)
{
    if (currentBasket()->isLocked()) {
        showPassiveImpossible(i18n("Cannot add note."));
        return;
    }
    currentBasket()->insertEmptyNote(type);
}

void BNPView::insertWizard(int type)
{
    if (currentBasket()->isLocked()) {
        showPassiveImpossible(i18n("Cannot add note."));
        return;
    }
    currentBasket()->insertWizard(type);
}

// BEGIN Screen Grabbing:
void BNPView::grabScreenshot(bool global)
{
    if (m_regionGrabber) {
        KWindowSystem::activateWindow(m_regionGrabber->winId());
        return;
    }

    // Delay before to take a screenshot because if we hide the main window OR the systray popup menu,
    // we should wait the windows below to be repainted!!!
    // A special case is where the action is triggered with the global keyboard shortcut.
    // In this case, global is true, and we don't wait.
    // In the future, if global is also defined for other cases, check for
    // enum QAction::ActivationReason { UnknownActivation, EmulatedActivation, AccelActivation, PopupMenuActivation, ToolBarActivation };
    int delay = (isMainWindowActive() ? 500 : (global /*qApp->activePopupWidget()*/ ? 0 : 200));

    m_colorPickWasGlobal = global;
    hideMainWindow();

    currentBasket()->saveInsertionData();
    usleep(delay * 1000);
    m_regionGrabber = new RegionGrabber;
    connect(m_regionGrabber, &RegionGrabber::regionGrabbed, this, &BNPView::screenshotGrabbed);
}

void BNPView::hideMainWindow()
{
    if (isMainWindowActive()) {
        if (Global::activeMainWindow()) {
            m_HiddenMainWindow = Global::activeMainWindow();
            m_HiddenMainWindow->hide();
        }
        m_colorPickWasShown = true;
    } else
        m_colorPickWasShown = false;
}

void BNPView::grabScreenshotGlobal()
{
    grabScreenshot(true);
}

void BNPView::screenshotGrabbed(const QPixmap &pixmap)
{
    delete m_regionGrabber;
    m_regionGrabber = nullptr;

    // Cancelled (pressed Escape):
    if (pixmap.isNull()) {
        if (m_colorPickWasShown)
            showMainWindow();
        return;
    }

    if (!currentBasket()->isLoaded()) {
        showPassiveLoading(currentBasket());
        currentBasket()->load();
    }
    currentBasket()->insertImage(pixmap);

    if (m_colorPickWasShown)
        showMainWindow();

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Grabbed screen zone to basket <i>%1</i>"));
}

BasketScene *BNPView::basketForFolderName(const QString &folderName)
{
    /*  QPtrList<Basket> basketsList = listBaskets();
        BasketScene *basket;
        for (basket = basketsList.first(); basket; basket = basketsList.next())
        if (basket->folderName() == folderName)
        return basket;
    */

    QString name = folderName;
    if (!name.endsWith('/'))
        name += '/';

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        if (item->basket()->folderName() == name)
            return item->basket();
        ++it;
    }

    return nullptr;
}

Note *BNPView::noteForFileName(const QString &fileName, BasketScene &basket, Note *note)
{
    if (!note)
        note = basket.firstNote();
    if (note->fullPath().endsWith(fileName))
        return note;
    Note *child = note->firstChild();
    Note *found;
    while (child) {
        found = noteForFileName(fileName, basket, child);
        if (found)
            return found;
        child = child->next();
    }
    return nullptr;
}

void BNPView::setFiltering(bool filtering)
{
    m_actShowFilter->setChecked(filtering);
    m_actResetFilter->setEnabled(filtering);
    if (!filtering)
        m_actFilterAllBaskets->setEnabled(false);
}

void BNPView::undo()
{
    // TODO
}

void BNPView::redo()
{
    // TODO
}

void BNPView::pasteToBasket(int /*index*/, QClipboard::Mode /*mode*/)
{
    // TODO: REMOVE!
    // basketAt(index)->pasteNote(mode);
}

void BNPView::propBasket()
{
    BasketPropertiesDialog dialog(currentBasket(), this);
    dialog.exec();
}

void BNPView::delBasket()
{
    //  DecoratedBasket *decoBasket    = currentDecoratedBasket();
    BasketScene *basket = currentBasket();

    int really = KMessageBox::questionYesNo(this,
                                            i18n("<qt>Do you really want to remove the basket <b>%1</b> and its contents?</qt>", Tools::textToHTMLWithoutP(basket->basketName())),
                                            i18n("Remove Basket"),
                                            KGuiItem(i18n("&Remove Basket"), "edit-delete"),
                                            KStandardGuiItem::cancel());

    if (really == KMessageBox::No)
        return;

    QStringList basketsList = listViewItemForBasket(basket)->childNamesTree(0);
    if (basketsList.count() > 0) {
        int deleteChilds = KMessageBox::questionYesNoList(this,
                                                          i18n("<qt><b>%1</b> has the following children baskets.<br>Do you want to remove them too?</qt>", Tools::textToHTMLWithoutP(basket->basketName())),
                                                          basketsList,
                                                          i18n("Remove Children Baskets"),
                                                          KGuiItem(i18n("&Remove Children Baskets"), "edit-delete"));

        if (deleteChilds == KMessageBox::No)
            return;
    }

    QString basketFolderName = basket->folderName();
    doBasketDeletion(basket);

    GitWrapper::commitDeleteBasket(basketFolderName);
}

void BNPView::doBasketDeletion(BasketScene *basket)
{
    basket->closeEditor();

    QTreeWidgetItem *basketItem = listViewItemForBasket(basket);
    for (int i = 0; i < basketItem->childCount(); i++) {
        // First delete the child baskets:
        doBasketDeletion(((BasketListViewItem *)basketItem->child(i))->basket());
    }
    // Then, basket have no child anymore, delete it:
    DecoratedBasket *decoBasket = basket->decoration();
    basket->deleteFiles();
    removeBasket(basket);
    // Remove the action to avoid keyboard-shortcut clashes:
    delete basket->m_action; // FIXME: It's quick&dirty. In the future, the Basket should be deleted, and then the QAction deleted in the Basket destructor.
    delete decoBasket;
    //  delete basket;
}

void BNPView::password()
{
#ifdef HAVE_LIBGPGME
    QPointer<PasswordDlg> dlg = new PasswordDlg(qApp->activeWindow());
    BasketScene *cur = currentBasket();

    dlg->setType(cur->encryptionType());
    dlg->setKey(cur->encryptionKey());
    if (dlg->exec()) {
        cur->setProtection(dlg->type(), dlg->key());
        if (cur->encryptionType() != BasketScene::NoEncryption) {
            // Clear metadata
            Tools::deleteMetadataRecursively(cur->fullPath());
            cur->lock();
        }
    }
#endif
}

void BNPView::lockBasket()
{
#ifdef HAVE_LIBGPGME
    BasketScene *cur = currentBasket();

    cur->lock();
#endif
}

void BNPView::saveAsArchive()
{
    BasketScene *basket = currentBasket();

    QDir dir;

    KConfigGroup config = KSharedConfig::openConfig()->group("Basket Archive");
    QString folder = config.readEntry("lastFolder", QDir::homePath()) + "/";
    QString url = folder + QString(basket->basketName()).replace('/', '_') + ".baskets";

    QString filter = "*.baskets|" + i18n("Basket Archives") + "\n*|" + i18n("All Files");
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        destination = QFileDialog::getSaveFileName(nullptr, i18n("Save as Basket Archive"), destination, filter);
        if (destination.isEmpty()) // User canceled
            return;
        if (dir.exists(destination)) {
            int result = KMessageBox::questionYesNoCancel(
                this, "<qt>" + i18n("The file <b>%1</b> already exists. Do you really want to override it?", QUrl::fromLocalFile(destination).fileName()), i18n("Override File?"), KGuiItem(i18n("&Override"), "document-save"));
            if (result == KMessageBox::Cancel)
                return;
            else if (result == KMessageBox::Yes)
                askAgain = false;
        } else
            askAgain = false;
    }
    bool withSubBaskets = true; // KMessageBox::questionYesNo(this, i18n("Do you want to export sub-baskets too?"), i18n("Save as Basket Archive")) == KMessageBox::Yes;

    config.writeEntry("lastFolder", QUrl::fromLocalFile(destination).adjusted(QUrl::RemoveFilename).path());
    config.sync();

    Archive::save(basket, withSubBaskets, destination);
}

QString BNPView::s_fileToOpen;

void BNPView::delayedOpenArchive()
{
    Archive::open(s_fileToOpen);
}

QString BNPView::s_basketToOpen;

void BNPView::delayedOpenBasket()
{
    BasketScene *bv = this->basketForFolderName(s_basketToOpen);
    this->setCurrentBasketInHistory(bv);
}

void BNPView::openArchive()
{
    QString filter = QStringLiteral("*.baskets|") + i18n("Basket Archives") + QStringLiteral("\n*|") + i18n("All Files");
    QString path = QFileDialog::getOpenFileName(this, i18n("Open Basket Archive"), QString(), filter);
    if (!path.isEmpty()) { // User has not canceled
        Archive::open(path);
    }
}

void BNPView::activatedTagShortcut()
{
    Tag *tag = Tag::tagForKAction((QAction *)sender());
    currentBasket()->activatedTagShortcut(tag);
}

void BNPView::slotBasketChanged()
{
    m_actFoldBasket->setEnabled(canFold());
    m_actExpandBasket->setEnabled(canExpand());
    if (currentBasket()->decoration()->filterData().isFiltering)
        currentBasket()->decoration()->filterBar()->show(); // especially important for Filter all
    setFiltering(currentBasket() && currentBasket()->decoration()->filterData().isFiltering);
    this->canUndoRedoChanged();
}

void BNPView::canUndoRedoChanged()
{
    if (m_history) {
        m_actPreviousBasket->setEnabled(m_history->canUndo());
        m_actNextBasket->setEnabled(m_history->canRedo());
    }
}

void BNPView::currentBasketChanged()
{
}

void BNPView::isLockedChanged()
{
    bool isLocked = currentBasket()->isLocked();

    setLockStatus(isLocked);

    //  m_actLockBasket->setChecked(isLocked);
    m_actPropBasket->setEnabled(!isLocked);
    m_actDelBasket->setEnabled(!isLocked);
    updateNotesActions();
}

void BNPView::askNewBasket()
{
    askNewBasket(nullptr, nullptr);

    GitWrapper::commitCreateBasket();
}

void BNPView::askNewBasket(BasketScene *parent, BasketScene *pickProperties)
{
    NewBasketDefaultProperties properties;
    if (pickProperties) {
        properties.icon = pickProperties->icon();
        properties.backgroundImage = pickProperties->backgroundImageName();
        properties.backgroundColor = pickProperties->backgroundColorSetting();
        properties.textColor = pickProperties->textColorSetting();
        properties.freeLayout = pickProperties->isFreeLayout();
        properties.columnCount = pickProperties->columnsCount();
    }

    NewBasketDialog(parent, properties, this).exec();
}

void BNPView::askNewSubBasket()
{
    askNewBasket(/*parent=*/currentBasket(), /*pickPropertiesOf=*/currentBasket());
}

void BNPView::askNewSiblingBasket()
{
    askNewBasket(/*parent=*/parentBasketOf(currentBasket()), /*pickPropertiesOf=*/currentBasket());
}

void BNPView::globalPasteInCurrentBasket()
{
    currentBasket()->setInsertPopupMenu();
    pasteInCurrentBasket();
    currentBasket()->cancelInsertPopupMenu();
}

void BNPView::pasteInCurrentBasket()
{
    currentBasket()->pasteNote();

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Clipboard content pasted to basket <i>%1</i>"));
}

void BNPView::pasteSelInCurrentBasket()
{
    currentBasket()->pasteNote(QClipboard::Selection);

    if (Settings::usePassivePopup())
        showPassiveDropped(i18n("Selection pasted to basket <i>%1</i>"));
}

void BNPView::showPassiveDropped(const QString &title)
{
    if (!currentBasket()->isLocked()) {
        // TODO: Keep basket, so that we show the message only if something was added to a NOT visible basket
        m_passiveDroppedTitle = title;
        m_passiveDroppedSelection = currentBasket()->selectedNotes();
        QTimer::singleShot(c_delayTooltipTime, this, SLOT(showPassiveDroppedDelayed()));
        // DELAY IT BELOW:
    } else
        showPassiveImpossible(i18n("No note was added."));
}

void BNPView::showPassiveDroppedDelayed()
{
    if (isMainWindowActive() || m_passiveDroppedSelection == nullptr)
        return;

    QString title = m_passiveDroppedTitle;

    QImage contentsImage = NoteDrag::feedbackPixmap(m_passiveDroppedSelection).toImage();
    QResource::registerResource(contentsImage.bits(), QStringLiteral(":/images/passivepopup_image"));

    if (Settings::useSystray()) {
        /*Uncomment after switching to QSystemTrayIcon or port to KStatusNotifierItem
         See also other occurrences of Global::systemTray below*/
        /*KPassivePopup::message(KPassivePopup::Boxed,
            title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
            (contentsImage.isNull() ? QString() : QStringLiteral("<img src=\":/images/passivepopup_image\">")),
            KIconLoader::global()->loadIcon(
                currentBasket()->icon(), KIconLoader::NoGroup, 16,
                KIconLoader::DefaultState, QStringList(), 0L, true
            ),
            Global::systemTray);*/
    } else {
        KPassivePopup::message(KPassivePopup::Boxed,
                               title.arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                               (contentsImage.isNull() ? QString() : QStringLiteral("<img src=\":/images/passivepopup_image\">")),
                               KIconLoader::global()->loadIcon(currentBasket()->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, true),
                               (QWidget *)this);
    }
}

void BNPView::showPassiveImpossible(const QString &message)
{
    if (Settings::useSystray()) {
        /*KPassivePopup::message(KPassivePopup::Boxed,
                        QString("<font color=red>%1</font>")
                        .arg(i18n("Basket <i>%1</i> is locked"))
                        .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                        message,
                        KIconLoader::global()->loadIcon(
                            currentBasket()->icon(), KIconLoader::NoGroup, 16,
                            KIconLoader::DefaultState, QStringList(), 0L, true
                        ),
        Global::systemTray);*/
    } else {
        /*KPassivePopup::message(KPassivePopup::Boxed,
                        QString("<font color=red>%1</font>")
                        .arg(i18n("Basket <i>%1</i> is locked"))
                        .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                        message,
                        KIconLoader::global()->loadIcon(
                            currentBasket()->icon(), KIconLoader::NoGroup, 16,
                            KIconLoader::DefaultState, QStringList(), 0L, true
                        ),
        (QWidget*)this);*/
    }
}

void BNPView::showPassiveContentForced()
{
    showPassiveContent(/*forceShow=*/true);
}

void BNPView::showPassiveContent(bool forceShow /* = false*/)
{
    if (!forceShow && isMainWindowActive())
        return;

    // FIXME: Duplicate code (2 times)
    QString message;

    if (Settings::useSystray()) {
        /*KPassivePopup::message(KPassivePopup::Boxed,
            "<qt>" + Tools::makeStandardCaption(
                currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>")
                .arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), i18n("(Locked)"))
                : Tools::textToHTMLWithoutP(currentBasket()->basketName())
            ),
            message,
            KIconLoader::global()->loadIcon(
                currentBasket()->icon(), KIconLoader::NoGroup, 16,
                KIconLoader::DefaultState, QStringList(), 0L, true
            ),
        Global::systemTray);*/
    } else {
        KPassivePopup::message(KPassivePopup::Boxed,
                               "<qt>" +
                                   Tools::makeStandardCaption(currentBasket()->isLocked() ? QString("%1 <font color=gray30>%2</font>").arg(Tools::textToHTMLWithoutP(currentBasket()->basketName()), i18n("(Locked)"))
                                                                                          : Tools::textToHTMLWithoutP(currentBasket()->basketName())),
                               message,
                               KIconLoader::global()->loadIcon(currentBasket()->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, true),
                               (QWidget *)this);
    }
}

void BNPView::showPassiveLoading(BasketScene *basket)
{
    if (isMainWindowActive())
        return;

    if (Settings::useSystray()) {
        /*KPassivePopup::message(KPassivePopup::Boxed,
            Tools::textToHTMLWithoutP(basket->basketName()),
            i18n("Loading..."),
            KIconLoader::global()->loadIcon(
                basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
                QStringList(), 0L, true
            ),
            Global::systemTray);*/
    } else {
        KPassivePopup::message(KPassivePopup::Boxed,
                               Tools::textToHTMLWithoutP(basket->basketName()),
                               i18n("Loading..."),
                               KIconLoader::global()->loadIcon(basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, true),
                               (QWidget *)this);
    }
}

void BNPView::addNoteText()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::Text);
}
void BNPView::addNoteHtml()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::Html);
}
void BNPView::addNoteImage()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::Image);
}
void BNPView::addNoteLink()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::Link);
}
void BNPView::addNoteCrossReference()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::CrossReference);
}
void BNPView::addNoteColor()
{
    showMainWindow();
    currentBasket()->insertEmptyNote(NoteType::Color);
}

void BNPView::aboutToHideNewBasketPopup()
{
    QTimer::singleShot(0, this, SLOT(cancelNewBasketPopup()));
}

void BNPView::cancelNewBasketPopup()
{
    m_newBasketPopup = false;
}

void BNPView::setNewBasketPopup()
{
    m_newBasketPopup = true;
}

void BNPView::setWindowTitle(QString s)
{
    Q_EMIT setWindowCaption(s);
}

void BNPView::updateStatusBarHint()
{
    m_statusbar->updateStatusBarHint();
}

void BNPView::setSelectionStatus(QString s)
{
    m_statusbar->setSelectionStatus(s);
}

void BNPView::setLockStatus(bool isLocked)
{
    m_statusbar->setLockStatus(isLocked);
}

void BNPView::postStatusbarMessage(const QString &msg)
{
    m_statusbar->postStatusbarMessage(msg);
}

void BNPView::setStatusBarHint(const QString &hint)
{
    m_statusbar->setStatusBarHint(hint);
}

void BNPView::setUnsavedStatus(bool isUnsaved)
{
    m_statusbar->setUnsavedStatus(isUnsaved);
}

void BNPView::setActive(bool active)
{
    KMainWindow *win = Global::activeMainWindow();
    if (!win)
        return;

    if (active == isMainWindowActive())
        return;
    // qApp->updateUserTimestamp(); // If "activate on mouse hovering systray", or "on drag through systray"
    Global::systemTray->activate();
}

void BNPView::hideOnEscape()
{
    if (Settings::useSystray())
        setActive(false);
}

bool BNPView::isMainWindowActive()
{
    KMainWindow *main = Global::activeMainWindow();
    if (main && main->isActiveWindow())
        return true;
    return false;
}

void BNPView::newBasket()
{
    askNewBasket();
}

bool BNPView::createNoteHtml(const QString content, const QString basket)
{
    BasketScene *b = basketForFolderName(basket);
    if (!b)
        return false;
    Note *note = NoteFactory::createNoteHtml(content, b);
    if (!note)
        return false;
    b->insertCreatedNote(note);
    return true;
}

bool BNPView::changeNoteHtml(const QString content, const QString basket, const QString noteName)
{
    BasketScene *b = basketForFolderName(basket);
    if (!b)
        return false;
    Note *note = noteForFileName(noteName, *b);
    if (!note || note->content()->type() != NoteType::Html)
        return false;
    HtmlContent *noteContent = (HtmlContent *)note->content();
    noteContent->setHtml(content);
    note->saveAgain();
    return true;
}

bool BNPView::createNoteFromFile(const QString url, const QString basket)
{
    BasketScene *b = basketForFolderName(basket);
    if (!b)
        return false;
    QUrl kurl(url);
    if (url.isEmpty())
        return false;
    Note *n = NoteFactory::copyFileAndLoad(kurl, b);
    if (!n)
        return false;
    b->insertCreatedNote(n);
    return true;
}

QStringList BNPView::listBaskets()
{
    QStringList basketList;

    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        BasketListViewItem *item = ((BasketListViewItem *)*it);
        basketList.append(item->basket()->basketName());
        basketList.append(item->basket()->folderName());
        ++it;
    }
    return basketList;
}

void BNPView::handleCommandLine()
{
    QCommandLineParser *parser = Global::commandLineOpts;

    /* Custom data folder */
    QString customDataFolder = parser->value("data-folder");
    if (!customDataFolder.isNull() && !customDataFolder.isEmpty()) {
        Global::setCustomSavesFolder(customDataFolder);
    }
    /* Debug window */
    if (parser->isSet("debug")) {
        new DebugWindow();
        Global::debugWindow->show();
    }
}

void BNPView::reloadBasket(const QString &folderName)
{
    basketForFolderName(folderName)->reload();
}

/** Scenario of "Hide main window to system tray icon when mouse move out of the window" :
 * - At enterEvent() we stop m_tryHideTimer
 * - After that and before next, we are SURE cursor is hovering window
 * - At leaveEvent() we restart m_tryHideTimer
 * - Every 'x' ms, timeoutTryHide() seek if cursor hover a widget of the application or not
 * - If yes, we musn't hide the window
 * - But if not, we start m_hideTimer to hide main window after a configured elapsed time
 * - timeoutTryHide() continue to be called and if cursor move again to one widget of the app, m_hideTimer is stopped
 * - If after the configured time cursor hasn't go back to a widget of the application, timeoutHide() is called
 * - It then hide the main window to systray icon
 * - When the user will show it, enterEvent() will be called the first time he enter mouse to it
 * - ...
 */

/** Why do as this ? Problems with the use of only enterEvent() and leaveEvent() :
 * - Resize window or hover titlebar isn't possible : leave/enterEvent
 *   are
 *   > Use the grip or Alt+rightDND to resize window
 *   > Use Alt+DND to move window
 * - Each menu trigger the leavEvent
 */

void BNPView::enterEvent(QEvent *)
{
    if (m_tryHideTimer)
        m_tryHideTimer->stop();
    if (m_hideTimer)
        m_hideTimer->stop();
}

void BNPView::leaveEvent(QEvent *)
{
    if (Settings::useSystray() && Settings::hideOnMouseOut() && m_tryHideTimer)
        m_tryHideTimer->start(50);
}

void BNPView::timeoutTryHide()
{
    // If a menu is displayed, do nothing for the moment
    if (qApp->activePopupWidget() != nullptr)
        return;

    if (qApp->widgetAt(QCursor::pos()) != nullptr)
        m_hideTimer->stop();
    else if (!m_hideTimer->isActive()) { // Start only one time
        m_hideTimer->setSingleShot(true);
        m_hideTimer->start(Settings::timeToHideOnMouseOut() * 100);
    }

    // If a subdialog is opened, we mustn't hide the main window:
    if (qApp->activeWindow() != nullptr && qApp->activeWindow() != Global::activeMainWindow())
        m_hideTimer->stop();
}

void BNPView::timeoutHide()
{
    // We check that because the setting can have been set to off
    if (Settings::useSystray() && Settings::hideOnMouseOut())
        setActive(false);
    m_tryHideTimer->stop();
}

void BNPView::changedSelectedNotes()
{
    //  tabChanged(0); // FIXME: NOT OPTIMIZED
}

/*void BNPView::areSelectedNotesCheckedChanged(bool checked)
{
    m_actCheckNotes->setChecked(checked && currentBasket()->showCheckBoxes());
}*/

void BNPView::enableActions()
{
    BasketScene *basket = currentBasket();
    if (!basket)
        return;
    if (m_actLockBasket)
        m_actLockBasket->setEnabled(!basket->isLocked() && basket->isEncrypted());
    if (m_actPassBasket)
        m_actPassBasket->setEnabled(!basket->isLocked());
    m_actPropBasket->setEnabled(!basket->isLocked());
    m_actDelBasket->setEnabled(!basket->isLocked());
    m_actExportToHtml->setEnabled(!basket->isLocked());
    m_actShowFilter->setEnabled(!basket->isLocked());
    m_actFilterAllBaskets->setEnabled(!basket->isLocked());
    m_actResetFilter->setEnabled(!basket->isLocked());
    basket->decoration()->filterBar()->setEnabled(!basket->isLocked());
}

void BNPView::showMainWindow()
{
    if (m_HiddenMainWindow) {
        m_HiddenMainWindow->show();
        m_HiddenMainWindow = nullptr;
    } else {
        KMainWindow *win = Global::activeMainWindow();

        if (win) {
            win->show();
        }
    }

    setActive(true);
    Q_EMIT showPart();
}

void BNPView::populateTagsMenu()
{
    QMenu *menu = (QMenu *)(popupMenu("tags"));
    if (menu == nullptr || currentBasket() == nullptr) // TODO: Display a messagebox. [menu is 0, surely because on first launch, the XMLGUI does not work!]
        return;
    menu->clear();

    Note *referenceNote;
    if (currentBasket()->focusedNote() && currentBasket()->focusedNote()->isSelected())
        referenceNote = currentBasket()->focusedNote();
    else
        referenceNote = currentBasket()->firstSelected();

    populateTagsMenu(*menu, referenceNote);

    m_lastOpenedTagsMenu = menu;
    //connect(menu, &QMenu::aboutToHide, this, &BNPView::disconnectTagsMenu);
}

void BNPView::populateTagsMenu(QMenu &menu, Note *referenceNote)
{
    if (currentBasket() == nullptr)
        return;

    currentBasket()->m_tagPopupNote = referenceNote;
    bool enable = currentBasket()->countSelecteds() > 0;

    QList<Tag *>::iterator it;
    Tag *currentTag;
    State *currentState;
    int i = 10;
    for (it = Tag::all.begin(); it != Tag::all.end(); ++it) {
        // Current tag and first state of it:
        currentTag = *it;
        currentState = currentTag->states().first();

        QKeySequence sequence;
        if (!currentTag->shortcut().isEmpty())
            sequence = currentTag->shortcut();

        StateAction *mi = new StateAction(currentState, QKeySequence(sequence), this, true);

        // The previously set ID will be set in the actions themselves as data.
        mi->setData(i);

        if (referenceNote && referenceNote->hasTag(currentTag))
            mi->setChecked(true);

        menu.addAction(mi);

        if (!currentTag->shortcut().isEmpty())
            m_actionCollection->setDefaultShortcut(mi, sequence);

        mi->setEnabled(enable);
        ++i;
    }

    menu.addSeparator();

    // I don't like how this is implemented; but I can't think of a better way
    // to do this, so I will have to leave it for now
    QAction *act = new QAction(i18n("&Assign new Tag..."), &menu);
    act->setData(1);
    act->setEnabled(enable);
    menu.addAction(act);

    act = new QAction(QIcon::fromTheme("edit-delete"), i18n("&Remove All"), &menu);
    act->setData(2);
    if (!currentBasket()->selectedNotesHaveTags())
        act->setEnabled(false);
    menu.addAction(act);

    act = new QAction(QIcon::fromTheme("configure"), i18n("&Customize..."), &menu);
    act->setData(3);
    menu.addAction(act);

    connect(&menu, &QMenu::triggered, currentBasket(), &BasketScene::toggledTagInMenu);
    connect(&menu, &QMenu::aboutToHide, currentBasket(), &BasketScene::unlockHovering);
    connect(&menu, &QMenu::aboutToHide, currentBasket(), &BasketScene::disableNextClick);
}

void BNPView::connectTagsMenu()
{
    connect(popupMenu("tags"), &QMenu::aboutToShow, this, [this]() { this->populateTagsMenu(); });
    connect(popupMenu("tags"), &QMenu::aboutToHide, this, &BNPView::disconnectTagsMenu);
}

void BNPView::showEvent(QShowEvent *)
{
    if (m_firstShow) {
        m_firstShow = false;
        onFirstShow();
    }
}

void BNPView::disconnectTagsMenu()
{
    QTimer::singleShot(0, this, SLOT(disconnectTagsMenuDelayed()));
}

void BNPView::disconnectTagsMenuDelayed()
{
    disconnect(m_lastOpenedTagsMenu, SIGNAL(triggered(QAction *)), currentBasket(), SLOT(toggledTagInMenu(QAction *)));
    disconnect(m_lastOpenedTagsMenu, SIGNAL(aboutToHide()), currentBasket(), SLOT(unlockHovering()));
    disconnect(m_lastOpenedTagsMenu, SIGNAL(aboutToHide()), currentBasket(), SLOT(disableNextClick()));
}

void BNPView::loadCrossReference(QString link)
{
    // remove "basket://" and any encoding.
    QString folderName = link.mid(9, link.length() - 9);
    folderName = QUrl::fromPercentEncoding(folderName.toUtf8());

    BasketScene *basket = this->basketForFolderName(folderName);

    if (!basket)
        return;

    this->setCurrentBasketInHistory(basket);
}

QString BNPView::folderFromBasketNameLink(QStringList pages, QTreeWidgetItem *parent)
{
    QString found;

    QString page = pages.first();

    page = QUrl::fromPercentEncoding(page.toUtf8());
    pages.removeFirst();

    if (page == "..") {
        QTreeWidgetItem *p;
        if (parent)
            p = parent->parent();
        else
            p = m_tree->currentItem()->parent();
        found = this->folderFromBasketNameLink(pages, p);
    } else if (!parent && page.isEmpty()) {
        parent = m_tree->invisibleRootItem();
        found = this->folderFromBasketNameLink(pages, parent);
    } else {
        if (!parent && (page == "." || !page.isEmpty())) {
            parent = m_tree->currentItem();
        }
        QRegExp re(":\\{([0-9]+)\\}");
        re.setMinimal(true);
        int pos = 0;

        pos = re.indexIn(page, pos);
        int basketNum = 1;

        if (pos != -1)
            basketNum = re.cap(1).toInt();

        page = page.left(page.length() - re.matchedLength());

        for (int i = 0; i < parent->childCount(); i++) {
            QTreeWidgetItem *child = parent->child(i);

            if (child->text(0).toLower() == page.toLower()) {
                basketNum--;
                if (basketNum == 0) {
                    if (pages.count() > 0) {
                        found = this->folderFromBasketNameLink(pages, child);
                        break;
                    } else {
                        found = ((BasketListViewItem *)child)->basket()->folderName();
                        break;
                    }
                }
            } else
                found = QString();
        }
    }

    return found;
}

void BNPView::sortChildrenAsc()
{
    m_tree->currentItem()->sortChildren(0, Qt::AscendingOrder);
}

void BNPView::sortChildrenDesc()
{
    m_tree->currentItem()->sortChildren(0, Qt::DescendingOrder);
}

void BNPView::sortSiblingsAsc()
{
    QTreeWidgetItem *parent = m_tree->currentItem()->parent();
    if (!parent)
        m_tree->sortItems(0, Qt::AscendingOrder);
    else
        parent->sortChildren(0, Qt::AscendingOrder);
}

void BNPView::sortSiblingsDesc()
{
    QTreeWidgetItem *parent = m_tree->currentItem()->parent();
    if (!parent)
        m_tree->sortItems(0, Qt::DescendingOrder);
    else
        parent->sortChildren(0, Qt::DescendingOrder);
}
