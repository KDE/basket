/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settings.h"

#include <QCheckBox>
#include <QDate>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPluginLoader>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <KConfig>
#include <KConfigGroup>
#include <KIO/Global>
#include <KLocalizedString>
#include <KPluginMetaData>
#include <QProcess>
#include <kcmutils_version.h>

#include "aboutdata.h"
#include "basketscene.h"
#include "kgpgme.h"
#include "linklabel.h"
#include "variouswidgets.h"

/** Settings */

// General:                                      // TODO: Use this grouping everywhere!
bool Settings::s_showNotesToolTip = true; // TODO: RENAME: useBasketTooltips
bool Settings::s_confirmNoteDeletion = true;
bool Settings::s_bigNotes = false;
bool Settings::s_autoBullet = true;
bool Settings::s_pasteAsPlainText = false;
bool Settings::s_exportTextTags = true;
bool Settings::s_detectTextTags = true;
bool Settings::s_useGnuPGAgent = false;
bool Settings::s_treeOnLeft = true;
bool Settings::s_filterOnTop = false;
int Settings::s_defImageX = 300;
int Settings::s_defImageY = 200;
bool Settings::s_enableReLockTimeout = true;
int Settings::s_reLockTimeoutMinutes = 0;
int Settings::s_newNotesPlace = 1;
int Settings::s_viewTextFileContent = false;
int Settings::s_viewHtmlFileContent = false;
int Settings::s_viewImageFileContent = false;
int Settings::s_viewSoundFileContent = false;
// Applications:
bool Settings::s_htmlUseProg = false; // TODO: RENAME: s_*App (with KService!)
bool Settings::s_imageUseProg = true;
bool Settings::s_animationUseProg = true;
bool Settings::s_soundUseProg = false;
bool Settings::s_linkUseProg = false;
QString Settings::s_htmlProg = QStringLiteral("quanta");
QString Settings::s_imageProg = QStringLiteral("kolourpaint");
QString Settings::s_animationProg = QStringLiteral("gimp");
QString Settings::s_soundProg = QString();
QString Settings::s_linkProg = QString();
// Addictive Features:
bool Settings::s_groupOnInsertionLine = false;
int Settings::s_middleAction = 0;
// Rememberings:
int Settings::s_defIconSize = 32; // TODO: RENAME: importIconSize
bool Settings::s_blinkedFilter = false;
int Settings::s_basketTreeWidth = -1;
bool Settings::s_welcomeBasketsAdded = false;
QString Settings::s_dataFolder = QString();
QDate Settings::s_lastBackup = QDate();
QPoint Settings::s_mainWindowPosition = QPoint();
QSize Settings::s_mainWindowSize = QSize();
bool Settings::s_showEmptyBasketInfo = true;
bool Settings::s_spellCheckTextNotes = true;
// Version Sync
bool Settings::s_versionSyncEnabled = false;

void Settings::loadConfig()
{
    DEBUG_WIN << QStringLiteral("Load Configuration");
    LinkLook defaultSoundLook;
    LinkLook defaultFileLook;
    LinkLook defaultLocalLinkLook;
    LinkLook defaultNetworkLinkLook;
    LinkLook defaultLauncherLook; /* italic  bold    underlining                color      hoverColor  iconSize  preview */
    LinkLook defaultCrossReferenceLook;

    defaultSoundLook.setLook(false, false, LinkLook::Never, QColor(), QColor(), 32, LinkLook::None);
    defaultFileLook.setLook(false, false, LinkLook::Never, QColor(), QColor(), 32, LinkLook::TwiceIconSize);
    defaultLocalLinkLook.setLook(true, false, LinkLook::OnMouseHover, QColor(), QColor(), 22, LinkLook::TwiceIconSize);
    defaultNetworkLinkLook.setLook(false, false, LinkLook::OnMouseOutside, QColor(), QColor(), 16, LinkLook::None);
    defaultLauncherLook.setLook(false, true, LinkLook::Never, QColor(), QColor(), 48, LinkLook::None);
    defaultCrossReferenceLook.setLook(false, false, LinkLook::OnMouseHover, QColor(), QColor(), 16, LinkLook::None);

    loadLinkLook(LinkLook::soundLook, QStringLiteral("Sound Look"), defaultSoundLook);
    loadLinkLook(LinkLook::fileLook, QStringLiteral("File Look"), defaultFileLook);
    loadLinkLook(LinkLook::localLinkLook, QStringLiteral("Local Link Look"), defaultLocalLinkLook);
    loadLinkLook(LinkLook::networkLinkLook, QStringLiteral("Network Link Look"), defaultNetworkLinkLook);
    loadLinkLook(LinkLook::launcherLook, QStringLiteral("Launcher Look"), defaultLauncherLook);
    loadLinkLook(LinkLook::crossReferenceLook, QStringLiteral("Cross Reference Look"), defaultCrossReferenceLook);

    KConfigGroup config = Global::config()->group(QStringLiteral("Main window")); // TODO: Split with a "System tray icon" group !
    setTreeOnLeft(config.readEntry("treeOnLeft", true));
    setFilterOnTop(config.readEntry("filterOnTop", false));
    setShowNotesToolTip(config.readEntry("showNotesToolTip", true));
    setBigNotes(config.readEntry("bigNotes", false));
    setConfirmNoteDeletion(config.readEntry("confirmNoteDeletion", true));
    setPasteAsPlainText(config.readEntry("pasteAsPlainText", false));
    setAutoBullet(config.readEntry("autoBullet", true));
    setExportTextTags(config.readEntry("exportTextTags", true));
    setDetectTextTags(config.readEntry("detectTextTags", true));
    setUseGnuPGAgent(config.readEntry("useGnuPGAgent", false));
    setBlinkedFilter(config.readEntry("blinkedFilter", false));
    setEnableReLockTimeout(config.readEntry("enableReLockTimeout", true));
    setReLockTimeoutMinutes(config.readEntry("reLockTimeoutMinutes", 0));
    setMiddleAction(config.readEntry("middleAction", 0));
    setGroupOnInsertionLine(config.readEntry("groupOnInsertionLine", false));
    setSpellCheckTextNotes(config.readEntry("spellCheckTextNotes", true));
    setBasketTreeWidth(config.readEntry("basketTreeWidth", -1));
    setWelcomeBasketsAdded(config.readEntry("welcomeBasketsAdded", false));
    setDataFolder(config.readEntry("dataFolder", QString()));
    setLastBackup(config.readEntry("lastBackup", QDate()));
    setMainWindowPosition(config.readEntry("position", QPoint()));
    setMainWindowSize(config.readEntry("size", QSize()));

    config = Global::config()->group(QStringLiteral("Notification Messages"));
    setShowEmptyBasketInfo(config.readEntry("emptyBasketInfo", true));

    config = Global::config()->group(QStringLiteral("Programs"));
    setIsHtmlUseProg(config.readEntry("htmlUseProg", false));
    setIsImageUseProg(config.readEntry("imageUseProg", true));
    setIsAnimationUseProg(config.readEntry("animationUseProg", true));
    setIsSoundUseProg(config.readEntry("soundUseProg", false));
    setIsLinkUseProg(config.readEntry("linkUseProg", false));
    setHtmlProg(config.readEntry("htmlProg", "org.kde.kate"));
    setImageProg(config.readEntry("imageProg", "org.kde.krita"));
    setAnimationProg(config.readEntry("animationProg", "glaxnimate"));
    setSoundProg(config.readEntry("soundProg", "org.kde.kwave"));
    setLinkProg(config.readEntry("linkProg", "org.kde.falkon"));

    config = Global::config()->group(QStringLiteral("Note Addition"));
    setNewNotesPlace(config.readEntry("newNotesPlace", 1));
    setViewTextFileContent(config.readEntry("viewTextFileContent", false));
    setViewHtmlFileContent(config.readEntry("viewHtmlFileContent", false));
    setViewImageFileContent(config.readEntry("viewImageFileContent", true));
    setViewSoundFileContent(config.readEntry("viewSoundFileContent", true));

    config = Global::config()->group(QStringLiteral("Insert Note Default Values"));
    setDefImageX(config.readEntry("defImageX", 300));
    setDefImageY(config.readEntry("defImageY", 200));
    setDefIconSize(config.readEntry("defIconSize", 32));

    config = Global::config()->group(QStringLiteral("MainWindow Toolbar mainToolBar"));
    // The first time we start, we define "Text Alongside Icons" for the main toolbar.
    // After that, the user is free to hide the text from the icons or customize as he/she want.
    // But it is a good default (Fitt's Laws, better looking, less "empty"-feeling), especially for this application.
    //  if (!config->readEntry("alreadySetIconTextRight", false)) {
    //      config->writeEntry( "IconText",                "IconTextRight" );
    //      config->writeEntry( "alreadySetIconTextRight", true            );
    //  }
    if (!config.readEntry("alreadySetToolbarSettings", false)) {
        config.writeEntry("IconText", "IconOnly"); // In 0.6.0 Alpha versions, it was made "IconTextRight". We're back to IconOnly
        config.writeEntry("Index", "0"); // Make sure the main toolbar is the first...
        config = Global::config()->group(QStringLiteral("MainWindow Toolbar richTextEditToolBar"));
        config.writeEntry("Position", "Top"); // In 0.6.0 Alpha versions, it was made "Bottom"
        config.writeEntry("Index", "1"); // ... and the rich text toolbar is on the right of the main toolbar
        config = Global::config()->group(QStringLiteral("MainWindow Toolbar mainToolBar"));
        config.writeEntry("alreadySetToolbarSettings", true);
    }

    config = Global::config()->group(QStringLiteral("Version Sync"));
    setVersionSyncEnabled(config.readEntry("enabled", false));
}

void Settings::saveConfig()
{
    DEBUG_WIN << QStringLiteral("Save Configuration");
    saveLinkLook(LinkLook::soundLook, QStringLiteral("Sound Look"));
    saveLinkLook(LinkLook::fileLook, QStringLiteral("File Look"));
    saveLinkLook(LinkLook::localLinkLook, QStringLiteral("Local Link Look"));
    saveLinkLook(LinkLook::networkLinkLook, QStringLiteral("Network Link Look"));
    saveLinkLook(LinkLook::launcherLook, QStringLiteral("Launcher Look"));
    saveLinkLook(LinkLook::crossReferenceLook, QStringLiteral("Cross Reference Look"));

    KConfigGroup config = Global::config()->group(QStringLiteral("Main window"));
    config.writeEntry("treeOnLeft", treeOnLeft());
    config.writeEntry("filterOnTop", filterOnTop());
    config.writeEntry("showNotesToolTip", showNotesToolTip());
    config.writeEntry("confirmNoteDeletion", confirmNoteDeletion());
    config.writeEntry("pasteAsPlainText", pasteAsPlainText());
    config.writeEntry("bigNotes", bigNotes());
    config.writeEntry("autoBullet", autoBullet());
    config.writeEntry("exportTextTags", exportTextTags());
    config.writeEntry("detectTextTags", detectTextTags());
#ifdef HAVE_LIBGPGME
    if (KGpgMe::isGnuPGAgentAvailable())
        config.writeEntry("useGnuPGAgent", useGnuPGAgent());
#endif
    config.writeEntry("blinkedFilter", blinkedFilter());
    config.writeEntry("enableReLockTimeout", enableReLockTimeout());
    config.writeEntry("reLockTimeoutMinutes", reLockTimeoutMinutes());
    config.writeEntry("middleAction", middleAction());
    config.writeEntry("groupOnInsertionLine", groupOnInsertionLine());
    config.writeEntry("spellCheckTextNotes", spellCheckTextNotes());
    config.writeEntry("basketTreeWidth", basketTreeWidth());
    config.writeEntry("welcomeBasketsAdded", welcomeBasketsAdded());
    config.writePathEntry("dataFolder", dataFolder());
    config.writeEntry("lastBackup", QDate(lastBackup()));
    config.writeEntry("position", mainWindowPosition());
    config.writeEntry("size", mainWindowSize());

    config = Global::config()->group(QStringLiteral("Notification Messages"));
    config.writeEntry("emptyBasketInfo", showEmptyBasketInfo());

    config = Global::config()->group(QStringLiteral("Programs"));
    config.writeEntry("htmlUseProg", isHtmlUseProg());
    config.writeEntry("imageUseProg", isImageUseProg());
    config.writeEntry("animationUseProg", isAnimationUseProg());
    config.writeEntry("soundUseProg", isSoundUseProg());
    config.writeEntry("linkUseProg", isLinkUseProg());
    config.writeEntry("htmlProg", htmlProg());
    config.writeEntry("imageProg", imageProg());
    config.writeEntry("animationProg", animationProg());
    config.writeEntry("soundProg", soundProg());
    config.writeEntry("linkProg", linkProg());

    config = Global::config()->group(QStringLiteral("Note Addition"));
    config.writeEntry("newNotesPlace", newNotesPlace());
    config.writeEntry("viewTextFileContent", viewTextFileContent());
    config.writeEntry("viewHtmlFileContent", viewHtmlFileContent());
    config.writeEntry("viewImageFileContent", viewImageFileContent());
    config.writeEntry("viewSoundFileContent", viewSoundFileContent());

    config = Global::config()->group(QStringLiteral("Insert Note Default Values"));
    config.writeEntry("defImageX", defImageX());
    config.writeEntry("defImageY", defImageY());
    config.writeEntry("defIconSize", defIconSize());

    config = Global::config()->group(QStringLiteral("Version Sync"));
    config.writeEntry("enabled", versionSyncEnabled());

    config.sync();
}

void Settings::loadLinkLook(LinkLook *look, const QString &name, const LinkLook &defaultLook)
{
    KConfigGroup config = Global::config()->group(name);

    QString underliningStrings[] = {QStringLiteral("Always"), QStringLiteral("Never"), QStringLiteral("OnMouseHover"), QStringLiteral("OnMouseOutside")};
    QString defaultUnderliningString = underliningStrings[defaultLook.underlining()];

    QString previewStrings[] = {QStringLiteral("None"), QStringLiteral("IconSize"), QStringLiteral("TwiceIconSize"), QStringLiteral("ThreeIconSize")};
    QString defaultPreviewString = previewStrings[defaultLook.preview()];

    bool italic = config.readEntry(QStringLiteral("italic"), defaultLook.italic());
    bool bold = config.readEntry(QStringLiteral("bold"), defaultLook.bold());
    QString underliningString = config.readEntry(QStringLiteral("underlining"), defaultUnderliningString);
    QColor color = config.readEntry(QStringLiteral("color"), defaultLook.color());
    QColor hoverColor = config.readEntry(QStringLiteral("hoverColor"), defaultLook.hoverColor());
    int iconSize = config.readEntry(QStringLiteral("iconSize"), defaultLook.iconSize());
    QString previewString = config.readEntry(QStringLiteral("preview"), defaultPreviewString);

    int underlining = 0;
    if (underliningString == underliningStrings[1])
        underlining = 1;
    else if (underliningString == underliningStrings[2])
        underlining = 2;
    else if (underliningString == underliningStrings[3])
        underlining = 3;

    int preview = 0;
    if (previewString == previewStrings[1])
        preview = 1;
    else if (previewString == previewStrings[2])
        preview = 2;
    else if (previewString == previewStrings[3])
        preview = 3;

    look->setLook(italic, bold, underlining, color, hoverColor, iconSize, preview);
}

void Settings::saveLinkLook(LinkLook *look, const QString &name)
{
    KConfigGroup config = Global::config()->group(name);

    QString underliningStrings[] = {QStringLiteral("Always"), QStringLiteral("Never"), QStringLiteral("OnMouseHover"), QStringLiteral("OnMouseOutside")};
    QString underliningString = underliningStrings[look->underlining()];

    QString previewStrings[] = {QStringLiteral("None"), QStringLiteral("IconSize"), QStringLiteral("TwiceIconSize"), QStringLiteral("ThreeIconSize")};
    QString previewString = previewStrings[look->preview()];

    config.writeEntry("italic", look->italic());
    config.writeEntry("bold", look->bold());
    config.writeEntry("underlining", underliningString);
    config.writeEntry("color", look->color());
    config.writeEntry("hoverColor", look->hoverColor());
    config.writeEntry("iconSize", look->iconSize());
    config.writeEntry("preview", previewString);
}

void Settings::setBigNotes(bool big)
{
    if (big == s_bigNotes)
        return;

    s_bigNotes = big;
    // Big notes for accessibility reasons OR Standard small notes:
    Note::NOTE_MARGIN = (big ? 4 : 2);
    Note::INSERTION_HEIGHT = (big ? 5 : 3);
    Note::EXPANDER_WIDTH = 9;
    Note::EXPANDER_HEIGHT = 9;
    Note::GROUP_WIDTH = 2 * Note::NOTE_MARGIN + Note::EXPANDER_WIDTH;
    Note::HANDLE_WIDTH = Note::GROUP_WIDTH;
    Note::RESIZER_WIDTH = Note::GROUP_WIDTH;
    Note::TAG_ARROW_WIDTH = 5 + (big ? 4 : 0);
    Note::EMBLEM_SIZE = 16;
    Note::MIN_HEIGHT = 2 * Note::NOTE_MARGIN + Note::EMBLEM_SIZE;

    if (Global::bnpView)
        Global::bnpView->relayoutAllBaskets();
}

void Settings::setAutoBullet(bool yes)
{
    s_autoBullet = yes;
    if (Global::bnpView && Global::bnpView->currentBasket()) {
        Global::bnpView->currentBasket()->editorPropertiesChanged();
    }
}

/** SettingsDialog */
SettingsDialog::SettingsDialog(QWidget *parent)
    : KCMultiDialog(parent)
{
    QCoreApplication::addLibraryPath(QStringLiteral("/lib/plugins"));
    const QList<KPluginMetaData> availablePlugins = KPluginMetaData::findPlugins(QStringLiteral("pim/kcms/basket"));
    qInfo() << "SettingsDialog" << availablePlugins.size();
    for (const KPluginMetaData &metaData : availablePlugins) {
        qInfo() << "SettingsDialog" << metaData.pluginId() << metaData.fileName() << metaData.name();
#if KCMUTILS_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        addModule(metaData);
#else
        addModule(metaData.pluginId());
#endif
    }

    // Access the button box and connect the buttons
    QDialogButtonBox *buttons = buttonBox();

    connect(buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &SettingsDialog::slotOkClicked);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::slotApplyClicked);
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SettingsDialog::slotCancelClicked);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &SettingsDialog::slotDefaultsClicked);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Run code that relies on the dialog being fully shown
    QTimer::singleShot(0, this, &SettingsDialog::exec);
}

int SettingsDialog::exec()
{
    int ret = KCMultiDialog::exec();
    QTimer::singleShot(0, this, &SettingsDialog::adjustSize);
    // FIXME(JH) slot doesn't exist - should it?
    // QTimer::singleShot(0, this, &SettingsDialog::loadAllPages);
    return ret;
}

void SettingsDialog::adjustSize()
{
    QSize maxPageSize;
    const auto *model = qobject_cast<const KPageWidgetModel *>(pageWidget()->model());
    if (!model)
        return;

    const int pageCount = model->rowCount();
    for (int pageId = 0; pageId < pageCount; ++pageId) {
        auto *page = model->item(model->index(pageId, 0));
        if (!page)
            continue;

        maxPageSize = maxPageSize.expandedTo(page->widget()->sizeHint());
    }

    if (!maxPageSize.isEmpty())
        resize(1.25 * maxPageSize.width(), 1.25 * maxPageSize.height());
}

void SettingsDialog::slotCancelClicked()
{
    Q_EMIT cancelRequested();
    accept(); // Close the dialog
}

void SettingsDialog::slotApplyClicked()
{
    // Q_EMIT applyRequested();
}

void SettingsDialog::slotOkClicked()
{
    Q_EMIT acceptRequested();
    accept(); // Close the dialog
}

void SettingsDialog::slotDefaultsClicked()
{
    Q_EMIT defaultsRequested();
}

/** GeneralPage */
GeneralPage::GeneralPage(QObject *parent, const KPluginMetaData &data)
    : AbstractSettingsPage(parent, data)
{
    auto *layout = new QFormLayout(this->widget());

    // Basket Tree Position:
    m_treeOnLeft = new KComboBox(this->widget());
    m_treeOnLeft->addItem(i18n("On left"));
    m_treeOnLeft->addItem(i18n("On right"));

    layout->addRow(i18n("&Basket tree position:"), m_treeOnLeft);
    connect(m_treeOnLeft, &QComboBox::activated, this, &KCModule::markAsChanged);

    // Filter Bar Position:
    m_filterOnTop = new KComboBox(this->widget());
    m_filterOnTop->addItem(i18n("On top"));
    m_filterOnTop->addItem(i18n("On bottom"));

    layout->addRow(i18n("&Filter bar position:"), m_filterOnTop);
    connect(m_filterOnTop, &QComboBox::activated, this, &KCModule::markAsChanged);

    GeneralPage::load();
}

void GeneralPage::load()
{
    m_treeOnLeft->setCurrentIndex((int)!Settings::treeOnLeft());
    m_filterOnTop->setCurrentIndex((int)!Settings::filterOnTop());
    setNeedsSave(false);
}

void GeneralPage::save()
{
    Settings::setTreeOnLeft(!m_treeOnLeft->currentIndex());
    Settings::setFilterOnTop(!m_filterOnTop->currentIndex());
    setNeedsSave(false);
}

void GeneralPage::defaults()
{
    // TODO
}

void GeneralPage::cancel()
{
    // TODO
}

/** BasketsPage */

BasketsPage::BasketsPage(QObject *parent, const KPluginMetaData &data)
    : AbstractSettingsPage(parent, data)
{
    auto *layout = new QVBoxLayout(this->widget());
    QHBoxLayout *hLay;
    HelpLabel *hLabel;

    // Appearance:

    auto *appearanceBox = new QGroupBox(i18n("Appearance"), this->widget());
    auto *appearanceLayout = new QVBoxLayout;
    appearanceBox->setLayout(appearanceLayout);
    layout->addWidget(appearanceBox);

    m_showNotesToolTip = new QCheckBox(i18n("&Show tooltips in baskets"), appearanceBox);
    appearanceLayout->addWidget(m_showNotesToolTip);
    connect(m_showNotesToolTip, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    m_bigNotes = new QCheckBox(i18n("&Big notes"), appearanceBox);
    appearanceLayout->addWidget(m_bigNotes);
    connect(m_bigNotes, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    // Behavior:

    auto *behaviorBox = new QGroupBox(i18n("Behavior"), this->widget());
    auto *behaviorLayout = new QVBoxLayout;
    behaviorBox->setLayout(behaviorLayout);
    layout->addWidget(behaviorBox);

    m_autoBullet = new QCheckBox(i18n("&Transform lines starting with * or - to lists in text editors"), behaviorBox);
    behaviorLayout->addWidget(m_autoBullet);
    connect(m_autoBullet, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    m_confirmNoteDeletion = new QCheckBox(i18n("Ask confirmation before &deleting notes"), behaviorBox);
    behaviorLayout->addWidget(m_confirmNoteDeletion);
    connect(m_confirmNoteDeletion, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    m_pasteAsPlainText = new QCheckBox(i18n("Keep text formatting when pasting"), behaviorBox);
    behaviorLayout->addWidget(m_pasteAsPlainText);
    connect(m_pasteAsPlainText, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    m_detectTextTags = new QCheckBox(i18n("Automatically detect tags from note's content"), behaviorBox);
    behaviorLayout->addWidget(m_detectTextTags);
    connect(m_detectTextTags, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    auto *widget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(widget);
    hLay = new QHBoxLayout(widget);
    m_exportTextTags = new QCheckBox(i18n("&Export tags in texts"), widget);
    connect(m_exportTextTags, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    hLabel = new HelpLabel(
        i18n("When does this apply?"),
        QStringLiteral("<p>") + i18n("It does apply when you copy and paste, or drag and drop notes to a text editor.") + QStringLiteral("</p>")
            + QStringLiteral("<p>") + i18n("If enabled, this property lets you paste the tags as textual equivalents.") + QStringLiteral("<br>")
            + i18n("For instance, a list of notes with the <b>To Do</b> and <b>Done</b> tags are exported as lines preceded by <b>[ ]</b> or <b>[x]</b>, "
                   "representing an empty checkbox and a checked box.")
            + QStringLiteral("</p>") + QStringLiteral("<p align='center'><img src=\":/images/tag_export_help.png\"></p>"),
        widget);
    hLay->addWidget(m_exportTextTags);
    hLay->addWidget(hLabel);
    hLay->setContentsMargins(0, 0, 0, 0);
    hLay->addStretch();

    m_groupOnInsertionLineWidget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(m_groupOnInsertionLineWidget);
    auto *hLayV = new QHBoxLayout(m_groupOnInsertionLineWidget);
    m_groupOnInsertionLine = new QCheckBox(i18n("&Group a new note when clicking on the right of the insertion line"), m_groupOnInsertionLineWidget);
    auto *helpV =
        new HelpLabel(i18n("How to group a new note?"),
                      i18n("<p>When this option is enabled, the insertion-line not only allows you to insert notes at the cursor position, but also allows you "
                           "to group a new note with the one under the cursor:</p>")
                          + QStringLiteral("<p align='center'><img src=\":/images/insertion_help.png\"></p>")
                          + i18n("<p>Place your mouse between notes, where you want to add a new one.<br>"
                                 "Click on the <b>left</b> of the insertion-line middle-mark to <b>insert</b> a note.<br>"
                                 "Click on the <b>right</b> to <b>group</b> a note, with the one <b>below or above</b>, depending on where your mouse is.</p>"),
                      m_groupOnInsertionLineWidget);
    hLayV->addWidget(m_groupOnInsertionLine);
    hLayV->addWidget(helpV);
    hLayV->insertStretch(-1);
    layout->addWidget(m_groupOnInsertionLineWidget);
    connect(m_groupOnInsertionLine, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    widget = new QWidget(behaviorBox);
    behaviorLayout->addWidget(widget);
    auto *ga = new QGridLayout(widget);
    ga->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), 0, 3);

    m_middleAction = new KComboBox(widget);
    m_middleAction->addItem(i18n("Do nothing"));
    m_middleAction->addItem(i18n("Paste clipboard"));
    m_middleAction->addItem(i18n("Insert image note"));
    m_middleAction->addItem(i18n("Insert link note"));
    m_middleAction->addItem(i18n("Insert cross reference"));
    m_middleAction->addItem(i18n("Insert launcher note"));
    m_middleAction->addItem(i18n("Insert color note"));
    m_middleAction->addItem(i18n("Grab screen zone"));
    m_middleAction->addItem(i18n("Insert color from screen"));
    m_middleAction->addItem(i18n("Load note from file"));
    m_middleAction->addItem(i18n("Import Launcher for desktop application"));
    m_middleAction->addItem(i18n("Import icon"));

    auto *labelM = new QLabel(widget);
    labelM->setText(i18n("&Shift+middle-click anywhere:"));
    labelM->setBuddy(m_middleAction);

    ga->addWidget(labelM, 0, 0);
    ga->addWidget(m_middleAction, 0, 1);
    ga->addWidget(new QLabel(i18n("at cursor position"), widget), 0, 2);
    ga->setContentsMargins(1, 0, 0, 0);
    connect(m_middleAction, &QComboBox::activated, this, &KCModule::markAsChanged);

    // Protection:

    auto *protectionBox = new QGroupBox(i18n("Password Protection"), this->widget());
    auto *protectionLayout = new QVBoxLayout;
    layout->addWidget(protectionBox);
    protectionBox->setLayout(protectionLayout);
    widget = new QWidget(protectionBox);
    protectionLayout->addWidget(widget);

    // Re-Lock timeout configuration
    hLay = new QHBoxLayout(widget);
    m_enableReLockTimeoutMinutes = new QCheckBox(i18n("A&utomatically lock protected baskets when closed for"), widget);
    hLay->addWidget(m_enableReLockTimeoutMinutes);
    m_reLockTimeoutMinutes = new QSpinBox(widget);
    m_reLockTimeoutMinutes->setMinimum(0);
    m_reLockTimeoutMinutes->setSuffix(i18n(" minutes"));
    hLay->addWidget(m_reLockTimeoutMinutes);
    // label = new QLabel(i18n("minutes"), this);
    // hLay->addWidget(label);
    hLay->addStretch();
    //  layout->addLayout(hLay);
    connect(m_enableReLockTimeoutMinutes, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_reLockTimeoutMinutes, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);
    connect(m_enableReLockTimeoutMinutes, &QCheckBox::toggled, m_reLockTimeoutMinutes, &QWidget::setEnabled);

#ifdef HAVE_LIBGPGME
    m_useGnuPGAgent = new QCheckBox(i18n("Use GnuPG agent for &private/public key protected baskets"), protectionBox);
    protectionLayout->addWidget(m_useGnuPGAgent);
    //  hLay->addWidget(m_useGnuPGAgent);
    connect(m_useGnuPGAgent, &QCheckBox::toggled, this, &KCModule::markAsChanged);
#endif

    layout->insertStretch(-1);
    BasketsPage::load();
}

void BasketsPage::load()
{
    m_showNotesToolTip->setChecked(Settings::showNotesToolTip());
    m_bigNotes->setChecked(Settings::bigNotes());

    m_autoBullet->setChecked(Settings::autoBullet());
    m_confirmNoteDeletion->setChecked(Settings::confirmNoteDeletion());
    m_pasteAsPlainText->setChecked(!Settings::pasteAsPlainText());
    m_exportTextTags->setChecked(Settings::exportTextTags());
    m_detectTextTags->setChecked(Settings::detectTextTags());
    m_groupOnInsertionLine->setChecked(Settings::groupOnInsertionLine());
    m_middleAction->setCurrentIndex(Settings::middleAction());

    // The correctness of this code depends on the default of enableReLockTimeout
    // being true - otherwise, the reLockTimeoutMinutes widget is not disabled properly.
    m_enableReLockTimeoutMinutes->setChecked(Settings::enableReLockTimeout());
    m_reLockTimeoutMinutes->setValue(Settings::reLockTimeoutMinutes());
#ifdef HAVE_LIBGPGME
    m_useGnuPGAgent->setChecked(Settings::useGnuPGAgent());

    if (KGpgMe::isGnuPGAgentAvailable()) {
        m_useGnuPGAgent->setChecked(Settings::useGnuPGAgent());
    } else {
        m_useGnuPGAgent->setChecked(false);
        m_useGnuPGAgent->setEnabled(false);
    }
#endif
    setNeedsSave(false);
}

void BasketsPage::save()
{
    Settings::setShowNotesToolTip(m_showNotesToolTip->isChecked());
    Settings::setBigNotes(m_bigNotes->isChecked());

    Settings::setAutoBullet(m_autoBullet->isChecked());
    Settings::setConfirmNoteDeletion(m_confirmNoteDeletion->isChecked());
    Settings::setPasteAsPlainText(!m_pasteAsPlainText->isChecked());
    Settings::setExportTextTags(m_exportTextTags->isChecked());
    Settings::setDetectTextTags(m_detectTextTags->isChecked());

    Settings::setGroupOnInsertionLine(m_groupOnInsertionLine->isChecked());
    Settings::setMiddleAction(m_middleAction->currentIndex());

    Settings::setEnableReLockTimeout(m_enableReLockTimeoutMinutes->isChecked());
    Settings::setReLockTimeoutMinutes(m_reLockTimeoutMinutes->value());
#ifdef HAVE_LIBGPGME
    Settings::setUseGnuPGAgent(m_useGnuPGAgent->isChecked());
#endif
    setNeedsSave(false);
}

void BasketsPage::defaults()
{
    // TODO
}

/** class NewNotesPage: */

NewNotesPage::NewNotesPage(QObject *parent, const KPluginMetaData &data)
    : AbstractSettingsPage(parent, data)
{
    auto *layout = new QVBoxLayout(this->widget());
    QHBoxLayout *hLay;
    QLabel *label;

    // Place of New Notes:

    hLay = new QHBoxLayout;
    m_newNotesPlace = new KComboBox(this->widget());

    label = new QLabel(this->widget());
    label->setText(i18n("&Place of new notes:"));
    label->setBuddy(m_newNotesPlace);

    m_newNotesPlace->addItem(i18n("On top"));
    m_newNotesPlace->addItem(i18n("On bottom"));
    m_newNotesPlace->addItem(i18n("At current note"));
    hLay->addWidget(label);
    hLay->addWidget(m_newNotesPlace);
    hLay->addStretch();
    // layout->addLayout(hLay);
    label->hide();
    m_newNotesPlace->hide();
    connect(m_newNotesPlace, &QComboBox::editTextChanged, this, &KCModule::markAsChanged);

    // New Images Size:

    hLay = new QHBoxLayout;
    m_imgSizeX = new QSpinBox(this->widget());
    m_imgSizeX->setMinimum(1);
    m_imgSizeX->setMaximum(4096);
    // m_imgSizeX->setReferencePoint(100); //from KIntNumInput
    connect(m_imgSizeX, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);

    label = new QLabel(this->widget());
    label->setText(i18n("&New images size:"));
    label->setBuddy(m_imgSizeX);

    hLay->addWidget(label);
    hLay->addWidget(m_imgSizeX);

    m_imgSizeY = new QSpinBox(this->widget());
    m_imgSizeY->setMinimum(1);
    m_imgSizeY->setMaximum(4096);
    // m_imgSizeY->setReferencePoint(100);
    connect(m_imgSizeY, &QSpinBox::valueChanged, this, &KCModule::markAsChanged);

    label = new QLabel(this->widget());
    label->setText(i18n("&by"));
    label->setBuddy(m_imgSizeY);

    hLay->addWidget(label);
    hLay->addWidget(m_imgSizeY);
    label = new QLabel(i18n("pixels"), this->widget());
    hLay->addWidget(label);
    m_pushVisualize = new QPushButton(i18n("&Visualize..."), this->widget());
    hLay->addWidget(m_pushVisualize);
    hLay->addStretch();
    layout->addLayout(hLay);
    connect(m_pushVisualize, &QAbstractButton::clicked, this, &NewNotesPage::visualize);

    // View File Content:

    auto *buttonGroup = new QGroupBox(i18n("View Content of Added Files for the Following Types"), this->widget());
    auto *buttonLayout = new QVBoxLayout;
    m_viewTextFileContent = new QCheckBox(i18n("&Plain text"), buttonGroup);
    m_viewHtmlFileContent = new QCheckBox(i18n("&HTML page"), buttonGroup);
    m_viewImageFileContent = new QCheckBox(i18n("&Image or animation"), buttonGroup);
    m_viewSoundFileContent = new QCheckBox(i18n("&Sound"), buttonGroup);

    buttonLayout->addWidget(m_viewTextFileContent);
    buttonLayout->addWidget(m_viewHtmlFileContent);
    buttonLayout->addWidget(m_viewImageFileContent);
    buttonLayout->addWidget(m_viewSoundFileContent);
    buttonGroup->setLayout(buttonLayout);

    layout->addWidget(buttonGroup);
    connect(m_viewTextFileContent, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_viewHtmlFileContent, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_viewImageFileContent, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_viewSoundFileContent, &QCheckBox::toggled, this, &KCModule::markAsChanged);

    layout->insertStretch(-1);
    NewNotesPage::load();
}

void NewNotesPage::load()
{
    m_newNotesPlace->setCurrentIndex(Settings::newNotesPlace());

    m_imgSizeX->setValue(Settings::defImageX());
    m_imgSizeY->setValue(Settings::defImageY());

    m_viewTextFileContent->setChecked(Settings::viewTextFileContent());
    m_viewHtmlFileContent->setChecked(Settings::viewHtmlFileContent());
    m_viewImageFileContent->setChecked(Settings::viewImageFileContent());
    m_viewSoundFileContent->setChecked(Settings::viewSoundFileContent());
    setNeedsSave(false);
}

void NewNotesPage::save()
{
    Settings::setNewNotesPlace(m_newNotesPlace->currentIndex());

    Settings::setDefImageX(m_imgSizeX->value());
    Settings::setDefImageY(m_imgSizeY->value());

    Settings::setViewTextFileContent(m_viewTextFileContent->isChecked());
    Settings::setViewHtmlFileContent(m_viewHtmlFileContent->isChecked());
    Settings::setViewImageFileContent(m_viewImageFileContent->isChecked());
    Settings::setViewSoundFileContent(m_viewSoundFileContent->isChecked());
    setNeedsSave(false);
}

void NewNotesPage::defaults()
{
    // TODO
}

void NewNotesPage::visualize()
{
    QPointer<ViewSizeDialog> size = new ViewSizeDialog(this->widget(), m_imgSizeX->value(), m_imgSizeY->value());
    size->exec();
    m_imgSizeX->setValue(size->width());
    m_imgSizeY->setValue(size->height());
}

/** class NotesAppearancePage: */

NotesAppearancePage::NotesAppearancePage(QObject *parent, const KPluginMetaData &data)
    : AbstractSettingsPage(parent, data)
{
    auto *layout = new QVBoxLayout(this->widget());
    auto *tabs = new QTabWidget(this->widget());
    layout->addWidget(tabs);

    m_soundLook = new LinkLookEditWidget(this, i18n("Conference audio record"), QStringLiteral("folder-sound"), tabs);
    m_fileLook = new LinkLookEditWidget(this, i18n("Annual report"), QStringLiteral("folder-documents"), tabs);
    m_localLinkLook = new LinkLookEditWidget(this, i18n("Home folder"), QStringLiteral("user-home"), tabs);
    m_networkLinkLook = new LinkLookEditWidget(this, QStringLiteral("kde.org"), KIO::iconNameForUrl(QUrl(QStringLiteral("https://kde.org"))), tabs);
    m_launcherLook = new LinkLookEditWidget(this, i18n("Launch %1", QGuiApplication::applicationDisplayName()), QStringLiteral("basket"), tabs);
    m_crossReferenceLook = new LinkLookEditWidget(this, i18n("Another basket"), QStringLiteral("basket"), tabs);

    tabs->addTab(m_soundLook, i18n("&Sounds"));
    tabs->addTab(m_fileLook, i18n("&Files"));
    tabs->addTab(m_localLinkLook, i18n("&Local Links"));
    tabs->addTab(m_networkLinkLook, i18n("&Network Links"));
    tabs->addTab(m_launcherLook, i18n("Launc&hers"));
    tabs->addTab(m_crossReferenceLook, i18n("&Cross References"));

    NotesAppearancePage::load();
}

void NotesAppearancePage::load()
{
    m_soundLook->set(LinkLook::soundLook);
    m_fileLook->set(LinkLook::fileLook);
    m_localLinkLook->set(LinkLook::localLinkLook);
    m_networkLinkLook->set(LinkLook::networkLinkLook);
    m_launcherLook->set(LinkLook::launcherLook);
    m_crossReferenceLook->set(LinkLook::crossReferenceLook);
    setNeedsSave(false);
}

void NotesAppearancePage::save()
{
    m_soundLook->saveChanges();
    m_fileLook->saveChanges();
    m_localLinkLook->saveChanges();
    m_networkLinkLook->saveChanges();
    m_launcherLook->saveChanges();
    m_crossReferenceLook->saveChanges();
    Global::bnpView->linkLookChanged();
    setNeedsSave(false);
}

void NotesAppearancePage::defaults()
{
    // TODO
}

/** class ApplicationsPage: */

ApplicationsPage::ApplicationsPage(QObject *parent, const KPluginMetaData &data)
    : AbstractSettingsPage(parent, data)
{
    /* Applications page */
    auto *layout = new QVBoxLayout(this->widget());

    auto *launchGroup = new QGroupBox(i18n("General Launch Associations"), this->widget());

    QString menuEditTooltip(i18n(
        "<p>On modern desktop environments, launcher configurations provide information to the system about how to handle an application."
        "These files are essential for integrating applications into the desktop environment, to list, lookup and launch apps with specific configurations</p>"
        "<p>Here is how to do if you want to launch Falkon Browser with a specific profile.</p>"
        "<ul>"
        "<li>Open the KDE Menu Editor, and type \"Falkon\" in the search filter;</li>"
        "<li>Select \"Falkon\" desktop launcher, right-click, copy and paste;</li>"
        "<li>Edit the \"General\" property fields like \"Name\", \"Description\" or \"Comment\".</li>"
        "<li>Edit the \"Command-line arguments\" field to something like \"--profile 'Your-Extensive-Hobby' %%u\".</li>"
        "</ul>"
        "<p>Now, when you choose your new launcher as basket link notes launcher, basket will open HTTP links with Falkon's \"Your-Extensive-Hobby\" "
        "profile</p>"
        "<p>For more fine-grained configuration (like opening pdf documents preferably with Falkon), read the second button tooltip.</p>"));

    QString componentChooserTooltip(
        i18n("<p>When opening Web links, they are opened in different applications, depending on the content of the link "
             "(a Web page, an image, a PDF document...), such as if they were files on your computer.</p>"
             "<p>Here is how to do if you want every Web addresses to be opened in your Web browser. "
             "It is useful if you are not using Plasma (if you are using eg. GNOME, XFCE...).</p>"
             "<ul>"
             "<li>Open the KDE System Settings (if it is not available, try to type \"systemsettings\" in a command line terminal);</li>"
             "<li>Go to the \"Applications\" and then \"Default Applications\" section;</li>"
             "<li>Choose \"Web Browser\", check \"with the following command:\" and enter the name of your Web browser (like \"firefox\" or \"epiphany\").</li>"
             "</ul>"
             "<p>Now, when you click <i>any</i> link that start with \"https://...\", it will be opened in your Web browser (eg. Mozilla Firefox or Epiphany "
             "or...).</p>"
             "<p>For more fine-grained configuration (like opening only Web pages in your Web browser), read the third button tooltip.</p>"));

    QString fileAssociationsTooltip(
        i18n("<p>Here is how to set the application to be used for each type of file. "
             "This also applies to Web links if you choose not to open them systematically in a Web browser (see the first help link). "
             "The default settings should be good enough for you, but this tip is useful if you are using GNOME, XFCE, or another environment than Plasma.</p>"
             "<p>This is an example of how to open HTML pages in your Web browser (and keep using the other applications for other addresses or files). "
             "Repeat these steps for each type of file you want to open in a specific application.</p>"
             "<ul>"
             "<li>Open the KDE System Settings (if it is not available, try to type \"systemsettings\" in a command line terminal);</li>"
             "<li>Go to the \"Applications\" and then \"File Associations\" section;</li>"
             "<li>In the tree, expand \"text\" and click \"html\";</li>"
             "<li>In the applications list, add your Web browser as the first entry;</li>"
             "<li>Do the same for the type \"application -> xhtml+xml\".</li>"
             "</ul>"));

    auto *vLay = new QVBoxLayout(this->widget());
    auto *hLay = new QHBoxLayout(launchGroup);
    m_menuEdit = new QPushButton();
    QIcon buttonIcon = QIcon::fromTheme(QStringLiteral("kmenuedit"));
    m_menuEdit->setIcon(buttonIcon);
    m_menuEdit->setIconSize(QSize(24, 24));
    m_menuEdit->setText(QStringLiteral("Launch KDE Menu Editor"));
    m_menuEdit->setToolTip(menuEditTooltip);
    connect(m_menuEdit, &QAbstractButton::clicked, this, &ApplicationsPage::openMenuEditor);
    hLay->addWidget(m_menuEdit);

    m_componentChooser = new QPushButton();
    buttonIcon = QIcon::fromTheme(QStringLiteral("preferences-desktop-default-applications"));
    m_componentChooser->setIcon(buttonIcon);
    m_componentChooser->setIconSize(QSize(24, 24));
    m_componentChooser->setText(QStringLiteral("Default Applications"));
    m_componentChooser->setToolTip(componentChooserTooltip);
    connect(m_componentChooser, &QAbstractButton::clicked, this, &ApplicationsPage::openDefaultApplications);
    hLay->addWidget(m_componentChooser);

    m_fileTypes = new QPushButton();
    buttonIcon = QIcon::fromTheme(QStringLiteral("preferences-desktop-filetype-association"));
    m_fileTypes->setIcon(buttonIcon);
    m_fileTypes->setIconSize(QSize(24, 24));
    m_fileTypes->setText(QStringLiteral("File Associations"));
    m_fileTypes->setToolTip(fileAssociationsTooltip);
    connect(m_fileTypes, &QAbstractButton::clicked, this, &ApplicationsPage::openFileAssociations);
    hLay->addWidget(m_fileTypes);

    vLay->addLayout(hLay);
    vLay->setAlignment(hLay, Qt::AlignHCenter); // Center align horizontally
    launchGroup->setLayout(vLay);
    layout->addWidget(launchGroup);

    vLay = new QVBoxLayout(this->widget());
    auto *basketLaunchGroup = new QGroupBox(i18n("Specific Basket Associations"), this->widget());

    m_htmlUseProg = new QCheckBox(i18n("Open &text notes with a custom application:"), this->widget());
    m_htmlProg = new ServiceLaunchRequester(QString(), i18n("Open text notes with:"), this->widget());
    auto *hLayH = new QHBoxLayout();
    hLayH->insertSpacing(-1, 20);
    hLayH->addWidget(m_htmlProg);
    connect(m_htmlUseProg, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_htmlProg, &ServiceLaunchRequester::launcherChanged, this, &KCModule::markAsChanged);

    m_imageUseProg = new QCheckBox(i18n("Open &image notes with a custom application:"), this->widget());
    m_imageProg = new ServiceLaunchRequester(QString(), i18n("Open image notes with:"), this->widget());
    auto *hLayI = new QHBoxLayout();
    hLayI->insertSpacing(-1, 20);
    hLayI->addWidget(m_imageProg);
    connect(m_imageUseProg, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_imageProg, &ServiceLaunchRequester::launcherChanged, this, &KCModule::markAsChanged);

    m_animationUseProg = new QCheckBox(i18n("Open a&nimation notes with a custom application:"), this->widget());
    m_animationProg = new ServiceLaunchRequester(QString(), i18n("Open animation notes with:"), this->widget());
    auto *hLayA = new QHBoxLayout();
    hLayA->insertSpacing(-1, 20);
    hLayA->addWidget(m_animationProg);
    connect(m_animationUseProg, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_animationProg, &ServiceLaunchRequester::launcherChanged, this, &KCModule::markAsChanged);

    m_soundUseProg = new QCheckBox(i18n("Open so&und notes with a custom application:"), this->widget());
    m_soundProg = new ServiceLaunchRequester(QString(), i18n("Open sound notes with:"), this->widget());
    auto *hLayS = new QHBoxLayout();
    hLayS->insertSpacing(-1, 20);
    hLayS->addWidget(m_soundProg);
    connect(m_soundUseProg, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_soundProg, &ServiceLaunchRequester::launcherChanged, this, &KCModule::markAsChanged);

    m_linkUseProg = new QCheckBox(i18n("Open http link notes with a custom application:"), this->widget());
    m_linkProg = new ServiceLaunchRequester(QString(), i18n("Open http link notes with:"), this->widget());
    auto *hLayL = new QHBoxLayout();
    hLayL->insertSpacing(-1, 20);
    hLayL->addWidget(m_linkProg);
    connect(m_linkUseProg, &QCheckBox::toggled, this, &KCModule::markAsChanged);
    connect(m_linkProg, &ServiceLaunchRequester::launcherChanged, this, &KCModule::markAsChanged);

    QString whatsthis = i18n(
        "<p>If checked, the application defined below will be used when opening that type of note.</p>"
        "<p>Otherwise, the application you've configured in Konqueror will be used.</p>");

    m_htmlUseProg->setWhatsThis(whatsthis);
    m_imageUseProg->setWhatsThis(whatsthis);
    m_animationUseProg->setWhatsThis(whatsthis);
    m_soundUseProg->setWhatsThis(whatsthis);
    m_linkUseProg->setWhatsThis(whatsthis);

    whatsthis = i18n(
        "<p>Define the application to use for opening that type of note instead of the "
        "application configured in Konqueror.</p>");

    m_htmlProg->setWhatsThis(whatsthis);
    m_imageProg->setWhatsThis(whatsthis);
    m_animationProg->setWhatsThis(whatsthis);
    m_soundProg->setWhatsThis(whatsthis);
    m_linkProg->setWhatsThis(whatsthis);

    vLay->addWidget(m_htmlUseProg);
    vLay->addItem(hLayH);
    vLay->addWidget(m_imageUseProg);
    vLay->addItem(hLayI);
    vLay->addWidget(m_animationUseProg);
    vLay->addItem(hLayA);
    vLay->addWidget(m_soundUseProg);
    vLay->addItem(hLayS);
    vLay->addWidget(m_linkUseProg);
    vLay->addItem(hLayL);

    basketLaunchGroup->setLayout(vLay);

    connect(m_htmlUseProg, &QCheckBox::toggled, m_htmlProg, &QWidget::setEnabled);
    connect(m_imageUseProg, &QCheckBox::toggled, m_imageProg, &QWidget::setEnabled);
    connect(m_animationUseProg, &QCheckBox::toggled, m_animationProg, &QWidget::setEnabled);
    connect(m_soundUseProg, &QCheckBox::toggled, m_soundProg, &QWidget::setEnabled);
    connect(m_linkUseProg, &QCheckBox::toggled, m_linkProg, &QWidget::setEnabled);

    layout->addWidget(basketLaunchGroup);
    layout->insertStretch(-1);
    ApplicationsPage::load();
}

void ApplicationsPage::load()
{
    m_htmlProg->setServiceLauncher(Settings::htmlProg());
    m_htmlUseProg->setChecked(Settings::isHtmlUseProg());
    m_htmlProg->setEnabled(Settings::isHtmlUseProg());

    m_imageProg->setServiceLauncher(Settings::imageProg());
    m_imageUseProg->setChecked(Settings::isImageUseProg());
    m_imageProg->setEnabled(Settings::isImageUseProg());

    m_animationProg->setServiceLauncher(Settings::animationProg());
    m_animationUseProg->setChecked(Settings::isAnimationUseProg());
    m_animationProg->setEnabled(Settings::isAnimationUseProg());

    m_soundProg->setServiceLauncher(Settings::soundProg());
    m_soundUseProg->setChecked(Settings::isSoundUseProg());
    m_soundProg->setEnabled(Settings::isSoundUseProg());

    m_linkProg->setServiceLauncher(Settings::linkProg());
    m_linkUseProg->setChecked(Settings::isLinkUseProg());
    m_linkProg->setEnabled(Settings::isLinkUseProg());
    setNeedsSave(false);
}

void ApplicationsPage::save()
{
    Settings::setIsHtmlUseProg(m_htmlUseProg->isChecked());
    Settings::setHtmlProg(m_htmlProg->serviceLauncher());

    Settings::setIsImageUseProg(m_imageUseProg->isChecked());
    Settings::setImageProg(m_imageProg->serviceLauncher());

    Settings::setIsAnimationUseProg(m_animationUseProg->isChecked());
    Settings::setAnimationProg(m_animationProg->serviceLauncher());

    Settings::setIsSoundUseProg(m_soundUseProg->isChecked());
    Settings::setSoundProg(m_soundProg->serviceLauncher());

    Settings::setIsLinkUseProg(m_linkUseProg->isChecked());
    Settings::setLinkProg(m_linkProg->serviceLauncher());
    setNeedsSave(false);
}

void ApplicationsPage::defaults()
{
    // TODO
}

bool ApplicationsPage::launchKDEModule(const QString &command, const QStringList &args)
{
    // Execute the command synchronously
    int ret = QProcess::execute(command, args);
    if (ret != 0) {
        return false;
    }
    return true;
}

void ApplicationsPage::openMenuEditor()
{
    launchKDEModule(QStringLiteral("kmenuedit"));
}

void ApplicationsPage::openDefaultApplications()
{
    launchKDEModule(QStringLiteral("kcmshell6"), {QStringLiteral("componentchooser")});
}

void ApplicationsPage::openFileAssociations()
{
    launchKDEModule(QStringLiteral("kcmshell6"), {QStringLiteral("filetypes")});
}

#include "moc_settings.cpp"
