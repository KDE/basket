/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bnpview.h"
#include "common.h"

#include <QAction>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QEvent>
#include <QGraphicsView>
#include <QHideEvent>
#include <QImage>
#include <QKeyEvent>
#include <QList>
#include <QMenu>
#include <QPixmap>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStackedWidget>
#include <QUndoStack>
#include <QtXml/QDomDocument>

#include <KAboutData>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
// #include <KPassivePopup>
#include <KMessageWidget>
#include <KStandardShortcut>
#include <KToggleAction>
#include <KWindowSystem>
#include <KXMLGUIFactory>

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

// #include "bnpviewadaptor.h"

using namespace std::chrono_literals;

/** class BNPView: */

const int BNPView::c_delayTooltipTime = 275;

BNPView::BNPView(QWidget *parent, const char *name, KXMLGUIClient *aGUIClient, KActionCollection *actionCollection, BasketStatusBar *bar)
    : QSplitter(Qt::Horizontal, parent)
    , m_actLockBasket(nullptr)
    , m_actPassBasket(nullptr)
    , m_loading(true)
    , m_newBasketPopup(false)
    , m_firstShow(true)
#ifndef _WIN32
    , m_colorPicker(new ColorPicker(this))
#endif
    , m_regionGrabber(nullptr)
    , m_passiveDroppedSelection(nullptr)
    , m_actionCollection(actionCollection)
    , m_guiClient(aGUIClient)
    , m_statusbar(bar)
{
    // new BNPViewAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/BNPView"), this);

    setObjectName(name);

    /* Settings */
    Settings::loadConfig();

    Global::bnpView = this;

    // Needed when loading the baskets:
    Global::backgroundManager = new BackgroundManager();

    setupGlobalShortcuts();
    m_history = new QUndoStack(this);
    initialize();
    QTimer::singleShot(0, this, &BNPView::lateInit);
}

BNPView::~BNPView()
{
    int treeWidth = Global::bnpView->sizes()[Settings::treeOnLeft() ? 0 : 1];

    Settings::setBasketTreeWidth(treeWidth);

    if (currentBasket() && currentBasket()->isDuringEdit())
        currentBasket()->closeEditor();

    Settings::saveConfig();

    Global::bnpView = nullptr;

    delete m_statusbar;
    delete m_history;
    m_history = nullptr;

    NoteDrag::createAndEmptyCuttingTmpFolder(); // Clean the temporary folder we used
}

void BNPView::lateInit()
{
    // If the main window is hidden when session is saved, Container::queryClose()
    //  isn't called and the last value would be kept
    Settings::saveConfig();

    // Load baskets
    DEBUG_WIN << QStringLiteral("Baskets are loaded from ") + Global::basketsFolder();

    NoteDrag::createAndEmptyCuttingTmpFolder(); // If last exec hasn't done it: clean the temporary folder we will use
    Tag::loadTags(); // Tags should be ready before loading baskets, but tags need the mainContainer to be ready to create KActions!
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
}

void BNPView::addWelcomeBaskets()
{
    // Possible paths where to find the welcome basket archive, trying the translated one, and falling back to the English one:
    QStringList possiblePaths;
    if (QString::fromUtf8(Tools::systemCodeset())
        == QStringLiteral("UTF-8")) { // Welcome baskets are encoded in UTF-8. If the system is not, then use the English version:
        QString lang = QLocale().languageToString(QLocale().language());
        possiblePaths.append(
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("basket/welcome/Welcome_") + lang + QStringLiteral(".baskets")));
        possiblePaths.append(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                    QStringLiteral("basket/welcome/Welcome_") + lang.split(QLatin1Char('_'))[0] + QStringLiteral(".baskets")));
    }
    possiblePaths.append(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("basket/welcome/Welcome_en_US.baskets")));

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
    auto *ac = new KActionCollection(this);
    QAction *a = nullptr;

    // Ctrl+Shift+W only works when started standalone:

    int modifier = Qt::CTRL | Qt::ALT | Qt::SHIFT;

    a = ac->addAction(QStringLiteral("global_paste"), Global::bnpView, &BNPView::globalPasteInCurrentBasket);
    a->setText(i18n("Paste clipboard contents in current basket"));
    a->setStatusTip(
        i18n("Allows you to paste clipboard contents in the current basket "
             "without having to open the main window."));
    KGlobalAccel::setGlobalShortcut(a, QKeySequence(modifier + Qt::Key_V));

    a = ac->addAction(QStringLiteral("global_paste_selection"), Global::bnpView, &BNPView::pasteSelInCurrentBasket);
    a->setText(i18n("Paste selection in current basket"));
    a->setStatusTip(
        i18n("Allows you to paste clipboard selection in the current basket "
             "without having to open the main window."));
    KGlobalAccel::setGlobalShortcut(a, (QKeySequence(Qt::CTRL | Qt::ALT | Qt::SHIFT + Qt::Key_S)));

    a = ac->addAction(QStringLiteral("global_new_basket"), Global::bnpView, qOverload<>(&BNPView::askNewBasket));
    a->setText(i18n("Create a new basket"));
    a->setStatusTip(
        i18n("Allows you to create a new basket without having to open the "
             "main window (you then can use the other global shortcuts to add "
             "a note, paste clipboard or paste selection in this new basket)."));

    a = ac->addAction(QStringLiteral("global_previous_basket"), Global::bnpView, &BNPView::goToPreviousBasket);
    a->setText(i18n("Go to previous basket"));
    a->setStatusTip(
        i18n("Allows you to change current basket to the previous one without "
             "having to open the main window."));

    a = ac->addAction(QStringLiteral("global_next_basket"), Global::bnpView, &BNPView::goToNextBasket);
    a->setText(i18n("Go to next basket"));
    a->setStatusTip(
        i18n("Allows you to change current basket to the next one "
             "without having to open the main window."));

    a = ac->addAction(QStringLiteral("global_note_add_html"), Global::bnpView, &BNPView::addNoteHtml);
    a->setText(i18n("Insert text note"));
    a->setStatusTip(
        i18n("Add a text note to the current basket without having to open "
             "the main window."));
    KGlobalAccel::setGlobalShortcut(a, (QKeySequence(modifier + Qt::Key_T)));

    a = ac->addAction(QStringLiteral("global_note_add_image"), Global::bnpView, &BNPView::addNoteImage);
    a->setText(i18n("Insert image note"));
    a->setStatusTip(
        i18n("Add an image note to the current basket without having to open "
             "the main window."));

    a = ac->addAction(QStringLiteral("global_note_add_link"), Global::bnpView, &BNPView::addNoteLink);
    a->setText(i18n("Insert link note"));
    a->setStatusTip(
        i18n("Add a link note to the current basket without having "
             "to open the main window."));

    a = ac->addAction(QStringLiteral("global_note_add_color"), Global::bnpView, &BNPView::addNoteColor);
    a->setText(i18n("Insert color note"));
    a->setStatusTip(
        i18n("Add a color note to the current basket without having to open "
             "the main window."));

    a = ac->addAction(QStringLiteral("global_note_pick_color"), Global::bnpView, &BNPView::slotColorFromScreenGlobal);
    a->setText(i18n("Pick color from screen"));
    a->setStatusTip(
        i18n("Add a color note picked from one pixel on screen to the current "
             "basket without "
             "having to open the main window."));

    a = ac->addAction(QStringLiteral("global_note_grab_screenshot"), Global::bnpView, &BNPView::grabScreenshotGlobal);
    a->setText(i18n("Grab screen zone"));
    a->setStatusTip(
        i18n("Grab a screen zone as an image in the current basket without "
             "having to open the main window."));

#if 0
    a = ac->addAction("global_note_add_text", Global::bnpView,
                      &BNPView::addNoteText);
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

    a = ac->addAction(QStringLiteral("basket_export_basket_archive"), this, &BNPView::saveAsArchive);
    a->setText(i18n("&Basket Archive..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("baskets")));
    a->setShortcut(0);
    m_actSaveAsArchive = a;

    a = ac->addAction(QStringLiteral("basket_import_basket_archive"), this, &BNPView::openArchive);
    a->setText(i18n("&Basket Archive..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("baskets")));
    a->setShortcut(0);
    m_actOpenArchive = a;

    a = ac->addAction(QStringLiteral("basket_export_html"), this, &BNPView::exportToHTML);
    a->setText(i18n("&HTML Web Page..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("text-html")));
    a->setShortcut(0);
    m_actExportToHtml = a;

    a = ac->addAction(QStringLiteral("basket_import_text_file"), this, &BNPView::importTextFile);
    a->setText(i18n("Text &File..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));
    a->setShortcut(0);

    a = ac->addAction(QStringLiteral("basket_backup_restore"), this, &BNPView::backupRestore);
    a->setText(i18n("&Backup && Restore..."));
    a->setShortcut(0);

    a = ac->addAction(QStringLiteral("check_cleanup"), this, &BNPView::checkCleanup);
    a->setText(i18n("&Check && Cleanup..."));
    a->setShortcut(0);
    if (Global::commandLineOpts->isSet(QStringLiteral("debug"))) {
        a->setEnabled(true);
    } else {
        a->setEnabled(false);
    }

    /** Note : ****************************************************************/

    a = ac->addAction(QStringLiteral("edit_delete"), this, &BNPView::delNote);
    a->setText(i18n("D&elete"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Delete")));
    m_actDelNote = a;

    m_actCutNote = KStandardAction::cut(this, &BNPView::cutNote, ac);
    m_actCopyNote = KStandardAction::copy(this, &BNPView::copyNote, ac);

    m_actSelectAll = KStandardAction::selectAll(this, &BNPView::slotSelectAll, ac);
    m_actSelectAll->setStatusTip(i18n("Selects all notes"));

    a = ac->addAction(QStringLiteral("edit_unselect_all"), this, &BNPView::slotUnselectAll);
    a->setText(i18n("U&nselect All"));
    m_actUnselectAll = a;
    m_actUnselectAll->setStatusTip(i18n("Unselects all selected notes"));

    a = ac->addAction(QStringLiteral("edit_invert_selection"), this, &BNPView::slotInvertSelection);
    a->setText(i18n("&Invert Selection"));
    m_actionCollection->setDefaultShortcut(a, Qt::CTRL + Qt::Key_Asterisk);
    m_actInvertSelection = a;

    m_actInvertSelection->setStatusTip(i18n("Inverts the current selection of notes"));

    m_actClearFormatting = ac->addAction(QStringLiteral("note_clear"), this, &BNPView::clearFormattingNote);
    m_actClearFormatting->setText(i18n("&Clear Formatting"));
    m_actClearFormatting->setIcon(QIcon::fromTheme(QStringLiteral("text-plain")));

    a = ac->addAction(QStringLiteral("note_edit"), this, &BNPView::editNote);
    a->setText(i18nc("Verb; not Menu", "&Edit..."));
    // a->setIcon(QIcon::fromTheme("edit"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Return")));
    m_actEditNote = a;

    m_actOpenNote = ac->addAction(KStandardAction::Open, QStringLiteral("note_open"), this, &BNPView::openNote);
    m_actOpenNote->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    m_actOpenNote->setText(i18n("&Open"));
    m_actionCollection->setDefaultShortcut(m_actOpenNote, QKeySequence(QStringLiteral("F9")));

    a = ac->addAction(QStringLiteral("note_open_with"), this, &BNPView::openNoteWith);
    a->setText(i18n("Open &With..."));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Shift+F9")));
    m_actOpenNoteWith = a;

    m_actSaveNoteAs = ac->addAction(KStandardAction::SaveAs, QStringLiteral("note_save_to_file"), this, &BNPView::saveNoteAs);
    m_actSaveNoteAs->setText(i18n("&Save to File..."));
    m_actionCollection->setDefaultShortcut(m_actSaveNoteAs, QKeySequence(QStringLiteral("F10")));

    a = ac->addAction(QStringLiteral("note_group"), this, &BNPView::noteGroup);
    a->setText(i18n("&Group"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("mail-attachment")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+G")));
    m_actGroup = a;

    a = ac->addAction(QStringLiteral("note_ungroup"), this, &BNPView::noteUngroup);
    a->setText(i18n("U&ngroup"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+G")));
    m_actUngroup = a;

    a = ac->addAction(QStringLiteral("note_move_top"), this, &BNPView::moveOnTop);
    a->setText(i18n("Move on &Top"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up-double")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+Home")));
    m_actMoveOnTop = a;

    a = ac->addAction(QStringLiteral("note_move_up"), this, &BNPView::moveNoteUp);
    a->setText(i18n("Move &Up"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+Up")));
    m_actMoveNoteUp = a;

    a = ac->addAction(QStringLiteral("note_move_down"), this, &BNPView::moveNoteDown);
    a->setText(i18n("Move &Down"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+Down")));
    m_actMoveNoteDown = a;

    a = ac->addAction(QStringLiteral("note_move_bottom"), this, &BNPView::moveOnBottom);
    a->setText(i18n("Move on &Bottom"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down-double")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+End")));
    m_actMoveOnBottom = a;

    m_actPaste = KStandardAction::paste(this, &BNPView::pasteInCurrentBasket, ac);

    /** Insert : **************************************************************/

#if 0
    a = ac->addAction("insert_text");
    a->setText(i18n("Plai&n Text"));
    a->setIcon(QIcon::fromTheme("text"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence("Ctrl+T"));
    m_actInsertText = a;
#endif

    a = ac->addAction(QStringLiteral("insert_html"));
    a->setText(i18n("&Text"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("text-html")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Insert")));
    m_actInsertHtml = a;

    a = ac->addAction(QStringLiteral("insert_link"));
    a->setText(i18n("&Link"));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::LINK)));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Y")));
    m_actInsertLink = a;

    a = ac->addAction(QStringLiteral("insert_cross_reference"));
    a->setText(i18n("Cross &Reference"));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::CROSS_REF)));
    m_actInsertCrossReference = a;

    a = ac->addAction(QStringLiteral("insert_image"));
    a->setText(i18n("&Image"));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::IMAGE)));
    m_actInsertImage = a;

    a = ac->addAction(QStringLiteral("insert_color"));
    a->setText(i18n("&Color"));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::COLOR)));
    m_actInsertColor = a;

    a = ac->addAction(QStringLiteral("insert_launcher"));
    a->setText(i18n("L&auncher"));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::LAUNCH)));
    m_actInsertLauncher = a;

    a = ac->addAction(QStringLiteral("insert_kmenu"));
    a->setText(i18n("Import Launcher for &desktop application..."));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::KMENU)));
    m_actImportKMenu = a;

    a = ac->addAction(QStringLiteral("insert_icon"));
    a->setText(i18n("Im&port Icon..."));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::ICONS)));
    m_actImportIcon = a;

    a = ac->addAction(QStringLiteral("insert_from_file"));
    a->setText(i18n("Load From &File..."));
    a->setIcon(QIcon::fromTheme(QString::fromUtf8(IconNames::DOCUMENT_IMPORT)));
    m_actLoadFile = a;

    //  connect( m_actInsertText, QAction::triggered, this, [this] () { insertEmpty(NoteType::Text); });
    connect(m_actInsertHtml, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::Html);
    });
    connect(m_actInsertImage, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::Image);
    });
    connect(m_actInsertLink, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::Link);
    });
    connect(m_actInsertCrossReference, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::CrossReference);
    });
    connect(m_actInsertColor, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::Color);
    });
    connect(m_actInsertLauncher, &QAction::triggered, this, [this]() {
        insertEmpty(NoteType::Launcher);
    });

    connect(m_actImportKMenu, &QAction::triggered, this, [this]() {
        insertWizard(1);
    });
    connect(m_actImportIcon, &QAction::triggered, this, [this]() {
        insertWizard(2);
    });
    connect(m_actLoadFile, &QAction::triggered, this, [this]() {
        insertWizard(3);
    });

#ifndef _WIN32
    a = ac->addAction(QStringLiteral("insert_screen_color"), this, &BNPView::slotColorFromScreen);
    a->setText(i18n("C&olor from Screen"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("kcolorchooser")));
    m_actColorPicker = a;

    connect(m_colorPicker, &ColorPicker::colorGrabbed, this, &BNPView::colorPicked);
#endif

    a = ac->addAction(QStringLiteral("insert_screen_capture"), this, &BNPView::grabScreenshot);
    a->setText(i18n("Grab Screen &Zone"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("ksnapshot")));
    m_actGrabScreenshot = a;

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
    auto *parentWidget = (QWidget *)parent();
    while (parentWidget) {
        if (parentWidget->inherits("MainWindow"))
            runInsideKontact = false;
        parentWidget = (QWidget *)parentWidget->parent();
    }

    // Use the "basket" icon in Kontact so it is consistent with the Kontact "New..." icon

    a = ac->addAction(QStringLiteral("basket_new"), this, qOverload<>(&BNPView::askNewBasket));
    a->setText(i18n("&New Basket..."));
    a->setIcon(QIcon::fromTheme((runInsideKontact ? QStringLiteral("basket") : QStringLiteral("document-new"))));
    m_actionCollection->setDefaultShortcuts(a, KStandardShortcut::shortcut(KStandardShortcut::New));
    actNewBasket = a;

    a = ac->addAction(QStringLiteral("basket_new_sub"), this, &BNPView::askNewSubBasket);
    a->setText(i18n("New &Sub-Basket..."));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    actNewSubBasket = a;

    a = ac->addAction(QStringLiteral("basket_new_sibling"), this, &BNPView::askNewSiblingBasket);
    a->setText(i18n("New Si&bling Basket..."));
    actNewSiblingBasket = a;

    auto *newBasketMenu = new KActionMenu(i18n("&New"), ac);
    newBasketMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    ac->addAction(QStringLiteral("basket_new_menu"), newBasketMenu);

    newBasketMenu->addAction(actNewBasket);
    newBasketMenu->addAction(actNewSubBasket);
    newBasketMenu->addAction(actNewSiblingBasket);
    connect(newBasketMenu, &QAction::triggered, this, qOverload<>(&BNPView::askNewBasket));

    a = ac->addAction(QStringLiteral("basket_properties"), this, &BNPView::propBasket);
    a->setText(i18n("&Properties..."));
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("F2")));
    m_actPropBasket = a;

    a = ac->addAction(QStringLiteral("basket_sort_children_asc"), this, &BNPView::sortChildrenAsc);
    a->setText(i18n("Sort Children Ascending"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-ascending")));
    m_actSortChildrenAsc = a;

    a = ac->addAction(QStringLiteral("basket_sort_children_desc"), this, &BNPView::sortChildrenDesc);
    a->setText(i18n("Sort Children Descending"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-descending")));
    m_actSortChildrenDesc = a;

    a = ac->addAction(QStringLiteral("basket_sort_siblings_asc"), this, &BNPView::sortSiblingsAsc);
    a->setText(i18n("Sort Siblings Ascending"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-ascending")));
    m_actSortSiblingsAsc = a;

    a = ac->addAction(QStringLiteral("basket_sort_siblings_desc"), this, &BNPView::sortSiblingsDesc);
    a->setText(i18n("Sort Siblings Descending"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-descending")));
    m_actSortSiblingsDesc = a;

    a = ac->addAction(QStringLiteral("basket_remove"), this, &BNPView::delBasket);
    a->setText(i18nc("Remove Basket", "&Remove"));
    a->setShortcut(0);
    m_actDelBasket = a;

#ifdef HAVE_LIBGPGME
    a = ac->addAction(QStringLiteral("basket_password"), this, &BNPView::password);
    a->setText(i18nc("Password protection", "Pass&word..."));
    a->setShortcut(0);
    m_actPassBasket = a;

    a = ac->addAction(QStringLiteral("basket_lock"), this, &BNPView::lockBasket);
    a->setText(i18nc("Lock Basket", "&Lock"));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+L")));
    m_actLockBasket = a;
#endif

    /** Edit : ****************************************************************/

    // m_actUndo = KStandardAction::undo(this, &BNPView::undo, actionCollection());
    // m_actUndo->setEnabled(false); // Not yet implemented !
    // m_actRedo = KStandardAction::redo(this, &BNPView::redo, actionCollection());
    // m_actRedo->setEnabled(false); // Not yet implemented !

    KToggleAction *toggleAct = nullptr;
    toggleAct = new KToggleAction(i18n("&Filter"), ac);
    ac->addAction(QStringLiteral("edit_filter"), toggleAct);
    toggleAct->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    m_actionCollection->setDefaultShortcuts(toggleAct, KStandardShortcut::shortcut(KStandardShortcut::Find));
    m_actShowFilter = toggleAct;

    connect(m_actShowFilter, &QAction::toggled, this, [this](bool checked) {
        showHideFilterBar(checked);
    });

    toggleAct = new KToggleAction(ac);
    ac->addAction(QStringLiteral("edit_filter_all_baskets"), toggleAct);
    toggleAct->setText(i18n("&Search All"));
    toggleAct->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
    m_actionCollection->setDefaultShortcut(toggleAct, QKeySequence(QStringLiteral("Ctrl+Shift+F")));
    m_actFilterAllBaskets = toggleAct;

    connect(m_actFilterAllBaskets, &KToggleAction::toggled, this, &BNPView::toggleFilterAllBaskets);

    a = ac->addAction(QStringLiteral("edit_filter_reset"), this, &BNPView::slotResetFilter);
    a->setText(i18n("&Reset Filter"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-locationbar-rtl")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+R")));
    m_actResetFilter = a;

    /** Go : ******************************************************************/

    a = ac->addAction(QStringLiteral("go_basket_previous"), this, &BNPView::goToPreviousBasket);
    a->setText(i18n("&Previous Basket"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Alt+Left")));
    m_actPreviousBasket = a;

    a = ac->addAction(QStringLiteral("go_basket_next"), this, &BNPView::goToNextBasket);
    a->setText(i18n("&Next Basket"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Alt+Right")));
    m_actNextBasket = a;

    a = ac->addAction(QStringLiteral("go_basket_fold"), this, &BNPView::foldBasket);
    a->setText(i18n("&Fold Basket"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Alt+Up")));
    m_actFoldBasket = a;

    a = ac->addAction(QStringLiteral("go_basket_expand"), this, &BNPView::expandBasket);
    a->setText(i18n("&Expand Basket"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    m_actionCollection->setDefaultShortcut(a, QKeySequence(QStringLiteral("Alt+Down")));
    m_actExpandBasket = a;

#if 0
    // FOR_BETA_PURPOSE:
    a = ac->addAction("beta_convert_texts", this, &BNPView::convertTexts);
    a->setText(i18n("Convert text notes to rich text notes"));
    a->setIcon(QIcon::fromTheme("run-build-file"));
    m_convertTexts = a;
#endif

    InlineEditors::instance()->initToolBars(actionCollection());
    /** Help : ****************************************************************/

    a = ac->addAction(QStringLiteral("help_welcome_baskets"), this, &BNPView::addWelcomeBaskets);
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
        menuName = QStringLiteral("basket_popup");
    } else {
        menuName = QStringLiteral("tab_bar_popup");
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
    DEBUG_WIN << QStringLiteral("Basket Tree: Saving...");

    QString data;
    QXmlStreamWriter stream(&data);
    XMLWork::setupXmlStream(stream, QStringLiteral("basketTree"));

    // Save Basket Tree:
    save(m_tree, nullptr, stream);

    stream.writeEndElement();
    stream.writeEndDocument();

    // Write to Disk:
    FileStorage::safelySaveToFile(Global::basketsFolder() + QStringLiteral("baskets.xml"), data);

    GitWrapper::commitBasketView();
}

void BNPView::save(QTreeWidget *listView, QTreeWidgetItem *item, QXmlStreamWriter &stream)
{
    if (item == nullptr) {
        if (listView == nullptr) {
            // This should not happen: we call either save(listView, 0) or save(0, item)
            DEBUG_WIN << QStringLiteral("BNPView::save error: listView=NULL and item=NULL");
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
    QScopedPointer<QDomDocument> doc(XMLWork::openFile(QStringLiteral("basketTree"), Global::basketsFolder() + QStringLiteral("baskets.xml")));
    // BEGIN Compatibility with 0.6.0 Pre-Alpha versions:
    if (!doc)
        doc.reset(XMLWork::openFile(QStringLiteral("basketsTree"), Global::basketsFolder() + QStringLiteral("baskets.xml")));
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
        if ((!element.isNull()) && element.tagName() == QStringLiteral("basket")) {
            QString folderName = element.attribute(QStringLiteral("folderName"));
            if (!folderName.isEmpty()) {
                BasketScene *basket = loadBasket(folderName);
                BasketListViewItem *basketItem = appendBasket(basket, item);
                basketItem->setExpanded(!XMLWork::trueOrFalse(element.attribute(QStringLiteral("folded"), QStringLiteral("false")), false));
                basket->loadProperties(XMLWork::getElement(element, QStringLiteral("properties")));
                if (XMLWork::trueOrFalse(
                        element.attribute(QStringLiteral("lastOpened"), element.attribute(QStringLiteral("lastOpened"), QStringLiteral("false"))),
                        false)) // Compat with 0.6.0-Alphas
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

    auto *decoBasket = new DecoratedBasket(m_stack, folderName);
    BasketScene *basket = decoBasket->basket();
    m_stack->addWidget(decoBasket);

    connect(this, &BNPView::showErrorMessage, decoBasket, &DecoratedBasket::showErrorMessage);
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

    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::KeyboardModifiers());
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::expandBasket()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::KeyboardModifiers());
    QApplication::postEvent(m_tree, keyEvent);
}

void BNPView::closeAllEditors()
{
    QTreeWidgetItemIterator it(m_tree);
    while (*it) {
        auto *item = (BasketListViewItem *)(*it);
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
        auto *item = (BasketListViewItem *)(*it);
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
        QTimer::singleShot(0, this, &BNPView::newFilter); // Keep time for the QLineEdit to display the filtered character and refresh correctly!
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
    auto *decoBasket = (DecoratedBasket *)m_stack->currentWidget();
    if (decoBasket)
        return decoBasket->basket();
    else
        return nullptr;
}

BasketScene *BNPView::parentBasketOf(BasketScene *basket)
{
    auto *item = (BasketListViewItem *)(listViewItemForBasket(basket)->parent());
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
    if (m_tree->currentItem() != nullptr && currentBasket() == basket)
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
    auto *nextBasketItem = (BasketListViewItem *)(m_tree->itemBelow(basketItem));
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
    } else { // No need to save two times if we add a basket
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
        auto *item = static_cast<BasketListViewItem *>(*it);
        auto *decoration = static_cast<DecoratedBasket *>(item->basket()->parent());
        decoration->setFilterBarPosition(onTop);
        ++it;
    }
}

void BNPView::updateBasketListViewItem(BasketScene *basket)
{
    if (basket == nullptr) {
        return;
    }

    BasketListViewItem *item = listViewItemForBasket(basket);
    if (item != nullptr) {
        item->setup();

        if (basket == currentBasket()) {
            setWindowTitle(basket->basketName());
        }

        if (basket->backgroundColor().isValid()) {
            item->setBackground(0, QBrush(basket->backgroundColor()));
        } else {
            item->setBackground(0, QBrush());
        }
    }

    // Don't save if we are loading!
    if (!m_loading) {
        save();
    }
}

void BNPView::needSave(QTreeWidgetItem *)
{
    if (!m_loading)
        // A basket has been collapsed/expanded or a new one is select: this is not urgent:
        QTimer::singleShot(500ms, this, qOverload<>(&BNPView::save));
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

void BNPView::clearFormattingNote()
{
    currentBasket()->clearFormattingNote();
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
                DEBUG_WIN << QStringLiteral("<font color='red'>") + noteFileName + QStringLiteral(" NOT FOUND!</font>");
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
        DEBUG_WIN << QStringLiteral("<font color='red'>") + basketFolderName + QStringLiteral(" NOT FOUND!</font>");
    } else {
        dirList.removeAt(basketFolderIndex);
    }
    int basketFileIndex = fileList.indexOf(basket->folderName() + QStringLiteral(".basket"));
    if (basketFileIndex < 0) {
        DEBUG_WIN << QStringLiteral("<font color='red'>.basket file of ") + basketFolderName + QStringLiteral(".basket NOT FOUND!</font>");
    } else {
        fileList.removeAt(basketFileIndex);
    }
    if (!basket->loadingLaunched() && !basket->isLocked()) {
        basket->load();
    }
    DEBUG_WIN << QStringLiteral("\t********************************************************************************");
    DEBUG_WIN << basket->basketName() << QStringLiteral("(") << basketFolderName << QStringLiteral(") loaded.");
    Note *note = basket->firstNote();
    if (!note) {
        DEBUG_WIN << QStringLiteral("\tHas NO notes!");
    } else {
        checkNote(note, fileList);
    }
    basket->save();
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
    for (int i = 0; i < item->childCount(); i++) {
        checkBasket((BasketListViewItem *)item->child(i), dirList, fileList);
    }
    if (basket != Global::bnpView->currentBasket()) {
        DEBUG_WIN << basket->basketName() << QStringLiteral("(") << basketFolderName << QStringLiteral(") unloading...");
        DEBUG_WIN << QStringLiteral("\t********************************************************************************");
        basket->unbufferizeAll();
    } else {
        DEBUG_WIN << basket->basketName() << QStringLiteral("(") << basketFolderName << QStringLiteral(") is the current basket, not unloading.");
        DEBUG_WIN << QStringLiteral("\t********************************************************************************");
    }
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents, 100);
}

void BNPView::checkCleanup()
{
    DEBUG_WIN << QStringLiteral("Starting the check, cleanup and reindexing... (") + Global::basketsFolder() + QLatin1Char(')');
    QList<QString> dirList;
    QList<QString> fileList;
    QString topDirEntry;
    QString subDirEntry;
    QFileInfo fileInfo;
    QDir topDir(Global::basketsFolder(), QString(), QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
    for (QString topDirEntry : topDir.entryList()) {
        if (topDirEntry != QStringLiteral(".") && topDirEntry != QStringLiteral("..")) {
            fileInfo.setFile(Global::basketsFolder() + QLatin1Char('/') + topDirEntry);
            if (fileInfo.isDir()) {
                dirList << topDirEntry + QLatin1Char('/');
                QDir basketDir(Global::basketsFolder() + QLatin1Char('/') + topDirEntry,
                               QString(),
                               QDir::Name | QDir::IgnoreCase,
                               QDir::TypeMask | QDir::Hidden);
                for (QString subDirEntry : basketDir.entryList()) {
                    if (subDirEntry != QStringLiteral(".") && subDirEntry != QStringLiteral("..")) {
                        fileList << topDirEntry + QLatin1Char('/') + subDirEntry;
                    }
                }
            } else if (topDirEntry != QStringLiteral(".") && topDirEntry != QStringLiteral("..") && topDirEntry != QStringLiteral("baskets.xml")) {
                fileList << topDirEntry;
            }
        }
    }
    DEBUG_WIN << QStringLiteral("Directories found: ") + QString::number(dirList.count());
    DEBUG_WIN << QStringLiteral("Files found: ") + QString::number(fileList.count());

    DEBUG_WIN << QStringLiteral("Checking Baskets:");
    for (int i = 0; i < topLevelItemCount(); i++) {
        checkBasket(topLevelItem(i), dirList, fileList);
    }
    DEBUG_WIN << QStringLiteral("Baskets checked.");
    DEBUG_WIN << QStringLiteral("Directories remaining (not in any basket): ") + QString::number(dirList.count());
    DEBUG_WIN << QStringLiteral("Files remaining (not in any basket): ") + QString::number(fileList.count());

    for (QString topDirEntry : dirList) {
        DEBUG_WIN << QStringLiteral("<font color='red'>") + topDirEntry + QStringLiteral(" does not belong to any basket!</font>");
        // Tools::deleteRecursively(Global::basketsFolder() + '/' + topDirEntry);
        // DEBUG_WIN << "<font color='red'>\t" + topDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + QStringLiteral("/") + topDirEntry);
        DEBUG_WIN << QStringLiteral("<font color='red'>\t") + topDirEntry + QStringLiteral(" trashed!</font>");
        for (QString subDirEntry : fileList) {
            fileInfo.setFile(Global::basketsFolder() + QLatin1Char('/') + subDirEntry);
            if (!fileInfo.isFile()) {
                fileList.removeAll(subDirEntry);
                DEBUG_WIN << QStringLiteral("<font color='red'>\t\t") + subDirEntry + QStringLiteral(" already removed!</font>");
            }
        }
    }
    for (QString subDirEntry : fileList) {
        DEBUG_WIN << QStringLiteral("<font color='red'>") + subDirEntry + QStringLiteral(" does not belong to any note!</font>");
        // Tools::deleteRecursively(Global::basketsFolder() + '/' + subDirEntry);
        // DEBUG_WIN << "<font color='red'>\t" + subDirEntry + " removed!</font>";
        Tools::trashRecursively(Global::basketsFolder() + QLatin1Char('/') + subDirEntry);
        DEBUG_WIN << QStringLiteral("<font color='red'>\t") + subDirEntry + QStringLiteral(" trashed!</font>");
    }
    DEBUG_WIN << QStringLiteral("Check, cleanup and reindexing completed");
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

    NoteType::Id selectedType = NoteType::Unknown;
    if (oneSelected && currentBasket()->theSelectedNote()) {
        selectedType = currentBasket()->theSelectedNote()->content()->type();
    }
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
    m_actClearFormatting->setEnabled(!isLocked && !currentBasket()->isDuringEdit() && selectedType == NoteType::Html);
    m_actOpenNote->setEnabled(oneOrSeveralSelected);
    m_actOpenNoteWith->setEnabled(oneSelected); // TODO: oneOrSeveralSelected IF SAME TYPE
    m_actSaveNoteAs->setEnabled(oneSelected); // IDEM?
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
                if (url().toDisplayString().isEmpty() && serviceLauncher().isEmpty())     // no URL nor serviceLauncher :
        popupMenu->setItemEnabled(7, false);                       //  no possible Open !
                if (url().toDisplayString().isEmpty())                               // no URL :
        popupMenu->setItemEnabled(8, false);                       //  no possible Open with !
                if (url().toDisplayString().isEmpty() || url().path().endsWith("/")) // no URL or target a folder :
        popupMenu->setItemEnabled(9, false);                       //  not possible to save target file
    }
    } else if (m_type != Color) {
        popupMenu->insertSeparator();
        popupMenu->insertItem(QIcon::fromTheme("document-save-as"), i18n("&Save a copy as..."), this, &BNPView::slotSaveAs, 0, 10);
    }*/
}

// BEGIN Color picker (code from KColorEdit):

/* Activate the mode
 */
void BNPView::slotColorFromScreen(bool global)
{
#ifndef _WIN32
    m_colorPickWasGlobal = global;

    currentBasket()->saveInsertionData();
    m_colorPicker->grabColor();
#endif
}

void BNPView::slotColorFromScreenGlobal()
{
    slotColorFromScreen(true);
}

void BNPView::colorPicked(const QColor &color)
{
    if (!currentBasket()->isLoaded()) {
        currentBasket()->load();
    }
    currentBasket()->insertColor(color);
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
        KXMLGUIFactory *factory = m_guiClient->factory();
        if (factory) {
            menu = (QMenu *)factory->container(menuName, m_guiClient);
        }
    }
    if (menu == nullptr) {
        QString basketDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/basket/");

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
        return;
    }
    currentBasket()->insertEmptyNote(type);
}

void BNPView::insertWizard(int type)
{
    if (currentBasket()->isLocked()) {
        Q_EMIT showErrorMessage(i18n("Cannot add note."));
        return;
    }
    currentBasket()->insertWizard(type);
}

// BEGIN Screen Grabbing:
void BNPView::grabScreenshot(bool global)
{
    if (m_regionGrabber) {
        KWindowSystem::activateWindow(m_regionGrabber->windowHandle());
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
        currentBasket()->load();
    }
    currentBasket()->insertImage(pixmap);

    if (m_colorPickWasShown)
        showMainWindow();
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
    if (!name.endsWith(QLatin1Char('/')))
        name += QLatin1Char('/');

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

    int really = KMessageBox::questionTwoActions(
        this,
        i18n("<qt>Do you really want to remove the basket <b>%1</b> and its contents?</qt>", Tools::textToHTMLWithoutP(basket->basketName())),
        i18n("Remove Basket"),
        KGuiItem(i18n("&Remove Basket"), QStringLiteral("edit-delete")),
        KStandardGuiItem::cancel());

    if (really == KMessageBox::Ok)
        return;

    QStringList basketsList = listViewItemForBasket(basket)->childNamesTree(0);
    if (basketsList.count() > 0) {
        int deleteChilds = KMessageBox::questionTwoActionsList(
            this,
            i18n("<qt><b>%1</b> has the following children baskets.<br>Do you want to remove them too?</qt>", Tools::textToHTMLWithoutP(basket->basketName())),
            basketsList,
            i18n("Remove Children Baskets"),
            KGuiItem(i18n("&Remove Children Baskets"), QStringLiteral("edit-delete")),
            KStandardGuiItem::cancel());

        if (deleteChilds == KMessageBox::Cancel)
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

    KConfigGroup config = KSharedConfig::openConfig()->group(QStringLiteral("Basket Archive"));
    QString folder = config.readEntry("lastFolder", QDir::homePath()) + QStringLiteral("/");
    QString url = folder + QString(basket->basketName()).replace(QLatin1Char('/'), QLatin1Char('_')) + QStringLiteral(".baskets");

    QString filter = QStringLiteral("*.baskets|") + i18n("Basket Archives") + QStringLiteral("\n*|") + i18n("All Files");
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        destination = QFileDialog::getSaveFileName(nullptr, i18n("Save as Basket Archive"), destination, filter);
        if (destination.isEmpty()) // User canceled
            return;
        if (dir.exists(destination)) {
            int result = KMessageBox::questionTwoActionsCancel(
                this,
                QStringLiteral("<qt>")
                    + i18n("The file <b>%1</b> already exists. Do you really want to overwrite it?", QUrl::fromLocalFile(destination).fileName()),
                i18n("Overwrite File?"),
                KGuiItem(i18n("&Overwrite"), QStringLiteral("document-save")),
                KStandardGuiItem::discard());
            if (result == KMessageBox::Cancel)
                return;
            else if (result == KMessageBox::Ok)
                askAgain = false;
        } else
            askAgain = false;
    }
    bool withSubBaskets =
        true; // KMessageBox::questionYesNo(this, i18n("Do you want to export sub-baskets too?"), i18n("Save as Basket Archive")) == KMessageBox::Yes;

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
}

void BNPView::pasteSelInCurrentBasket()
{
    currentBasket()->pasteNote(QClipboard::Selection);
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
    QTimer::singleShot(0, this, &BNPView::cancelNewBasketPopup);
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
    auto *noteContent = (HtmlContent *)note->content();
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
    QString customDataFolder = parser->value(QStringLiteral("data-folder"));
    if (!customDataFolder.isNull() && !customDataFolder.isEmpty()) {
        Global::setCustomSavesFolder(customDataFolder);
    }
    /* Debug window */
    if (parser->isSet(QStringLiteral("debug"))) {
        new DebugWindow();
        Global::debugWindow->show();
    }
}

void BNPView::reloadBasket(const QString &folderName)
{
    basketForFolderName(folderName)->reload();
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
    QMenu *menu = (QMenu *)(popupMenu(QStringLiteral("tags")));
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
    // connect(menu, &QMenu::aboutToHide, this, &BNPView::disconnectTagsMenu);
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

        auto *mi = new StateAction(currentState, QKeySequence(sequence), this, true);
        mi->setShortcutContext(Qt::WidgetShortcut);

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
    auto *act = new QAction(i18n("&Assign new Tag..."), &menu);
    act->setData(1);
    act->setEnabled(enable);
    menu.addAction(act);

    act = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("&Remove All"), &menu);
    act->setData(2);
    if (!currentBasket()->selectedNotesHaveTags())
        act->setEnabled(false);
    menu.addAction(act);

    act = new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("&Customize..."), &menu);
    act->setData(3);
    menu.addAction(act);

    connect(&menu, &QMenu::triggered, currentBasket(), &BasketScene::toggledTagInMenu);
    connect(&menu, &QMenu::aboutToHide, currentBasket(), &BasketScene::unlockHovering);
    connect(&menu, &QMenu::aboutToHide, currentBasket(), &BasketScene::disableNextClick);
}

void BNPView::connectTagsMenu()
{
    connect(popupMenu(QStringLiteral("tags")), &QMenu::aboutToShow, this, [this]() {
        this->populateTagsMenu();
    });
    connect(popupMenu(QStringLiteral("tags")), &QMenu::aboutToHide, this, &BNPView::disconnectTagsMenu);
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
    QTimer::singleShot(0, this, &BNPView::disconnectTagsMenuDelayed);
}

void BNPView::disconnectTagsMenuDelayed()
{
    m_lastOpenedTagsMenu->disconnect(currentBasket());
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
    if (pages.isEmpty())
        return {};
    QString page = QUrl::fromPercentEncoding(pages.takeFirst().toUtf8()).trimmed();

    if (page == QStringLiteral("..")) {
        QTreeWidgetItem *p = parent ? parent->parent() : m_tree->currentItem()->parent();
        return this->folderFromBasketNameLink(pages, p);
    }

    if (page == QStringLiteral(".")) {
        parent = parent ? parent : m_tree->currentItem();
        return this->folderFromBasketNameLink(pages, parent);
    }

    if (!parent && page.isEmpty()) {
        parent = m_tree->invisibleRootItem();
        return this->folderFromBasketNameLink(pages, parent);
    }

    parent = parent ? parent : m_tree->currentItem();
    QRegularExpression re(QStringLiteral(":\\{([0-9]+)\\}"));
    int pos = 0;

    pos = page.indexOf(re, pos);
    int basketNum = 1;

    QRegularExpressionMatch m = re.match(page);
    if (pos != -1) {
        basketNum = m.captured(1).toInt();
    }

    page = page.left(page.length() - m.capturedLength());

    for (int i = 0; i < parent->childCount(); i++) {
        QTreeWidgetItem *child = parent->child(i);

        if (child->text(0).toLower() == page.toLower()) {
            basketNum--;
            if (basketNum == 0) {
                if (pages.count() > 0) {
                    return this->folderFromBasketNameLink(pages, child);
                } else {
                    return ((BasketListViewItem *)child)->basket()->folderName();
                }
            }
        }
    }

    return {};
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

#include "moc_bnpview.cpp"
