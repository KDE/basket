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
#include <QKeyEvent>
#include <QLineEdit>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>

#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShortcutWidget>

#include <algorithm>

#include "bnpview.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "tag.h"
#include "variouswidgets.h" //For FontSizeCombo

#include <basket_debug.h>
#include <ui_tagsedit.h>

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
    , m_ui(new Ui::TagsEditDialog)
    , m_loading(false)
{
    m_ui->setupUi(this);

    QPushButton *okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    okButton->setDefault(true);

    connect(m_ui->newTag, &QPushButton::clicked, this, &TagsEditDialog::newTag);
    connect(m_ui->newState, &QPushButton::clicked, this, &TagsEditDialog::newState);

    connect(m_ui->moveUp, &QPushButton::clicked, this, &TagsEditDialog::moveUp);
    connect(m_ui->moveDown, &QPushButton::clicked, this, &TagsEditDialog::moveDown);
    connect(m_ui->deleteTag, &QPushButton::clicked, this, &TagsEditDialog::deleteTag);

    m_ui->shortcutLabel->setBuddy(m_ui->shortcut);
    // connect(m_shortcut, &KShortcutWidget::shortcutChanged, this, &TagsEditDialog::capturedShortcut);
    connect(m_ui->removeShortcut, &QPushButton::clicked, this, &TagsEditDialog::removeShortcut);

    m_ui->allowCrossReferenceHelp->setMessage(
        QStringLiteral("<p>")
        + i18n("This option will enable you to type a cross reference link directly into a text note. Cross Reference links can have the following syntax:")
        + QStringLiteral("</p>") + QStringLiteral("<p>") + i18n("From the top of the tree (Absolute path):") + QStringLiteral("<br />")
        + i18n("[[/top level item/child|optional title]]") + QStringLiteral("<p>") + QStringLiteral("<p>") + i18n("Relative to the current basket:")
        + QStringLiteral("<br />") + i18n("[[../sibling|optional title]]") + QStringLiteral("<br />") + i18n("[[child|optional title]]")
        + QStringLiteral("<br />") + i18n("[[./child|optional title]]") + QStringLiteral("<p>") + QStringLiteral("<p>")
        + i18n("Baskets matching is cAse inSEnsItive.") + QStringLiteral("</p>"));

    m_ui->emblem->setIconType(KIconLoader::NoGroup, KIconLoader::Action);
    connect(m_ui->removeEmblem, &QPushButton::clicked, this, &TagsEditDialog::removeEmblem); // m_emblem.resetIcon() is not a slot!

    // Make the icon button and the remove button the same height:
    int height = std::max(m_ui->emblem->sizeHint().width(), m_ui->emblem->sizeHint().height());
    height = std::max(height, m_ui->removeEmblem->sizeHint().height());
    m_ui->emblem->setFixedSize(height, height); // Make it square
    m_ui->removeEmblem->setFixedHeight(height);
    m_ui->emblem->resetIcon();

    m_ui->backgroundColor->setDefaultColor(palette().color(QPalette::Base));

    int size = std::max(m_ui->bold->sizeHint().width(), m_ui->bold->sizeHint().height());
    m_ui->bold->setFixedSize(size, size); // Make it square!
    m_ui->underline->setFixedSize(size, size); // Make it square!
    m_ui->italic->setFixedSize(size, size); // Make it square!
    m_ui->strike->setFixedSize(size, size); // Make it square!

    m_ui->textColor->setDefaultColor(palette().color(QPalette::Text));

    m_ui->font->addItem(i18n("(Default)"), 0);

    // Create and place the font size combobox instead of the placeholder combobox:
    m_fontSize = new FontSizeCombo(/*rw=*/true, /*withDefault=*/true, m_ui->stateBox);
    delete m_ui->stateGrid->replaceWidget(m_ui->fontSizePlaceholder, m_fontSize);
    delete m_ui->fontSizePlaceholder;
    m_ui->fontSizePlaceholder = nullptr;
    m_ui->fontSizeLabel->setBuddy(m_fontSize);

    QFont font = m_ui->textEquivalent->font();
    font.setFamily(QStringLiteral("monospace"));
    m_ui->textEquivalent->setFont(font);

    m_ui->textEquivalentHelp->setMessage(
        QStringLiteral("<p>")
        + i18n("When you copy and paste or drag and drop notes to a text editor, this text will be inserted as a textual equivalent of the tag.")
        + QStringLiteral("</p>") +
        //      "<p>" + i18n("If filled, this property lets you paste this tag or this state as textual equivalent.") + "<br>" +
        i18n("For instance, a list of notes with the <b>To Do</b> and <b>Done</b> tags are exported as lines preceded by <b>[ ]</b> or <b>[x]</b>, "
             "representing an empty checkbox and a checked box.")
        + QStringLiteral("</p>") + QStringLiteral("<p align='center'><img src=\":images/tag_export_help.png\"></p>"));

    m_ui->onEveryLinesHelp->setMessage(
        QStringLiteral("<p>")
        + i18n("When a note has several lines, you can choose to export the tag or the state on the first line or on every line of the note.")
        + QStringLiteral("</p>") + QStringLiteral("<p align='center'><img src=\":images/tag_export_on_every_lines_help.png\"></p>") + QStringLiteral("<p>")
        + i18n("In the example above, the tag of the top note is only exported on the first line, while the tag of the bottom note is exported on every "
               "line of the note."));

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
            item = new TagListViewItem(m_ui->tags, lastInsertedItem, *tagCopyIt);
        else
            item = new TagListViewItem(m_ui->tags, *tagCopyIt);
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
    connect(m_ui->tagName, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_ui->shortcut, &KShortcutWidget::shortcutChanged, this, &TagsEditDialog::modified);
    connect(m_ui->inherit, &QCheckBox::stateChanged, this, &TagsEditDialog::modified);
    connect(m_ui->allowCrossRefernce, &QCheckBox::clicked, this, &TagsEditDialog::modified);
    connect(m_ui->stateName, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_ui->emblem, &KIconButton::iconChanged, this, &TagsEditDialog::modified);
    connect(m_ui->backgroundColor, &KColorCombo2::colorChanged, this, &TagsEditDialog::modified);
    connect(m_ui->bold, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_ui->underline, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_ui->italic, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_ui->strike, &QPushButton::toggled, this, &TagsEditDialog::modified);
    connect(m_ui->textColor, &KColorCombo2::colorChanged, this, &TagsEditDialog::modified);
    connect(m_ui->font, &QFontComboBox::editTextChanged, this, &TagsEditDialog::modified);
    connect(m_fontSize, &FontSizeCombo::editTextChanged, this, &TagsEditDialog::modified);
    connect(m_ui->textEquivalent, &QLineEdit::textChanged, this, &TagsEditDialog::modified);
    connect(m_ui->onEveryLines, &QCheckBox::stateChanged, this, &TagsEditDialog::modified);

    connect(m_ui->tags, &TagListView::currentItemChanged, this, &TagsEditDialog::currentItemChanged);
    connect(m_ui->tags, &TagListView::deletePressed, this, &TagsEditDialog::deleteTag);
    connect(m_ui->tags, &TagListView::doubleClickedItem, this, &TagsEditDialog::renameIt);

    QTreeWidgetItem *firstItem = m_ui->tags->firstChild();
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
        m_ui->tags->setCurrentItem(firstItem);
        currentItemChanged(firstItem);
        if (stateToEdit == nullptr)
            m_ui->tags->scrollToItem(firstItem);
        m_ui->tags->setFocus();
    } else {
        m_ui->moveUp->setEnabled(false);
        m_ui->moveDown->setEnabled(false);
        m_ui->deleteTag->setEnabled(false);
        m_ui->tagBox->setEnabled(false);
        m_ui->stateBox->setEnabled(false);
    }
    // TODO: Disabled both boxes if no tag!!!

    // Some keyboard shortcuts:       // Ctrl+arrows instead of Alt+arrows (same as Go menu in the main window) because Alt+Down is for combo boxes
    auto *selectAbove = new QAction(this);
    selectAbove->setShortcut(Qt::CTRL | Qt::Key_Up);
    connect(selectAbove, &QAction::triggered, this, &TagsEditDialog::selectUp);

    auto *selectBelow = new QAction(this);
    selectBelow->setShortcut(Qt::CTRL | Qt::Key_Down);
    connect(selectBelow, &QAction::triggered, this, &TagsEditDialog::selectDown);

    auto *selectLeft = new QAction(this);
    selectLeft->setShortcut(Qt::CTRL | Qt::Key_Left);
    connect(selectLeft, &QAction::triggered, this, &TagsEditDialog::selectLeft);

    auto *selectRight = new QAction(this);
    selectRight->setShortcut(Qt::CTRL | Qt::Key_Right);
    connect(selectRight, &QAction::triggered, this, &TagsEditDialog::selectRight);

    auto *moveAbove = new QAction(this);
    moveAbove->setShortcut(Qt::CTRL | Qt::Key_Up);
    connect(moveAbove, &QAction::triggered, this, &TagsEditDialog::moveUp);

    auto *moveBelow = new QAction(this);
    moveBelow->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Down);
    connect(moveBelow, &QAction::triggered, this, &TagsEditDialog::moveDown);

    auto *rename = new QAction(this);
    rename->setShortcut(Qt::Key_F2);
    connect(rename, &QAction::triggered, this, &TagsEditDialog::renameIt);

    m_ui->tags->setMinimumSize(m_ui->tags->sizeHint().width() * 2, m_ui->tagBox->sizeHint().height() + m_ui->stateBox->sizeHint().height());

    if (addNewTag)
        QTimer::singleShot(0, this, &TagsEditDialog::newTag);
    else
        // Once the window initial size is computed and the window show, allow the user to resize it down:
        QTimer::singleShot(0, this, &TagsEditDialog::resetTreeSizeHint);
}

TagsEditDialog::~TagsEditDialog()
{
    delete m_ui;
}

void TagsEditDialog::resetTreeSizeHint()
{
    m_ui->tags->setMinimumSize(m_ui->tags->sizeHint());
}

TagListViewItem *TagsEditDialog::itemForState(State *state)
{
    // Browse all tags:
    QTreeWidgetItemIterator it(m_ui->tags);
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
    if (m_ui->tags->firstChild()) {
        // QListView::lastItem is the last item in the tree. If we the last item is a state item, the new tag gets appended to the begin of the list.
        TagListViewItem *last = m_ui->tags->lastItem();
        if (last->parent())
            last = last->parent();
        item = new TagListViewItem(m_ui->tags, last, newTagCopy);
    } else
        item = new TagListViewItem(m_ui->tags, newTagCopy);

    m_ui->deleteTag->setEnabled(true);
    m_ui->tagBox->setEnabled(true);

    // Add to the "controller":
    m_ui->tags->setCurrentItem(item);
    currentItemChanged(item);
    item->setSelected(true);
    m_ui->tagName->setFocus();
}

void TagsEditDialog::newState()
{
    TagListViewItem *tagItem = m_ui->tags->currentItem();
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
    m_ui->tags->setCurrentItem(item);
    currentItemChanged(item);
    m_ui->stateName->setFocus();
}

void TagsEditDialog::moveUp()
{
    if (!m_ui->moveUp->isEnabled()) // Triggered by keyboard shortcut
        return;

    TagListViewItem *tagItem = m_ui->tags->currentItem();

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
        idx = m_ui->tags->indexOfTopLevelItem(tagItem);
        if (idx > 0) {
            tagItem = (TagListViewItem *)m_ui->tags->takeTopLevelItem(idx);
            children = tagItem->takeChildren();
            m_ui->tags->insertTopLevelItem(idx - 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    }

    m_ui->tags->setCurrentItem(tagItem);

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

    m_ui->moveDown->setEnabled(tagItem->nextSibling() != nullptr);
    m_ui->moveUp->setEnabled(tagItem->prevSibling() != nullptr);
}

void TagsEditDialog::moveDown()
{
    if (!m_ui->moveDown->isEnabled()) // Triggered by keyboard shortcut
        return;

    TagListViewItem *tagItem = m_ui->tags->currentItem();

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
        idx = m_ui->tags->indexOfTopLevelItem(tagItem);
        if (idx < m_ui->tags->topLevelItemCount() - 1) {
            children = tagItem->takeChildren();
            tagItem = (TagListViewItem *)m_ui->tags->takeTopLevelItem(idx);
            m_ui->tags->insertTopLevelItem(idx + 1, tagItem);
            tagItem->insertChildren(0, children);
            tagItem->setExpanded(true);
        }
    }

    m_ui->tags->setCurrentItem(tagItem);

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

    m_ui->moveDown->setEnabled(tagItem->nextSibling() != nullptr);
    m_ui->moveUp->setEnabled(tagItem->prevSibling() != nullptr);
}

void TagsEditDialog::selectUp()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_ui->tags, keyEvent);
}

void TagsEditDialog::selectDown()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_ui->tags, keyEvent);
}

void TagsEditDialog::selectLeft()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_ui->tags, keyEvent);
}

void TagsEditDialog::selectRight()
{
    auto *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::KeyboardModifiers(), QString());
    QApplication::postEvent(m_ui->tags, keyEvent);
}

void TagsEditDialog::deleteTag()
{
    if (!m_ui->deleteTag->isEnabled())
        return;

    TagListViewItem *item = m_ui->tags->currentItem();

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
            m_ui->tags->setCurrentItem(parentItem);
        }
    }

    delete item;
    if (m_ui->tags->currentItem())
        m_ui->tags->currentItem()->setSelected(true);

    if (!m_ui->tags->firstChild()) {
        m_ui->deleteTag->setEnabled(false);
        m_ui->tagBox->setEnabled(false);
        m_ui->stateBox->setEnabled(false);
    }
}

void TagsEditDialog::renameIt()
{
    if (m_ui->tags->currentItem()->tagCopy())
        m_ui->tagName->setFocus();
    else
        m_ui->stateName->setFocus();
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
    m_ui->emblem->resetIcon();
    modified();
}

void TagsEditDialog::modified()
{
    if (m_loading)
        return;

    TagListViewItem *tagItem = m_ui->tags->currentItem();
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

    m_ui->tags->currentItem()->setup();
    if (m_ui->tags->currentItem()->parent())
        m_ui->tags->currentItem()->parent()->setup();

    m_ui->removeShortcut->setEnabled(!m_ui->shortcut->shortcut().isEmpty());
    m_ui->removeEmblem->setEnabled(!m_ui->emblem->icon().isEmpty() && !m_ui->tags->currentItem()->isEmblemObligatory());
    m_ui->onEveryLines->setEnabled(!m_ui->textEquivalent->text().isEmpty());
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
            m_ui->stateBox->setEnabled(false);
            m_ui->stateBox->setTitle(i18n("State"));
            m_ui->stateNameLabel->setEnabled(true);
            m_ui->stateName->setEnabled(true);
        } else {
            loadTagFrom(tagItem->tagCopy()->newTag); // TODO: No duplicate
            loadStateFrom(tagItem->tagCopy()->stateCopies[0]->newState);
            m_ui->stateBox->setEnabled(true);
            m_ui->stateBox->setTitle(i18n("Appearance"));
            m_ui->stateName->setText(QString());
            m_ui->stateNameLabel->setEnabled(false);
            m_ui->stateName->setEnabled(false);
        }
    } else if (tagItem->stateCopy()) {
        loadTagFrom(((TagListViewItem *)(tagItem->parent()))->tagCopy()->newTag);
        loadStateFrom(tagItem->stateCopy()->newState);
        m_ui->stateBox->setEnabled(true);
        m_ui->stateBox->setTitle(i18n("State"));
        m_ui->stateNameLabel->setEnabled(true);
        m_ui->stateName->setEnabled(true);
    }

    ensureCurrentItemVisible();

    m_loading = false;
}

void TagsEditDialog::ensureCurrentItemVisible()
{
    TagListViewItem *tagItem = m_ui->tags->currentItem();

    // Ensure the tag and the states (if available) to be visible, but if there is a looooot of states,
    // ensure the tag is still visible, even if the last states are not...
    m_ui->tags->scrollToItem(tagItem);

    int idx = 0;

    if (tagItem->parent()) {
        idx = ((QTreeWidgetItem *)tagItem->parent())->indexOfChild(tagItem);
        m_ui->moveDown->setEnabled(idx < ((QTreeWidgetItem *)tagItem->parent())->childCount());
    } else {
        idx = m_ui->tags->indexOfTopLevelItem(tagItem);
        m_ui->moveDown->setEnabled(idx < m_ui->tags->topLevelItemCount());
    }

    m_ui->moveUp->setEnabled(idx > 0);
}

void TagsEditDialog::loadBlankState()
{
    QFont defaultFont;
    m_ui->stateName->setText(QString());
    m_ui->emblem->resetIcon();
    m_ui->removeEmblem->setEnabled(false);
    m_ui->backgroundColor->setColor(QColor());
    m_ui->bold->setChecked(false);
    m_ui->underline->setChecked(false);
    m_ui->italic->setChecked(false);
    m_ui->strike->setChecked(false);
    m_ui->textColor->setColor(QColor());
    // m_font->setCurrentIndex(0);
    m_ui->font->setCurrentFont(defaultFont.family());
    m_fontSize->setCurrentIndex(0);
    m_ui->textEquivalent->setText(QString());
    m_ui->onEveryLines->setChecked(false);
    m_ui->allowCrossRefernce->setChecked(false);
}

void TagsEditDialog::loadStateFrom(State *state)
{
    m_ui->stateName->setText(state->name());
    if (state->emblem().isEmpty())
        m_ui->emblem->resetIcon();
    else
        m_ui->emblem->setIcon(state->emblem());
    m_ui->removeEmblem->setEnabled(!state->emblem().isEmpty() && !m_ui->tags->currentItem()->isEmblemObligatory());
    m_ui->backgroundColor->setColor(state->backgroundColor());
    m_ui->bold->setChecked(state->bold());
    m_ui->underline->setChecked(state->underline());
    m_ui->italic->setChecked(state->italic());
    m_ui->strike->setChecked(state->strikeOut());
    m_ui->textColor->setColor(state->textColor());
    m_ui->textEquivalent->setText(state->textEquivalent());
    m_ui->onEveryLines->setChecked(state->onAllTextLines());
    m_ui->allowCrossRefernce->setChecked(state->allowCrossReferences());

    QFont defaultFont;
    if (state->fontName().isEmpty())
        m_ui->font->setCurrentFont(defaultFont.family());
    else
        m_ui->font->setCurrentFont(state->fontName());

    if (state->fontSize() == -1)
        m_fontSize->setCurrentIndex(0);
    else
        m_fontSize->setItemText(m_fontSize->currentIndex(), QString::number(state->fontSize()));
}

void TagsEditDialog::loadTagFrom(Tag *tag)
{
    m_ui->tagName->setText(tag->name());
    QList<QKeySequence> shortcuts{tag->shortcut()};
    m_ui->shortcut->setShortcut(shortcuts);
    m_ui->removeShortcut->setEnabled(!tag->shortcut().isEmpty());
    m_ui->inherit->setChecked(tag->inheritedBySiblings());
}

void TagsEditDialog::saveStateTo(State *state)
{
    state->setName(m_ui->stateName->text());
    state->setEmblem(m_ui->emblem->icon());
    state->setBackgroundColor(m_ui->backgroundColor->color());
    state->setBold(m_ui->bold->isChecked());
    state->setUnderline(m_ui->underline->isChecked());
    state->setItalic(m_ui->italic->isChecked());
    state->setStrikeOut(m_ui->strike->isChecked());
    state->setTextColor(m_ui->textColor->color());
    state->setTextEquivalent(m_ui->textEquivalent->text());
    state->setOnAllTextLines(m_ui->onEveryLines->isChecked());
    state->setAllowCrossReferences(m_ui->allowCrossRefernce->isChecked());

    if (m_ui->font->currentIndex() == 0)
        state->setFontName(QString());
    else
        state->setFontName(m_ui->font->currentFont().family());

    bool conversionOk;
    int fontSize = m_fontSize->currentText().toInt(&conversionOk);
    if (conversionOk)
        state->setFontSize(fontSize);
    else
        state->setFontSize(-1);
}

void TagsEditDialog::saveTagTo(Tag *tag)
{
    tag->setName(m_ui->tagName->text());

    QKeySequence shortcut;
    if (m_ui->shortcut->shortcut().count() > 0)
        shortcut = m_ui->shortcut->shortcut()[0];
    tag->setShortcut(shortcut);

    tag->setInheritedBySiblings(m_ui->inherit->isChecked());
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

#include "moc_tagsedit.cpp"
