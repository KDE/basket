/**
 * SPDX-FileCopyrightText: (C) 2005 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tagsedit.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView> //For m_tags->header()
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSeparator>
#include <KShortcutWidget>

#include "bnpview.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "tag.h"
#include "variouswidgets.h" //For FontSizeCombo

/** class StateCopy: */

StateCopy::StateCopy(State *old /* = 0*/)
{
    oldState = old;
    newState = new State();
    if (oldState)
        oldState->copyTo(newState);
}

StateCopy::~StateCopy()
{
    delete newState;
}

void StateCopy::copyBack()
{
}

/** class TagCopy: */

TagCopy::TagCopy(Tag *old /* = 0*/)
{
    oldTag = old;
    newTag = new Tag();
    if (oldTag)
        oldTag->copyTo(newTag);

    if (old)
        for (State::List::iterator it = old->states().begin(); it != old->states().end(); ++it)
            stateCopies.append(new StateCopy(*it));
    else
        stateCopies.append(new StateCopy());
}

TagCopy::~TagCopy()
{
    delete newTag;
}

void TagCopy::copyBack()
{
}

bool TagCopy::isMultiState()
{
    return (stateCopies.count() > 1);
}

/** class TagListViewItem: */

TagListViewItem::TagListViewItem(QTreeWidget *parent, TagCopy *tagCopy)
    : QTreeWidgetItem(parent)
    , m_tagCopy(tagCopy)
    , m_stateCopy(nullptr)
{
    setText(0, tagCopy->newTag->name());
}

TagListViewItem::TagListViewItem(QTreeWidgetItem *parent, TagCopy *tagCopy)
    : QTreeWidgetItem(parent)
    , m_tagCopy(tagCopy)
    , m_stateCopy(nullptr)
{
    setText(0, tagCopy->newTag->name());
}

TagListViewItem::TagListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, TagCopy *tagCopy)
    : QTreeWidgetItem(parent, after)
    , m_tagCopy(tagCopy)
    , m_stateCopy(nullptr)
{
    setText(0, tagCopy->newTag->name());
}

TagListViewItem::TagListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, TagCopy *tagCopy)
    : QTreeWidgetItem(parent, after)
    , m_tagCopy(tagCopy)
    , m_stateCopy(nullptr)
{
    setText(0, tagCopy->newTag->name());
}

/* */

TagListViewItem::TagListViewItem(QTreeWidget *parent, StateCopy *stateCopy)
    : QTreeWidgetItem(parent)
    , m_tagCopy(nullptr)
    , m_stateCopy(stateCopy)
{
    setText(0, stateCopy->newState->name());
}

TagListViewItem::TagListViewItem(QTreeWidgetItem *parent, StateCopy *stateCopy)
    : QTreeWidgetItem(parent)
    , m_tagCopy(nullptr)
    , m_stateCopy(stateCopy)
{
    setText(0, stateCopy->newState->name());
}

TagListViewItem::TagListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, StateCopy *stateCopy)
    : QTreeWidgetItem(parent, after)
    , m_tagCopy(nullptr)
    , m_stateCopy(stateCopy)
{
    setText(0, stateCopy->newState->name());
}

TagListViewItem::TagListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, StateCopy *stateCopy)
    : QTreeWidgetItem(parent, after)
    , m_tagCopy(nullptr)
    , m_stateCopy(stateCopy)
{
    setText(0, stateCopy->newState->name());
}

/* */

TagListViewItem::~TagListViewItem() = default;

TagListViewItem *TagListViewItem::lastChild()
{
    if (childCount() <= 0)
        return nullptr;
    return (TagListViewItem *)child(childCount() - 1);
}

bool TagListViewItem::isEmblemObligatory()
{
    return m_stateCopy != nullptr; // It's a state of a multi-state
}

TagListViewItem *TagListViewItem::prevSibling()
{
    TagListViewItem *item = this;
    int idx = 0;
    if (!parent()) {
        idx = treeWidget()->indexOfTopLevelItem(item);
        if (idx <= 0)
            return nullptr;
        return (TagListViewItem *)treeWidget()->topLevelItem(idx - 1);
    } else {
        idx = parent()->indexOfChild(item);
        if (idx <= 0)
            return nullptr;
        return (TagListViewItem *)parent()->child(idx - 1);
    }
}

TagListViewItem *TagListViewItem::nextSibling()
{
    TagListViewItem *item = this;
    int idx = 0;
    if (!parent()) {
        idx = treeWidget()->indexOfTopLevelItem(item);
        if (idx >= treeWidget()->topLevelItemCount())
            return nullptr;
        return (TagListViewItem *)treeWidget()->topLevelItem(idx + 1);
    } else {
        idx = parent()->indexOfChild(item);
        if (idx >= parent()->childCount())
            return nullptr;
        return (TagListViewItem *)parent()->child(idx + 1);
    }
}

TagListViewItem *TagListViewItem::parent() const
{
    return (TagListViewItem *)QTreeWidgetItem::parent();
}

// TODO: TagListViewItem::
int TAG_ICON_SIZE = 16;
int TAG_MARGIN = 1;

int TagListViewItem::width(const QFontMetrics & /* fontMetrics */, const QTreeWidget * /*listView*/, int /* column */) const
{
    return treeWidget()->width();
}

void TagListViewItem::setup()
{
    QString text = (m_tagCopy ? m_tagCopy->newTag->name() : m_stateCopy->newState->name());
    State *state = (m_tagCopy ? m_tagCopy->stateCopies[0]->newState : m_stateCopy->newState);

    QFont font = state->font(treeWidget()->font());

    setText(0, text);

    QBrush brush;

    bool withIcon = m_stateCopy || (m_tagCopy && !m_tagCopy->isMultiState());
    brush.setColor(isSelected() ? qApp->palette().color(QPalette::Highlight)
                                : (withIcon && state->backgroundColor().isValid()
                                       ? state->backgroundColor()
                                       : treeWidget()->viewport()->palette().color(treeWidget()->viewport()->backgroundRole())));
    setBackground(1, brush);
}

/** class TagListView: */

TagListView::TagListView(QWidget *parent)
    : QTreeWidget(parent)
{
    setItemDelegate(new TagListDelegate);
}

TagListView::~TagListView() = default;

void TagListView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete)
        Q_EMIT deletePressed();
    else if (event->key() != Qt::Key_Left || (currentItem() && currentItem()->parent()))
        // Do not allow to open/close first-level items
        QTreeWidget::keyPressEvent(event);
}

void TagListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Ignore this event! Do not open/close first-level items!

    // But trigger edit (change focus to name) when double-click an item:
    if (itemAt(event->pos()) != nullptr)
        Q_EMIT doubleClickedItem();
}

void TagListView::mousePressEvent(QMouseEvent *event)
{
    // When clicking on an empty space, QListView would unselect the current item! We forbid that!
    if (itemAt(event->pos()) != nullptr)
        QTreeWidget::mousePressEvent(event);
}

void TagListView::mouseReleaseEvent(QMouseEvent *event)
{
    // When clicking on an empty space, QListView would unselect the current item! We forbid that!
    if (itemAt(event->pos()) != nullptr)
        QTreeWidget::mouseReleaseEvent(event);
}

TagListViewItem *TagListView::currentItem() const
{
    return (TagListViewItem *)QTreeWidget::currentItem();
}

TagListViewItem *TagListView::firstChild() const
{
    if (topLevelItemCount() <= 0)
        return nullptr;
    return (TagListViewItem *)topLevelItem(0);
}

TagListViewItem *TagListView::lastItem() const
{
    if (topLevelItemCount() <= 0)
        return nullptr;
    return (TagListViewItem *)topLevelItem(topLevelItemCount() - 1);
}

/** class TagsEditDialog: */

TagsEditDialog::TagsEditDialog(QWidget *parent, State *stateToEdit, bool addNewTag)
    : QDialog(parent)
    , m_loading(false)
{
    // QDialog options
    setWindowTitle(i18n("Customize Tags"));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TagsEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TagsEditDialog::reject);
    mainLayout->addWidget(buttonBox);
    okButton->setDefault(true);
    setObjectName("CustomizeTags");

    auto *layout = new QHBoxLayout(mainWidget);

    /* Left part: */

    auto *newTag = new QPushButton(i18n("Ne&w Tag"), mainWidget);
    auto *newState = new QPushButton(i18n("New St&ate"), mainWidget);

    connect(newTag, &QPushButton::clicked, this, &TagsEditDialog::newTag);
    connect(newState, &QPushButton::clicked, this, &TagsEditDialog::newState);

    m_tags = new TagListView(mainWidget);
    m_tags->header()->hide();
    m_tags->setRootIsDecorated(false);
    // m_tags->addColumn(QString());
    // m_tags->setSorting(-1); // Sort column -1, so disabled sorting
    // m_tags->setResizeMode(QTreeWidget::LastColumn);

    m_tags->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_moveUp = new QPushButton(mainWidget);
    KGuiItem::assign(m_moveUp, KGuiItem(QString(), QStringLiteral("arrow-up")));
    m_moveDown = new QPushButton(mainWidget);
    KGuiItem::assign(m_moveDown, KGuiItem(QString(), QStringLiteral("arrow-down")));
    m_deleteTag = new QPushButton(mainWidget);
    KGuiItem::assign(m_deleteTag, KGuiItem(QString(), QStringLiteral("edit-delete")));

    m_moveUp->setToolTip(i18n("Move Up (Ctrl+Shift+Up)"));
    m_moveDown->setToolTip(i18n("Move Down (Ctrl+Shift+Down)"));
    m_deleteTag->setToolTip(i18n("Delete"));

    connect(m_moveUp, &QPushButton::clicked, this, &TagsEditDialog::moveUp);
    connect(m_moveDown, &QPushButton::clicked, this, &TagsEditDialog::moveDown);
    connect(m_deleteTag, &QPushButton::clicked, this, &TagsEditDialog::deleteTag);

    auto *topLeftLayout = new QHBoxLayout;
    topLeftLayout->addWidget(m_moveUp);
    topLeftLayout->addWidget(m_moveDown);
    topLeftLayout->addWidget(m_deleteTag);

    auto *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(newTag);
    leftLayout->addWidget(newState);
    leftLayout->addWidget(m_tags);
    leftLayout->addLayout(topLeftLayout);

    layout->addLayout(leftLayout);

    /* Right part: */

    auto *rightWidget = new QWidget(mainWidget);

    m_tagBox = new QGroupBox(i18n("Tag"), rightWidget);
    m_tagBoxLayout = new QHBoxLayout;
    m_tagBox->setLayout(m_tagBoxLayout);

    auto *tagWidget = new QWidget;
    m_tagBoxLayout->addWidget(tagWidget);

    m_tagName = new QLineEdit(tagWidget);
    auto *tagNameLabel = new QLabel(i18n("&Name:"), tagWidget);
    tagNameLabel->setBuddy(m_tagName);

    m_shortcut = new KShortcutWidget(tagWidget);
    m_removeShortcut = new QPushButton(i18nc("Remove tag shortcut", "&Remove"), tagWidget);
    auto *shortcutLabel = new QLabel(i18n("S&hortcut:"), tagWidget);
    shortcutLabel->setBuddy(m_shortcut);
    // connect(m_shortcut, &KShortcutWidget::shortcutChanged, this, &TagsEditDialog::capturedShortcut);
    connect(m_removeShortcut, &QPushButton::clicked, this, &TagsEditDialog::removeShortcut);

    m_inherit = new QCheckBox(i18n("&Inherited by new sibling notes"), tagWidget);

    m_allowCrossRefernce = new QCheckBox(i18n("Allow Cross Reference Links"), tagWidget);

    auto *allowCrossReferenceHelp = new HelpLabel(
        i18n("What does this do?"),
        QStringLiteral("<p>")
            + i18n("This option will enable you to type a cross reference link directly into a text note. Cross Reference links can have the following syntax:")
            + QStringLiteral("</p>") + QStringLiteral("<p>") + i18n("From the top of the tree (Absolute path):") + QStringLiteral("<br />")
            + i18n("[[/top level item/child|optional title]]") + QStringLiteral("<p>") + QStringLiteral("<p>") + i18n("Relative to the current basket:")
            + QStringLiteral("<br />") + i18n("[[../sibling|optional title]]") + QStringLiteral("<br />") + i18n("[[child|optional title]]")
            + QStringLiteral("<br />") + i18n("[[./child|optional title]]") + QStringLiteral("<p>") + QStringLiteral("<p>")
            + i18n("Baskets matching is cAse inSEnsItive.") + QStringLiteral("</p>"),
        tagWidget);

    auto *tagGrid = new QGridLayout(tagWidget);
    tagGrid->addWidget(tagNameLabel, 0, 0);
    tagGrid->addWidget(m_tagName, 0, 1, 1, 3);
    tagGrid->addWidget(shortcutLabel, 1, 0);
    tagGrid->addWidget(m_shortcut, 1, 1);
    tagGrid->addWidget(m_removeShortcut, 1, 2);
    tagGrid->addWidget(m_inherit, 2, 0, 1, 4);
    tagGrid->addWidget(m_allowCrossRefernce, 3, 0);
    tagGrid->addWidget(allowCrossReferenceHelp, 3, 1);
    tagGrid->setColumnStretch(/*col=*/3, /*stretch=*/255);

    m_stateBox = new QGroupBox(i18n("State"), rightWidget);
    m_stateBoxLayout = new QHBoxLayout;
    m_stateBox->setLayout(m_stateBoxLayout);

    auto *stateWidget = new QWidget;
    m_stateBoxLayout->addWidget(stateWidget);

    m_stateName = new QLineEdit(stateWidget);
    m_stateNameLabel = new QLabel(i18n("Na&me:"), stateWidget);
    m_stateNameLabel->setBuddy(m_stateName);

    auto *emblemWidget = new QWidget(stateWidget);
    m_emblem = new KIconButton(emblemWidget);
    m_emblem->setIconType(KIconLoader::NoGroup, KIconLoader::Action);
    m_emblem->setIconSize(16);
    m_emblem->setIcon(QStringLiteral("edit-delete"));
    m_removeEmblem = new QPushButton(i18nc("Remove tag emblem", "Remo&ve"), emblemWidget);
    auto *emblemLabel = new QLabel(i18n("&Emblem:"), stateWidget);
    emblemLabel->setBuddy(m_emblem);
    connect(m_removeEmblem, &QPushButton::clicked, this, &TagsEditDialog::removeEmblem); // m_emblem.resetIcon() is not a slot!

    // Make the icon button and the remove button the same height:
    int height = qMax(m_emblem->sizeHint().width(), m_emblem->sizeHint().height());
    height = qMax(height, m_removeEmblem->sizeHint().height());
    m_emblem->setFixedSize(height, height); // Make it square
    m_removeEmblem->setFixedHeight(height);
    m_emblem->resetIcon();

    auto *emblemLayout = new QHBoxLayout(emblemWidget);
    emblemLayout->addWidget(m_emblem);
    emblemLayout->addWidget(m_removeEmblem);
    emblemLayout->addStretch();

    m_backgroundColor = new KColorCombo2(QColor(), palette().color(QPalette::Base), stateWidget);
    auto *backgroundColorLabel = new QLabel(i18n("&Background:"), stateWidget);
    backgroundColorLabel->setBuddy(m_backgroundColor);

    auto *backgroundColorLayout = new QHBoxLayout(nullptr);
    backgroundColorLayout->addWidget(m_backgroundColor);
    backgroundColorLayout->addStretch();

    QIcon boldIconSet = QIcon::fromTheme(QStringLiteral("format-text-bold"));
    m_bold = new QPushButton(boldIconSet, QString(), stateWidget);
    m_bold->setCheckable(true);
    int size = qMax(m_bold->sizeHint().width(), m_bold->sizeHint().height());
    m_bold->setFixedSize(size, size); // Make it square!
    m_bold->setToolTip(i18n("Bold"));

    QIcon underlineIconSet = QIcon::fromTheme(QStringLiteral("format-text-underline"));
    m_underline = new QPushButton(underlineIconSet, QString(), stateWidget);
    m_underline->setCheckable(true);
    m_underline->setFixedSize(size, size); // Make it square!
    m_underline->setToolTip(i18n("Underline"));

    QIcon italicIconSet = QIcon::fromTheme(QStringLiteral("format-text-italic"));
    m_italic = new QPushButton(italicIconSet, QString(), stateWidget);
    m_italic->setCheckable(true);
    m_italic->setFixedSize(size, size); // Make it square!
    m_italic->setToolTip(i18n("Italic"));

    QIcon strikeIconSet = QIcon::fromTheme(QStringLiteral("format-text-strikethrough"));
    m_strike = new QPushButton(strikeIconSet, QString(), stateWidget);
    m_strike->setCheckable(true);
    m_strike->setFixedSize(size, size); // Make it square!
    m_strike->setToolTip(i18n("Strike Through"));

    auto *textLabel = new QLabel(i18n("&Text:"), stateWidget);
    textLabel->setBuddy(m_bold);

    auto *textLayout = new QHBoxLayout;
    textLayout->addWidget(m_bold);
    textLayout->addWidget(m_underline);
    textLayout->addWidget(m_italic);
    textLayout->addWidget(m_strike);
    textLayout->addStretch();

    m_textColor = new KColorCombo2(QColor(), palette().color(QPalette::Text), stateWidget);
    auto *textColorLabel = new QLabel(i18n("Co&lor:"), stateWidget);
    textColorLabel->setBuddy(m_textColor);

    m_font = new QFontComboBox(stateWidget);
    m_font->addItem(i18n("(Default)"), 0);
    auto *fontLabel = new QLabel(i18n("&Font:"), stateWidget);
    fontLabel->setBuddy(m_font);

    m_fontSize = new FontSizeCombo(/*rw=*/true, /*withDefault=*/true, stateWidget);
    auto *fontSizeLabel = new QLabel(i18n("&Size:"), stateWidget);
    fontSizeLabel->setBuddy(m_fontSize);

    m_textEquivalent = new QLineEdit(stateWidget);
    auto *textEquivalentLabel = new QLabel(i18n("Te&xt equivalent:"), stateWidget);
    textEquivalentLabel->setBuddy(m_textEquivalent);
    QFont font = m_textEquivalent->font();
    font.setFamily(QStringLiteral("monospace"));
    m_textEquivalent->setFont(font);

    auto *textEquivalentHelp = new HelpLabel(
        i18n("What is this for?"),
        QStringLiteral("<p>")
            + i18n("When you copy and paste or drag and drop notes to a text editor, this text will be inserted as a textual equivalent of the tag.")
            + QStringLiteral("</p>") +
            //      "<p>" + i18n("If filled, this property lets you paste this tag or this state as textual equivalent.") + "<br>" +
            i18n("For instance, a list of notes with the <b>To Do</b> and <b>Done</b> tags are exported as lines preceded by <b>[ ]</b> or <b>[x]</b>, "
                 "representing an empty checkbox and a checked box.")
            + QStringLiteral("</p>") + QStringLiteral("<p align='center'><img src=\":images/tag_export_help.png\"></p>"),
        stateWidget);
    auto *textEquivalentHelpLayout = new QHBoxLayout;
    textEquivalentHelpLayout->addWidget(textEquivalentHelp);
    textEquivalentHelpLayout->addStretch(255);

    m_onEveryLines = new QCheckBox(i18n("On ever&y line"), stateWidget);

    auto *onEveryLinesHelp = new HelpLabel(
        i18n("What does this mean?"),
        QStringLiteral("<p>")
            + i18n("When a note has several lines, you can choose to export the tag or the state on the first line or on every line of the note.")
            + QStringLiteral("</p>") + QStringLiteral("<p align='center'><img src=\":images/tag_export_on_every_lines_help.png\"></p>") + QStringLiteral("<p>")
            + i18n("In the example above, the tag of the top note is only exported on the first line, while the tag of the bottom note is exported on every "
                   "line of the note."),
        stateWidget);
    auto *onEveryLinesHelpLayout = new QHBoxLayout;
    onEveryLinesHelpLayout->addWidget(onEveryLinesHelp);
    onEveryLinesHelpLayout->addStretch(255);

    auto *textEquivalentGrid = new QGridLayout;
    textEquivalentGrid->addWidget(textEquivalentLabel, 0, 0);
    textEquivalentGrid->addWidget(m_textEquivalent, 0, 1);
    textEquivalentGrid->addLayout(textEquivalentHelpLayout, 0, 2);
    textEquivalentGrid->addWidget(m_onEveryLines, 1, 1);
    textEquivalentGrid->addLayout(onEveryLinesHelpLayout, 1, 2);
    textEquivalentGrid->setColumnStretch(/*col=*/3, /*stretch=*/255);

    auto *separator = new KSeparator(Qt::Horizontal, stateWidget);

    auto *stateGrid = new QGridLayout(stateWidget);
    stateGrid->addWidget(m_stateNameLabel, 0, 0);
    stateGrid->addWidget(m_stateName, 0, 1, 1, 6);
    stateGrid->addWidget(emblemLabel, 1, 0);
    stateGrid->addWidget(emblemWidget, 1, 1, 1, 6);
    stateGrid->addWidget(backgroundColorLabel, 1, 5);
    stateGrid->addLayout(backgroundColorLayout, 1, 6, 1, 1);
    stateGrid->addWidget(textLabel, 2, 0);
    stateGrid->addLayout(textLayout, 2, 1, 1, 4);
    stateGrid->addWidget(textColorLabel, 2, 5);
    stateGrid->addWidget(m_textColor, 2, 6);
    stateGrid->addWidget(fontLabel, 3, 0);
    stateGrid->addWidget(m_font, 3, 1, 1, 4);
    stateGrid->addWidget(fontSizeLabel, 3, 5);
    stateGrid->addWidget(m_fontSize, 3, 6);
    stateGrid->addWidget(separator, 4, 0, 1, 7);
    stateGrid->addLayout(textEquivalentGrid, 5, 0, 1, 7);

    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addWidget(m_tagBox);
    rightLayout->addWidget(m_stateBox);
    rightLayout->addStretch();

    layout->addWidget(rightWidget);
    rightWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

    // Equalize the width of the first column of the two grids:
    int maxWidth = tagNameLabel->sizeHint().width();
    maxWidth = qMax(maxWidth, shortcutLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, m_stateNameLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, emblemLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, textLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, fontLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, backgroundColorLabel->sizeHint().width());
    maxWidth = qMax(maxWidth, textEquivalentLabel->sizeHint().width());

    tagNameLabel->setFixedWidth(maxWidth);
    m_stateNameLabel->setFixedWidth(maxWidth);
    textEquivalentLabel->setFixedWidth(maxWidth);

    // Load Tags:
    for (Tag::List::iterator tagIt = Tag::all.begin(); tagIt != Tag::all.end(); ++tagIt)
        m_tagCopies.append(new TagCopy(*tagIt));

    TagListViewItem *lastInsertedItem = nullptr;
    TagListViewItem *lastInsertedSubItem;
    TagListViewItem *item;
    TagListViewItem *subItem;
    for (TagCopy::List::iterator tagCopyIt = m_tagCopies.begin(); tagCopyIt != m_tagCopies.end(); ++tagCopyIt) {
        // New List View Item:
        if (lastInsertedItem)
            item = new TagListViewItem(m_tags, lastInsertedItem, *tagCopyIt);
        else
            item = new TagListViewItem(m_tags, *tagCopyIt);
        item->setExpanded(true);
        lastInsertedItem = item;
        // Load
        if ((*tagCopyIt)->isMultiState()) {
            lastInsertedSubItem = nullptr;
            StateCopy::List stateCopies = item->tagCopy()->stateCopies;
            for (StateCopy::List::iterator stateCopyIt = stateCopies.begin(); stateCopyIt != stateCopies.end(); ++stateCopyIt) {
                if (lastInsertedSubItem)
                    subItem = new TagListViewItem(item, lastInsertedSubItem, *stateCopyIt);
                else
                    subItem = new TagListViewItem(item, *stateCopyIt);
                lastInsertedSubItem = subItem;
            }
        }
    }

    // Connect Signals:
    connect(m_tagName, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_shortcut, &KShortcutWidget::shortcutChanged, this, &TagsEditDialog::modified);
    connect(m_inherit, &QCheckBox::stateChanged, this, &TagsEditDialog::modified);
    connect(m_allowCrossRefernce, &QCheckBox::clicked, this, &TagsEditDialog::modified);
    connect(m_stateName, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_emblem, &KIconButton::iconChanged, this, &TagsEditDialog::modified);
    connect(m_backgroundColor, &KColorCombo2::colorChanged, this, &TagsEditDialog::modified);
    connect(m_bold, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_underline, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_italic, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_strike, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_textColor, &KColorCombo2::colorChanged, this, &TagsEditDialog::modified);
    connect(m_font, &QFontComboBox::editTextChanged, this, &TagsEditDialog::modified);
    connect(m_fontSize, &FontSizeCombo::editTextChanged, this, &TagsEditDialog::modified);
    connect(m_textEquivalent, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_onEveryLines, &QCheckBox::stateChanged, this, &TagsEditDialog::modified);

    connect(m_tags, &TagListView::currentItemChanged, this, &TagsEditDialog::currentItemChanged);
    connect(m_tags, &TagListView::deletePressed, this, &TagsEditDialog::deleteTag);
    connect(m_tags, &TagListView::doubleClickedItem, this, &TagsEditDialog::renameIt);

    QTreeWidgetItem *firstItem = m_tags->firstChild();
    if (stateToEdit != nullptr) {
        TagListViewItem *item = itemForState(stateToEdit);
        if (item)
            firstItem = item;
    }
    // Select the first tag unless the first tag is a multi-state tag.
    // In this case, select the first state, as it let customize the state AND the associated tag.
    if (firstItem) {
        if (firstItem->childCount() > 0)
            firstItem = firstItem->child(0);
        firstItem->setSelected(true);
        m_tags->setCurrentItem(firstItem);
        currentItemChanged(firstItem);
        if (stateToEdit == nullptr)
            m_tags->scrollToItem(firstItem);
        m_tags->setFocus();
    } else {
        m_moveUp->setEnabled(false);
        m_moveDown->setEnabled(false);
        m_deleteTag->setEnabled(false);
        m_tagBox->setEnabled(false);
        m_stateBox->setEnabled(false);
    }
    // TODO: Disabled both boxes if no tag!!!

    // Some keyboard shortcuts:       // Ctrl+arrows instead of Alt+arrows (same as Go menu in the main window) because Alt+Down is for combo boxes
    auto *selectAbove = new QAction(this);
    selectAbove->setShortcut(Qt::CTRL + Qt::Key_Up);
    connect(selectAbove, &QAction::triggered, this, &TagsEditDialog::selectUp);

    auto *selectBelow = new QAction(this);
    selectBelow->setShortcut(Qt::CTRL + Qt::Key_Down);
    connect(selectBelow, &QAction::triggered, this, &TagsEditDialog::selectDown);

    auto *selectLeft = new QAction(this);
    selectLeft->setShortcut(Qt::CTRL + Qt::Key_Left);
    connect(selectLeft, &QAction::triggered, this, &TagsEditDialog::selectLeft);

    auto *selectRight = new QAction(this);
    selectRight->setShortcut(Qt::CTRL + Qt::Key_Right);
    connect(selectRight, &QAction::triggered, this, &TagsEditDialog::selectRight);

    auto *moveAbove = new QAction(this);
    moveAbove->setShortcut(Qt::CTRL + Qt::Key_Up);
    connect(moveAbove, &QAction::triggered, this, &TagsEditDialog::moveUp);

    auto *moveBelow = new QAction(this);
    moveBelow->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Down);
    connect(moveBelow, &QAction::triggered, this, &TagsEditDialog::moveDown);

    auto *rename = new QAction(this);
    rename->setShortcut(Qt::Key_F2);
    connect(rename, &QAction::triggered, this, &TagsEditDialog::renameIt);

    m_tags->setMinimumSize(m_tags->sizeHint().width() * 2, m_tagBox->sizeHint().height() + m_stateBox->sizeHint().height());

    if (addNewTag)
        QTimer::singleShot(0, this, &TagsEditDialog::newTag);
    else
        // Once the window initial size is computed and the window show, allow the user to resize it down:
        QTimer::singleShot(0, this, &TagsEditDialog::resetTreeSizeHint);
}

TagsEditDialog::~TagsEditDialog() = default;

void TagsEditDialog::resetTreeSizeHint()
{
    m_tags->setMinimumSize(m_tags->sizeHint());
}

TagListViewItem *TagsEditDialog::itemForState(State *state)
{
    // Browse all tags:
    QTreeWidgetItemIterator it(m_tags);
    while (*it) {
        QTreeWidgetItem *item = *it;

        // Return if we found the tag item:
        auto *tagItem = (TagListViewItem *)item;
        if (tagItem->tagCopy() && tagItem->tagCopy()->oldTag && tagItem->tagCopy()->stateCopies[0]->oldState == state)
            return tagItem;

        // Browser all sub-states:
        QTreeWidgetItemIterator it2(item);
        while (*it2) {
            QTreeWidgetItem *subItem = *it2;

            // Return if we found the state item:
            auto *stateItem = (TagListViewItem *)subItem;
            if (stateItem->stateCopy() && stateItem->stateCopy()->oldState && stateItem->stateCopy()->oldState == state)
                return stateItem;

            ++it2;
        }

        ++it;
    }
    return nullptr;
}

void TagsEditDialog::newTag()
{
    // Add to the "model":
    auto *newTagCopy = new TagCopy();
    newTagCopy->stateCopies[0]->newState->setId(QStringLiteral("tag_state_") + QString::number(Tag::getNextStateUid())); // TODO: Check if it's really unique
    m_tagCopies.append(newTagCopy);
    m_addedStates.append(newTagCopy->stateCopies[0]->newState);

    // Add to the "view":
    TagListViewItem *item;
    if (m_tags->firstChild()) {
        // QListView::lastItem is the last item in the tree. If we the last item is a state item, the new tag gets appended to the begin of the list.
        TagListViewItem *last = m_tags->lastItem();
        if (last->parent())
            last = last->parent();
        item = new TagListViewItem(m_tags, last, newTagCopy);
    } else
        item = new TagListViewItem(m_tags, newTagCopy);

    m_deleteTag->setEnabled(true);
    m_tagBox->setEnabled(true);

    // Add to the "controller":
    m_tags->setCurrentItem(item);
    currentItemChanged(item);
    item->setSelected(true);
    m_tagName->setFocus();
}

void TagsEditDialog::newState()
{
    TagListViewItem *tagItem = m_tags->currentItem();
    if (tagItem->parent())
        tagItem = ((TagListViewItem *)(tagItem->parent()));
    tagItem->setExpanded(true); // Show sub-states if we're adding them for the first time!

    State *firstState = tagItem->tagCopy()->stateCopies[0]->newState;

    // Add the first state to the "view". From now on, it's a multi-state tag:
    if (tagItem->childCount() <= 0) {
        firstState->setName(tagItem->tagCopy()->newTag->name());
        // Force emblem to exists for multi-state tags:
        if (firstState->emblem().isEmpty())
            firstState->setEmblem(QStringLiteral("empty"));
        new TagListViewItem(tagItem, tagItem->tagCopy()->stateCopies[0]);
    }

    // Add to the "model":
    auto *newStateCopy = new StateCopy();
    firstState->copyTo(newStateCopy->newState);
    newStateCopy->newState->setId(QStringLiteral("tag_state_") + QString::number(Tag::getNextStateUid())); // TODO: Check if it's really unique
    newStateCopy->newState->setName(QString()); // We copied it too but it's likely the name will not be the same
    tagItem->tagCopy()->stateCopies.append(newStateCopy);
    m_addedStates.append(newStateCopy->newState);

    // Add to the "view":
    auto *item = new TagListViewItem(tagItem, tagItem->lastChild(), newStateCopy);

    // Add to the "controller":
    m_tags->setCurrentItem(item);
    currentItemChanged(item);
    m_stateName->setFocus();
}

void TagsEditDialog::moveUp()
{
    if (!m_moveUp->isEnabled()) // Trggered by keyboard shortcut
        return;

    TagListViewItem *tagItem = m_tags->currentItem();

    // Move in the list view:
    int idx = 0;
    QList<QTreeWidgetItem *> children;
    if (tagItem->parent()) {
        TagListViewItem *parentItem = tagItem->parent();
        idx = parentItem->indexOfChild(tagItem);
        if (idx > 0) {
            tagItem = (TagListViewItem *)parentItem->takeChild(idx);
            children = tagItem->takeChildren();
            parentItem->insertChild(idx - 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    } else {
        idx = m_tags->indexOfTopLevelItem(tagItem);
        if (idx > 0) {
            tagItem = (TagListViewItem *)m_tags->takeTopLevelItem(idx);
            children = tagItem->takeChildren();
            m_tags->insertTopLevelItem(idx - 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    }

    m_tags->setCurrentItem(tagItem);

    // Move in the value list:
    if (tagItem->tagCopy()) {
        int pos = m_tagCopies.indexOf(tagItem->tagCopy());
        m_tagCopies.removeAll(tagItem->tagCopy());
        int i = 0;
        for (TagCopy::List::iterator it = m_tagCopies.begin(); it != m_tagCopies.end(); ++it, ++i)
            if (i == pos - 1) {
                m_tagCopies.insert(it, tagItem->tagCopy());
                break;
            }
    } else {
        StateCopy::List &stateCopies = ((TagListViewItem *)(tagItem->parent()))->tagCopy()->stateCopies;
        int pos = stateCopies.indexOf(tagItem->stateCopy());
        stateCopies.removeAll(tagItem->stateCopy());
        int i = 0;
        for (StateCopy::List::iterator it = stateCopies.begin(); it != stateCopies.end(); ++it, ++i)
            if (i == pos - 1) {
                stateCopies.insert(it, tagItem->stateCopy());
                break;
            }
    }

    ensureCurrentItemVisible();

    m_moveDown->setEnabled(tagItem->nextSibling() != nullptr);
    m_moveUp->setEnabled(tagItem->prevSibling() != nullptr);
}

void TagsEditDialog::moveDown()
{
    if (!m_moveDown->isEnabled()) // Trggered by keyboard shortcut
        return;

    TagListViewItem *tagItem = m_tags->currentItem();

    // Move in the list view:
    int idx = 0;
    QList<QTreeWidgetItem *> children;
    if (tagItem->parent()) {
        TagListViewItem *parentItem = tagItem->parent();
        idx = parentItem->indexOfChild(tagItem);
        if (idx < parentItem->childCount() - 1) {
            children = tagItem->takeChildren();
            tagItem = (TagListViewItem *)parentItem->takeChild(idx);
            parentItem->insertChild(idx + 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    } else {
        idx = m_tags->indexOfTopLevelItem(tagItem);
        if (idx < m_tags->topLevelItemCount() - 1) {
            children = tagItem->takeChildren();
            tagItem = (TagListViewItem *)m_tags->takeTopLevelItem(idx);
            m_tags->insertTopLevelItem(idx + 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    }

    m_tags->setCurrentItem(tagItem);

    // Move in the value list:
    if (tagItem->tagCopy()) {
        uint pos = m_tagCopies.indexOf(tagItem->tagCopy());
        m_tagCopies.removeAll(tagItem->tagCopy());
        if (pos == (uint)m_tagCopies.count() - 1) // Insert at end: iterator does not go there
            m_tagCopies.append(tagItem->tagCopy());
        else {
            uint i = 0;
            for (TagCopy::List::iterator it = m_tagCopies.begin(); it != m_tagCopies.end(); ++it, ++i)
                if (i == pos + 1) {
                    m_tagCopies.insert(it, tagItem->tagCopy());
                    break;
                }
        }
    } else {
        StateCopy::List &stateCopies = ((TagListViewItem *)(tagItem->parent()))->tagCopy()->stateCopies;
        uint pos = stateCopies.indexOf(tagItem->stateCopy());
        stateCopies.removeAll(tagItem->stateCopy());
        if (pos == (uint)stateCopies.count() - 1) // Insert at end: iterator does not go there
            stateCopies.append(tagItem->stateCopy());
        else {
            uint i = 0;
            for (StateCopy::List::iterator it = stateCopies.begin(); it != stateCopies.end(); ++it, ++i)
                if (i == pos + 1) {
                    stateCopies.insert(it, tagItem->stateCopy());
                    break;
                }
        }
    }

    ensureCurrentItemVisible();

    m_moveDown->setEnabled(tagItem->nextSibling() != nullptr);
    m_moveUp->setEnabled(tagItem->prevSibling() != nullptr);
}

void TagsEditDialog::selectUp()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_tags, keyEvent);
}

void TagsEditDialog::selectDown()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_tags, keyEvent);
}

void TagsEditDialog::selectLeft()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_tags, keyEvent);
}

void TagsEditDialog::selectRight()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_tags, keyEvent);
}

void TagsEditDialog::deleteTag()
{
    if (!m_deleteTag->isEnabled())
        return;

    TagListViewItem *item = m_tags->currentItem();

    int result = KMessageBox::Continue;
    if (item->tagCopy() && item->tagCopy()->oldTag)
        result = KMessageBox::warningContinueCancel(this,
                                                    i18n("Deleting the tag will remove it from every note it is currently assigned to."),
                                                    i18n("Confirm Delete Tag"),
                                                    KGuiItem(i18n("Delete Tag"), QStringLiteral("edit-delete")));
    else if (item->stateCopy() && item->stateCopy()->oldState)
        result = KMessageBox::warningContinueCancel(this,
                                                    i18n("Deleting the state will remove the tag from every note the state is currently assigned to."),
                                                    i18n("Confirm Delete State"),
                                                    KGuiItem(i18n("Delete State"), QStringLiteral("edit-delete")));
    if (result != KMessageBox::Continue)
        return;

    if (item->tagCopy()) {
        StateCopy::List stateCopies = item->tagCopy()->stateCopies;
        for (StateCopy::List::iterator stateCopyIt = stateCopies.begin(); stateCopyIt != stateCopies.end(); ++stateCopyIt) {
            StateCopy *stateCopy = *stateCopyIt;
            if (stateCopy->oldState) {
                m_deletedStates.append(stateCopy->oldState);
                m_addedStates.removeAll(stateCopy->oldState);
            }
            m_addedStates.removeAll(stateCopy->newState);
        }
        m_tagCopies.removeAll(item->tagCopy());
        // Remove the new tag, to avoid keyboard-shortcut clashes:
        delete item->tagCopy()->newTag;
    } else {
        TagListViewItem *parentItem = item->parent();
        // Remove the state:
        parentItem->tagCopy()->stateCopies.removeAll(item->stateCopy());
        if (item->stateCopy()->oldState) {
            m_deletedStates.append(item->stateCopy()->oldState);
            m_addedStates.removeAll(item->stateCopy()->oldState);
        }
        m_addedStates.removeAll(item->stateCopy()->newState);
        delete item;
        item = nullptr;
        // Transform to single-state tag if needed:
        if (parentItem->childCount() == 1) {
            delete parentItem->child(0);
            m_tags->setCurrentItem(parentItem);
        }
    }

    delete item;
    if (m_tags->currentItem())
        m_tags->currentItem()->setSelected(true);

    if (!m_tags->firstChild()) {
        m_deleteTag->setEnabled(false);
        m_tagBox->setEnabled(false);
        m_stateBox->setEnabled(false);
    }
}

void TagsEditDialog::renameIt()
{
    if (m_tags->currentItem()->tagCopy())
        m_tagName->setFocus();
    else
        m_stateName->setFocus();
}

void TagsEditDialog::capturedShortcut(const QKeySequence &shortcut)
{
    Q_UNUSED(shortcut);
    // TODO: Validate it!
    // m_shortcut->setShortcut(shortcut);
}

void TagsEditDialog::removeShortcut()
{
    // m_shortcut->setShortcut(QKeySequence());
    modified();
}

void TagsEditDialog::removeEmblem()
{
    m_emblem->resetIcon();
    modified();
}

void TagsEditDialog::modified()
{
    if (m_loading)
        return;

    TagListViewItem *tagItem = m_tags->currentItem();
    if (tagItem == nullptr)
        return;

    if (tagItem->tagCopy()) {
        if (tagItem->tagCopy()->isMultiState()) {
            saveTagTo(tagItem->tagCopy()->newTag);
        } else {
            saveTagTo(tagItem->tagCopy()->newTag);
            saveStateTo(tagItem->tagCopy()->stateCopies[0]->newState);
        }
    } else if (tagItem->stateCopy()) {
        saveTagTo(((TagListViewItem *)(tagItem->parent()))->tagCopy()->newTag);
        saveStateTo(tagItem->stateCopy()->newState);
    }

    m_tags->currentItem()->setup();
    if (m_tags->currentItem()->parent())
        m_tags->currentItem()->parent()->setup();

    m_removeShortcut->setEnabled(!m_shortcut->shortcut().isEmpty());
    m_removeEmblem->setEnabled(!m_emblem->icon().isEmpty() && !m_tags->currentItem()->isEmblemObligatory());
    m_onEveryLines->setEnabled(!m_textEquivalent->text().isEmpty());
}

void TagsEditDialog::currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *nextItem)
{
    Q_UNUSED(nextItem);
    if (item == nullptr)
        return;

    m_loading = true;

    auto *tagItem = (TagListViewItem *)item;
    if (tagItem->tagCopy()) {
        if (tagItem->tagCopy()->isMultiState()) {
            loadTagFrom(tagItem->tagCopy()->newTag);
            loadBlankState();
            m_stateBox->setEnabled(false);
            m_stateBox->setTitle(i18n("State"));
            m_stateNameLabel->setEnabled(true);
            m_stateName->setEnabled(true);
        } else {
            loadTagFrom(tagItem->tagCopy()->newTag); // TODO: No duplicate
            loadStateFrom(tagItem->tagCopy()->stateCopies[0]->newState);
            m_stateBox->setEnabled(true);
            m_stateBox->setTitle(i18n("Appearance"));
            m_stateName->setText(QString());
            m_stateNameLabel->setEnabled(false);
            m_stateName->setEnabled(false);
        }
    } else if (tagItem->stateCopy()) {
        loadTagFrom(((TagListViewItem *)(tagItem->parent()))->tagCopy()->newTag);
        loadStateFrom(tagItem->stateCopy()->newState);
        m_stateBox->setEnabled(true);
        m_stateBox->setTitle(i18n("State"));
        m_stateNameLabel->setEnabled(true);
        m_stateName->setEnabled(true);
    }

    ensureCurrentItemVisible();

    m_loading = false;
}

void TagsEditDialog::ensureCurrentItemVisible()
{
    TagListViewItem *tagItem = m_tags->currentItem();

    // Ensure the tag and the states (if available) to be visible, but if there is a looooot of states,
    // ensure the tag is still visible, even if the last states are not...
    m_tags->scrollToItem(tagItem);

    int idx = 0;

    if (tagItem->parent()) {
        idx = ((QTreeWidgetItem *)tagItem->parent())->indexOfChild(tagItem);
        m_moveDown->setEnabled(idx < ((QTreeWidgetItem *)tagItem->parent())->childCount());
    } else {
        idx = m_tags->indexOfTopLevelItem(tagItem);
        m_moveDown->setEnabled(idx < m_tags->topLevelItemCount());
    }

    m_moveUp->setEnabled(idx > 0);
}

void TagsEditDialog::loadBlankState()
{
    QFont defaultFont;
    m_stateName->setText(QString());
    m_emblem->resetIcon();
    m_removeEmblem->setEnabled(false);
    m_backgroundColor->setColor(QColor());
    m_bold->setChecked(false);
    m_underline->setChecked(false);
    m_italic->setChecked(false);
    m_strike->setChecked(false);
    m_textColor->setColor(QColor());
    // m_font->setCurrentIndex(0);
    m_font->setCurrentFont(defaultFont.family());
    m_fontSize->setCurrentIndex(0);
    m_textEquivalent->setText(QString());
    m_onEveryLines->setChecked(false);
    m_allowCrossRefernce->setChecked(false);
}

void TagsEditDialog::loadStateFrom(State *state)
{
    m_stateName->setText(state->name());
    if (state->emblem().isEmpty())
        m_emblem->resetIcon();
    else
        m_emblem->setIcon(state->emblem());
    m_removeEmblem->setEnabled(!state->emblem().isEmpty() && !m_tags->currentItem()->isEmblemObligatory());
    m_backgroundColor->setColor(state->backgroundColor());
    m_bold->setChecked(state->bold());
    m_underline->setChecked(state->underline());
    m_italic->setChecked(state->italic());
    m_strike->setChecked(state->strikeOut());
    m_textColor->setColor(state->textColor());
    m_textEquivalent->setText(state->textEquivalent());
    m_onEveryLines->setChecked(state->onAllTextLines());
    m_allowCrossRefernce->setChecked(state->allowCrossReferences());

    QFont defaultFont;
    if (state->fontName().isEmpty())
        m_font->setCurrentFont(defaultFont.family());
    else
        m_font->setCurrentFont(state->fontName());

    if (state->fontSize() == -1)
        m_fontSize->setCurrentIndex(0);
    else
        m_fontSize->setItemText(m_fontSize->currentIndex(), QString::number(state->fontSize()));
}

void TagsEditDialog::loadTagFrom(Tag *tag)
{
    m_tagName->setText(tag->name());
    QList<QKeySequence> shortcuts{tag->shortcut()};
    m_shortcut->setShortcut(shortcuts);
    m_removeShortcut->setEnabled(!tag->shortcut().isEmpty());
    m_inherit->setChecked(tag->inheritedBySiblings());
}

void TagsEditDialog::saveStateTo(State *state)
{
    state->setName(m_stateName->text());
    state->setEmblem(m_emblem->icon());
    state->setBackgroundColor(m_backgroundColor->color());
    state->setBold(m_bold->isChecked());
    state->setUnderline(m_underline->isChecked());
    state->setItalic(m_italic->isChecked());
    state->setStrikeOut(m_strike->isChecked());
    state->setTextColor(m_textColor->color());
    state->setTextEquivalent(m_textEquivalent->text());
    state->setOnAllTextLines(m_onEveryLines->isChecked());
    state->setAllowCrossReferences(m_allowCrossRefernce->isChecked());

    if (m_font->currentIndex() == 0)
        state->setFontName(QString());
    else
        state->setFontName(m_font->currentFont().family());

    bool conversionOk;
    int fontSize = m_fontSize->currentText().toInt(&conversionOk);
    if (conversionOk)
        state->setFontSize(fontSize);
    else
        state->setFontSize(-1);
}

void TagsEditDialog::saveTagTo(Tag *tag)
{
    tag->setName(m_tagName->text());

    QKeySequence shortcut;
    if (m_shortcut->shortcut().count() > 0)
        shortcut = m_shortcut->shortcut()[0];
    tag->setShortcut(shortcut);

    tag->setInheritedBySiblings(m_inherit->isChecked());
}

void TagsEditDialog::reject()
{
    // All copies of tag have a shortcut, that is stored as a QAction.
    // So, shortcuts are duplicated, and if the user press one tag keyboard-shortcut in the main window, there is a conflict.
    // We then should delete every copies:
    for (TagCopy::List::iterator tagCopyIt = m_tagCopies.begin(); tagCopyIt != m_tagCopies.end(); ++tagCopyIt) {
        delete (*tagCopyIt)->newTag;
    }

    QDialog::reject();
}

void TagsEditDialog::accept()
{
    Tag::all.clear();
    for (TagCopy::List::iterator tagCopyIt = m_tagCopies.begin(); tagCopyIt != m_tagCopies.end(); ++tagCopyIt) {
        TagCopy *tagCopy = *tagCopyIt;
        // Copy changes to the tag and append in the list of tags::
        if (tagCopy->oldTag) {
            tagCopy->newTag->copyTo(tagCopy->oldTag);
            delete tagCopy->newTag;
        }
        Tag *tag = (tagCopy->oldTag ? tagCopy->oldTag : tagCopy->newTag);
        Tag::all.append(tag);
        // And change all states:
        State::List &states = tag->states();
        StateCopy::List &stateCopies = tagCopy->stateCopies;
        states.clear();
        for (StateCopy::List::iterator stateCopyIt = stateCopies.begin(); stateCopyIt != stateCopies.end(); ++stateCopyIt) {
            StateCopy *stateCopy = *stateCopyIt;
            // Copy changes to the state and append in the list of tags:
            if (stateCopy->oldState)
                stateCopy->newState->copyTo(stateCopy->oldState);
            State *state = (stateCopy->oldState ? stateCopy->oldState : stateCopy->newState);
            states.append(state);
            state->setParentTag(tag);
        }
    }
    Tag::saveTags();
    Tag::updateCaches();

    // Notify removed states and tags, and then remove them:
    if (!m_deletedStates.isEmpty())
        Global::bnpView->removedStates(m_deletedStates);

    // Update every note (change colors, size because of font change or added/removed emblems...):
    Global::bnpView->relayoutAllBaskets();
    Global::bnpView->recomputeAllStyles();

    QDialog::accept();
}

void TagListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //    TagListViewItem* thisItem  = qvariant_cast<TagListViewItem*>(index.data());
    //    qDebug() << "Pointer is: " << thisItem << endl;
    QItemDelegate::paint(painter, option, index);
}

#include "moc_tagsedit.cpp"
