/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <iostream>
// #include <boost/stacktrace.hpp>

#include "note.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QGraphicsView>
#include <QImage>
#include <QList>
#include <QLocale> //For KGLobal::locale(
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOption>
#include <QTimeLine>

#include <KIconLoader>

#include <cmath> // sqrt() and pow() functions
#include <cstdlib> // rand() function

#include "animation.h"
#include "basketscene.h"
#include "common.h"
#include "debugwindow.h"
#include "filter.h"
#include "notefactory.h" // For NoteFactory::filteredURL()
#include "noteselection.h"
#include "settings.h"
#include "tag.h"
#include "tools.h"

/** class Note: */

#define FOR_EACH_CHILD(childVar) for (Note *childVar = firstChild(); childVar; childVar = childVar->next())

class NotePrivate
{
public:
    NotePrivate()
        : prev(nullptr)
        , next(nullptr)
        , width(-1)
        , height(Note::MIN_HEIGHT)
    {
    }
    Note *prev;
    Note *next;
    qreal width;
    qreal height;
};

qreal Note::NOTE_MARGIN = 2;
qreal Note::INSERTION_HEIGHT = 5;
qreal Note::EXPANDER_WIDTH = 9;
qreal Note::EXPANDER_HEIGHT = 9;
qreal Note::GROUP_WIDTH = 2 * NOTE_MARGIN + EXPANDER_WIDTH;
qreal Note::HANDLE_WIDTH = GROUP_WIDTH;
qreal Note::RESIZER_WIDTH = GROUP_WIDTH;
qreal Note::TAG_ARROW_WIDTH = 5;
qreal Note::EMBLEM_SIZE = 16;
qreal Note::MIN_HEIGHT = 2 * NOTE_MARGIN + EMBLEM_SIZE;

Note::Note(BasketScene *parent)
    : QObject(parent)
    , d(new NotePrivate)
    , m_target_x(0)
    , m_target_y(0)
    , m_groupWidth(250)
    , m_isFolded(false)
    , m_firstChild(nullptr)
    , m_parentNote(nullptr)
    , m_basket(parent)
    , m_content(nullptr)
    , m_addedDate(QDateTime::currentDateTime())
    , m_lastModificationDate(QDateTime::currentDateTime())
    , m_computedAreas(false)
    , m_onTop(false)
    , m_hovered(false)
    , m_hoveredZone(Note::None)
    , m_focused(false)
    , m_selected(false)
    , m_wasInLastSelectionRect(false)
    , m_computedState()
    , m_emblemsCount(0)
    , m_haveInvisibleTags(false)
    , m_matching(true)
{
    m_target_x = x();
    m_target_y = y();
    m_animX = new NoteAnimation(this, "x");
    m_animY = new NoteAnimation(this, "y");
    // m_animX->setEasingCurve(QEasingCurve::InOutQuad);
    // m_animY->setEasingCurve(QEasingCurve::InOutQuad);

    connect(m_animX, SIGNAL(valueChanged), this, SLOT(xAnimated));
    connect(m_animY, SIGNAL(valueChanged), this, SLOT(yAnimated));

    setHeight(MIN_HEIGHT);
    if (m_basket) {
        m_basket->addItem(this);
        m_basket->addAnimation(m_animX);
        m_basket->addAnimation(m_animY);
    }
}

Note::~Note()
{
    if (m_basket) {
        if (m_content && m_content->graphicsItem()) {
            m_basket->removeItem(m_content->graphicsItem());
        }
        m_basket->removeAnimation(m_animX);
        m_basket->removeAnimation(m_animY);
        m_basket->removeItem(this);
    }
    delete m_content;
    deleteChilds();
}

void Note::setNext(Note *next)
{
    d->next = next;
}

Note *Note::next() const
{
    return d->next;
}

void Note::setPrev(Note *prev)
{
    d->prev = prev;
}

Note *Note::prev() const
{
    return d->prev;
}

qreal Note::bottom() const
{
    return y() + height() - 1;
}

void Note::setParentBasket(BasketScene *basket)
{
    if (m_basket)
        m_basket->removeItem(this);
    m_basket = basket;
    if (m_basket)
        m_basket->addItem(this);
}

QString Note::addedStringDate()
{
    return m_addedDate.toString();
}

QString Note::lastModificationStringDate()
{
    return m_lastModificationDate.toString();
}

QString Note::toText(const QString &cuttedFullPath)
{
    if (content()) {
        // Convert note to text:
        QString text = content()->toText(cuttedFullPath);
        // If we should not export tags with the text, return immediately:
        if (!Settings::exportTextTags())
            return text;
        // Compute the text equivalent of the tag states:
        QString firstLine;
        QString otherLines;
        for (State::List::Iterator it = m_states.begin(); it != m_states.end(); ++it) {
            if (!(*it)->textEquivalent().isEmpty()) {
                firstLine += (*it)->textEquivalent() + QLatin1Char(' ');
                if ((*it)->onAllTextLines())
                    otherLines += (*it)->textEquivalent() + QLatin1Char(' ');
            }
        }
        // Merge the texts:
        if (firstLine.isEmpty())
            return text;
        if (otherLines.isEmpty())
            return firstLine + text;
        QStringList lines = text.split(QLatin1Char('\n'));
        QString result = firstLine + lines[0] + (lines.count() > 1 ? QStringLiteral("\n") : QString());
        for (int i = 1 /*Skip the first line*/; i < lines.count(); ++i)
            result += otherLines + lines[i] + (i < lines.count() - 1 ? QStringLiteral("\n") : QString());

        return result;
    } else
        return {};
}

bool Note::computeMatching(const FilterData &data)
{
    // Groups are always matching:
    if (!content())
        return true;

    // If we were editing this note and there is a save operation in the middle, then do not hide it suddenly:
    if (basket()->editedNote() == this)
        return true;

    bool matching;
    // First match tags (they are fast to compute):
    switch (data.tagFilterType) {
    default:
    case FilterData::DontCareTagsFilter:
        matching = true;
        break;
    case FilterData::NotTaggedFilter:
        matching = m_states.count() <= 0;
        break;
    case FilterData::TaggedFilter:
        matching = m_states.count() > 0;
        break;
    case FilterData::TagFilter:
        matching = hasTag(data.tag);
        break;
    case FilterData::StateFilter:
        matching = hasState(data.state);
        break;
    }

    // Don't try to match the content text if we are not matching now (the filter is of 'AND' type) or if we shouldn't try to match the string:
    if (matching && !data.string.isEmpty())
        matching = content()->match(data);

    return matching;
}

int Note::newFilter(const FilterData &data)
{
    bool wasMatching = matching();
    m_matching = computeMatching(data);
    setOnTop(wasMatching && matching());
    if (!matching()) {
        setSelected(false);
        hide();
    } else if (!wasMatching) {
        show();
    }

    int countMatches = (content() && matching() ? 1 : 0);

    FOR_EACH_CHILD(child)
    {
        countMatches += child->newFilter(data);
    }

    return countMatches;
}

void Note::deleteSelectedNotes(bool deleteFilesToo, QSet<Note *> *notesToBeDeleted)
{
    if (content()) {
        if (isSelected()) {
            basket()->unplugNote(this);
            if (deleteFilesToo && content()->useFile()) {
                Tools::deleteRecursively(fullPath()); // basket()->deleteFiles(fullPath()); // Also delete the folder if it's a folder
            }
            if (notesToBeDeleted) {
                notesToBeDeleted->insert(this);
            }
        }
        return;
    }

    bool isColumn = this->isColumn();
    Note *child = firstChild();
    Note *next;
    while (child) {
        next = child->next(); // If we delete 'child' on the next line, child->next() will be 0!
        child->deleteSelectedNotes(deleteFilesToo, notesToBeDeleted);
        child = next;
    }

    // if it remains at least two notes, the group must not be deleted
    if (!isColumn && !(firstChild() && firstChild()->next())) {
        if (notesToBeDeleted) {
            notesToBeDeleted->insert(this);
        }
    }
}

int Note::count()
{
    if (content())
        return 1;

    int count = 0;
    FOR_EACH_CHILD(child)
    {
        count += child->count();
    }
    return count;
}

int Note::countDirectChilds()
{
    int count = 0;
    FOR_EACH_CHILD(child)
    {
        ++count;
    }
    return count;
}

QString Note::fullPath()
{
    if (content())
        return basket()->fullPath() + content()->fileName();
    else
        return {};
}

void Note::update()
{
    QGraphicsItem::update(0, 0, boundingRect().width(), boundingRect().height());
}

void Note::setFocused(bool focused)
{
    if (m_focused == focused)
        return;

    m_focused = focused;
    unbufferize();
    update();
}

void Note::setSelected(bool selected)
{
    if (isGroup())
        selected = false; // A group cannot be selected!

    if (m_selected == selected)
        return;

    if (!selected && basket()->editedNote() == this) {
        // basket()->closeEditor();
        return; // To avoid a bug that would count 2 less selected notes instead of 1 less! Because m_selected is modified only below.
    }

    if (selected)
        basket()->addSelectedNote();
    else
        basket()->removeSelectedNote();

    m_selected = selected;
    unbufferize();
    update();
}

void Note::resetWasInLastSelectionRect()
{
    m_wasInLastSelectionRect = false;

    FOR_EACH_CHILD(child)
    {
        child->resetWasInLastSelectionRect();
    }
}

void Note::finishLazyLoad()
{
    if (content())
        content()->finishLazyLoad();

    FOR_EACH_CHILD(child)
    {
        child->finishLazyLoad();
    }
}

void Note::selectIn(const QRectF &rect, bool invertSelection, bool unselectOthers /*= true*/)
{
    //  QRect myRect(x(), y(), width(), height());

    //  bool intersects = myRect.intersects(rect);

    // Only intersects with visible areas.
    // If the note is not visible, the user don't think it will be selected while selecting the note(s) that hide this, so act like the user think:
    bool intersects = false;
    for (QList<QRectF>::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
        QRectF &r = *it;
        if (r.intersects(rect)) {
            intersects = true;
            break;
        }
    }

    bool toSelect = intersects || (!unselectOthers && isSelected());
    if (invertSelection) {
        toSelect = (m_wasInLastSelectionRect == intersects) ? isSelected() : !isSelected();
    }

    setSelected(toSelect);
    m_wasInLastSelectionRect = intersects;

    Note *child = firstChild();
    bool first = true;
    while (child) {
        if ((showSubNotes() || first) && child->matching())
            child->selectIn(rect, invertSelection, unselectOthers);
        else
            child->setSelectedRecursively(false);
        child = child->next();
        first = false;
    }
}

bool Note::allSelected()
{
    if (isGroup()) {
        Note *child = firstChild();
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching())
                if (!child->allSelected())
                    return false;
            ;
            child = child->next();
            first = false;
        }
        return true;
    } else
        return isSelected();
}

void Note::setSelectedRecursively(bool selected)
{
    setSelected(selected && matching());

    FOR_EACH_CHILD(child)
    {
        child->setSelectedRecursively(selected);
    }
}

void Note::invertSelectionRecursively()
{
    if (content())
        setSelected(!isSelected() && matching());

    FOR_EACH_CHILD(child)
    {
        child->invertSelectionRecursively();
    }
}

void Note::unselectAllBut(Note *toSelect)
{
    if (this == toSelect)
        setSelectedRecursively(true);
    else {
        setSelected(false);

        Note *child = firstChild();
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching())
                child->unselectAllBut(toSelect);
            else
                child->setSelectedRecursively(false);
            child = child->next();
            first = false;
        }
    }
}

void Note::invertSelectionOf(Note *toSelect)
{
    if (this == toSelect)
        setSelectedRecursively(!isSelected());
    else {
        Note *child = firstChild();
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching())
                child->invertSelectionOf(toSelect);
            child = child->next();
            first = false;
        }
    }
}

Note *Note::theSelectedNote()
{
    if (!isGroup() && isSelected())
        return this;

    Note *selectedOne;
    Note *child = firstChild();
    while (child) {
        selectedOne = child->theSelectedNote();
        if (selectedOne)
            return selectedOne;
        child = child->next();
    }

    return nullptr;
}

NoteSelection *Note::selectedNotes()
{
    if (content()) {
        if (isSelected())
            return new NoteSelection(this);
        else
            return nullptr;
    }

    auto *selection = new NoteSelection(this);

    FOR_EACH_CHILD(child)
    {
        selection->append(child->selectedNotes());
    }

    if (selection->firstChild) {
        if (selection->firstChild->next)
            return selection;
        else {
            // If 'selection' is a group with only one content, return directly that content:
            NoteSelection *reducedSelection = selection->firstChild;
            //          delete selection; // TODO: Cut all connections of 'selection' before deleting it!
            for (NoteSelection *node = reducedSelection; node; node = node->next)
                node->parent = nullptr;
            return reducedSelection;
        }
    } else {
        delete selection;
        return nullptr;
    }
}

bool Note::isAfter(Note *note)
{
    if (note == nullptr)
        return true;

    Note *next = this;
    while (next) {
        if (next == note)
            return false;
        next = next->nextInStack();
    }
    return true;
}

bool Note::containsNote(Note *note)
{
    //  if (this == note)
    //      return true;

    while (note)
        if (note == this)
            return true;
        else
            note = note->parentNote();

    //  FOR_EACH_CHILD (child)
    //      if (child->containsNote(note))
    //          return true;
    return false;
}

Note *Note::firstRealChild()
{
    Note *child = m_firstChild;
    while (child) {
        if (!child->isGroup() /*&& child->matching()*/)
            return child;
        child = child->firstChild();
    }
    // Empty group:
    return nullptr;
}

Note *Note::lastRealChild()
{
    Note *child = lastChild();
    while (child) {
        if (child->content())
            return child;
        Note *possibleChild = child->lastRealChild();
        if (possibleChild && possibleChild->content())
            return possibleChild;
        child = child->prev();
    }
    return nullptr;
}

Note *Note::lastChild()
{
    Note *child = m_firstChild;
    while (child && child->next())
        child = child->next();

    return child;
}

Note *Note::lastSibling()
{
    Note *last = this;
    while (last && last->next())
        last = last->next();

    return last;
}

qreal Note::yExpander()
{
    Note *child = firstRealChild();
    if (child && !child->isShown())
        child = child->nextShownInStack(); // FIXME: Restrict scope to 'this'

    if (child)
        return (child->boundingRect().height() - EXPANDER_HEIGHT) / 2;
    else // Groups always have at least 2 notes, except for columns which can have no child (but should exists anyway):
        return 0;
}

bool Note::isFree() const
{
    return parentNote() == nullptr && basket() && basket()->isFreeLayout();
}

bool Note::isColumn() const
{
    return parentNote() == nullptr && basket() && basket()->isColumnsLayout();
}

bool Note::hasResizer() const
{
    // "isFree" || "isColumn but not the last"
    return parentNote() == nullptr && ((basket() && basket()->isFreeLayout()) || d->next != nullptr);
}

qreal Note::resizerHeight() const
{
    return (isColumn() ? basket()->sceneRect().height() : d->height);
}

void Note::setHoveredZone(Zone zone) // TODO: Remove setHovered(bool) and assume it is hovered if zone != None !!!!!!!
{
    if (m_hoveredZone != zone) {
        if (content())
            content()->setHoveredZone(m_hoveredZone, zone);
        m_hoveredZone = zone;
        unbufferize();
    }
}

Note::Zone Note::zoneAt(const QPointF &pos, bool toAdd)
{
    // Keep the resizer highlighted when resizong, even if the cursor is over another note:
    if (basket()->resizingNote() == this)
        return Resizer;

    // When dropping/pasting something on a column resizer, add it at the bottom of the column, and don't group it with the whole column:
    if (toAdd && isColumn() && hasResizer()) {
        qreal right = rightLimit() - x();
        if ((pos.x() >= right) && (pos.x() < right + RESIZER_WIDTH) && (pos.y() >= 0) && (pos.y() < resizerHeight())) // Code copied from below
            return BottomColumn;
    }

    // Below a column:
    if (isColumn()) {
        if (pos.y() >= height() && pos.x() < rightLimit() - x())
            return BottomColumn;
    }

    // If toAdd, return only TopInsert, TopGroup, BottomInsert or BottomGroup
    // (by spanning those areas in 4 equal rectangles in the note):
    if (toAdd) {
        if (!isFree() && !Settings::groupOnInsertionLine()) {
            if (pos.y() < height() / 2)
                return TopInsert;
            else
                return BottomInsert;
        }
        if (isColumn() && pos.y() >= height())
            return BottomGroup;
        if (pos.y() < height() / 2)
            if (pos.x() < width() / 2 && !isFree())
                return TopInsert;
            else
                return TopGroup;
        else if (pos.x() < width() / 2 && !isFree())
            return BottomInsert;
        else
            return BottomGroup;
    }

    // If in the resizer:
    if (hasResizer()) {
        qreal right = rightLimit() - x();
        if ((pos.x() >= right) && (pos.x() < right + RESIZER_WIDTH) && (pos.y() >= 0) && (pos.y() < resizerHeight()))
            return Resizer;
    }

    // If isGroup, return only Group, GroupExpander, TopInsert or BottomInsert:
    if (isGroup()) {
        if (pos.y() < INSERTION_HEIGHT) {
            if (isFree())
                return TopGroup;
            else
                return TopInsert;
        }
        if (pos.y() >= height() - INSERTION_HEIGHT) {
            if (isFree())
                return BottomGroup;
            else
                return BottomInsert;
        }
        if (pos.x() >= NOTE_MARGIN && pos.x() < NOTE_MARGIN + EXPANDER_WIDTH) {
            qreal yExp = yExpander();
            if (pos.y() >= yExp && pos.y() < yExp + EXPANDER_HEIGHT)
                return GroupExpander;
        }
        if (pos.x() < width())
            return Group;
        else
            return Note::None;
    }

    // Else, it's a normal note:

    if (pos.x() < HANDLE_WIDTH)
        return Handle;

    if (pos.y() < INSERTION_HEIGHT) {
        if ((!isFree() && !Settings::groupOnInsertionLine()) || (pos.x() < width() / 2 && !isFree()))
            return TopInsert;
        else
            return TopGroup;
    }

    if (pos.y() >= height() - INSERTION_HEIGHT) {
        if ((!isFree() && !Settings::groupOnInsertionLine()) || (pos.x() < width() / 2 && !isFree()))
            return BottomInsert;
        else
            return BottomGroup;
    }

    for (int i = 0; i < m_emblemsCount; i++) {
        if (pos.x() >= HANDLE_WIDTH + (NOTE_MARGIN + EMBLEM_SIZE) * i && pos.x() < HANDLE_WIDTH + (NOTE_MARGIN + EMBLEM_SIZE) * i + NOTE_MARGIN + EMBLEM_SIZE)
            return (Zone)(Emblem0 + i);
    }

    if (pos.x() < HANDLE_WIDTH + (NOTE_MARGIN + EMBLEM_SIZE) * m_emblemsCount + NOTE_MARGIN + TAG_ARROW_WIDTH + NOTE_MARGIN)
        return TagsArrow;

    if (!linkAt(pos).isEmpty())
        return Link;

    int customZone = content()->zoneAt(pos - QPointF(contentX(), NOTE_MARGIN));
    if (customZone)
        return (Note::Zone)customZone;

    return Content;
}

QString Note::linkAt(const QPointF &pos)
{
    QString link = m_content->linkAt(pos - QPointF(contentX(), NOTE_MARGIN));
    if (link.isEmpty() || link.startsWith(QStringLiteral("basket://")))
        return link;
    else
        return NoteFactory::filteredURL(QUrl::fromUserInput(link)).toDisplayString(); // KURIFilter::self()->filteredURI(link);
}

qreal Note::contentX() const
{
    return HANDLE_WIDTH + NOTE_MARGIN + (EMBLEM_SIZE + NOTE_MARGIN) * m_emblemsCount + TAG_ARROW_WIDTH + NOTE_MARGIN;
}

QRectF Note::zoneRect(Note::Zone zone, const QPointF &pos)
{
    if (zone >= Emblem0)
        return QRect(HANDLE_WIDTH + (NOTE_MARGIN + EMBLEM_SIZE) * (zone - Emblem0),
                     INSERTION_HEIGHT,
                     NOTE_MARGIN + EMBLEM_SIZE,
                     height() - 2 * INSERTION_HEIGHT);

    qreal yExp;
    qreal right;
    qreal xGroup = (isFree() ? (isGroup() ? 0 : GROUP_WIDTH) : width() / 2);
    QRectF rect;
    qreal insertSplit = (Settings::groupOnInsertionLine() ? 2 : 1);
    switch (zone) {
    case Note::Handle:
        return {0, 0, HANDLE_WIDTH, d->height};
    case Note::Group:
        yExp = yExpander();
        if (pos.y() < yExp) {
            return {0, INSERTION_HEIGHT, d->width, yExp - INSERTION_HEIGHT};
        }
        if (pos.y() > yExp + EXPANDER_HEIGHT) {
            return {0, yExp + EXPANDER_HEIGHT, d->width, d->height - yExp - EXPANDER_HEIGHT - INSERTION_HEIGHT};
        }
        if (pos.x() < NOTE_MARGIN) {
            return {0, 0, NOTE_MARGIN, d->height};
        } else {
            return {d->width - NOTE_MARGIN, 0, NOTE_MARGIN, d->height};
        }
    case Note::TagsArrow:
        return {HANDLE_WIDTH + (NOTE_MARGIN + EMBLEM_SIZE) * m_emblemsCount,
                INSERTION_HEIGHT,
                NOTE_MARGIN + TAG_ARROW_WIDTH + NOTE_MARGIN,
                d->height - 2 * INSERTION_HEIGHT};
    case Note::Custom0:
    case Note::Content:
        rect = content()->zoneRect(zone, pos - QPointF(contentX(), NOTE_MARGIN));
        rect.translate(contentX(), NOTE_MARGIN);
        return rect.intersected(QRectF(contentX(), INSERTION_HEIGHT, d->width - contentX(), d->height - 2 * INSERTION_HEIGHT)); // Only IN contentRect
    case Note::GroupExpander:
        return {NOTE_MARGIN, yExpander(), EXPANDER_WIDTH, EXPANDER_HEIGHT};
    case Note::Resizer:
        right = rightLimit();
        return {right - x(), 0, RESIZER_WIDTH, resizerHeight()};
    case Note::Link:
    case Note::TopInsert:
        if (isGroup())
            return {0, 0, d->width, INSERTION_HEIGHT};
        else
            return {HANDLE_WIDTH, 0, d->width / insertSplit - HANDLE_WIDTH, INSERTION_HEIGHT};
    case Note::TopGroup:
        return {xGroup, 0, d->width - xGroup, INSERTION_HEIGHT};
    case Note::BottomInsert:
        if (isGroup())
            return {0, d->height - INSERTION_HEIGHT, d->width, INSERTION_HEIGHT};
        else
            return {HANDLE_WIDTH, d->height - INSERTION_HEIGHT, d->width / insertSplit - HANDLE_WIDTH, INSERTION_HEIGHT};
    case Note::BottomGroup:
        return {xGroup, d->height - INSERTION_HEIGHT, d->width - xGroup, INSERTION_HEIGHT};
    case Note::BottomColumn:
        return {0, d->height, rightLimit() - x(), basket()->sceneRect().height() - d->height};
    case Note::None:
        return {/*0, 0, -1, -1*/};
    default:
        return {/*0, 0, -1, -1*/};
    }
}

Qt::CursorShape Note::cursorFromZone(Zone zone) const
{
    switch (zone) {
    case Note::Handle:
    case Note::Group:
        return Qt::SizeAllCursor;
        break;
    case Note::Resizer:
        if (isColumn()) {
            return Qt::SplitHCursor;
        } else {
            return Qt::SizeHorCursor;
        }
        break;

    case Note::Custom0:
        return m_content->cursorFromZone(zone);
        break;

    case Note::Link:
    case Note::TagsArrow:
    case Note::GroupExpander:
        return Qt::PointingHandCursor;
        break;

    case Note::Content:
        return Qt::IBeamCursor;
        break;

    case Note::TopInsert:
    case Note::TopGroup:
    case Note::BottomInsert:
    case Note::BottomGroup:
    case Note::BottomColumn:
        return Qt::CrossCursor;
        break;
    case Note::None:
        return Qt::ArrowCursor;
        break;
    default:
        State *state = stateForEmblemNumber(zone - Emblem0);
        if (state && state->parentTag()->states().count() > 1)
            return Qt::PointingHandCursor;
        else
            return Qt::ArrowCursor;
    }
}

qreal Note::targetX() const
{
    return m_target_x;
}

qreal Note::targetY() const
{
    return m_target_y;
}

qreal Note::height() const
{
    return d->height;
}

void Note::setHeight(qreal height)
{
    setInitialHeight(height);
}

void Note::setInitialHeight(qreal height)
{
    prepareGeometryChange();
    d->height = height;
}

void Note::unsetWidth()
{
    prepareGeometryChange();

    d->width = 0;
    unbufferize();

    FOR_EACH_CHILD(child)
    child->unsetWidth();
}

qreal Note::width() const
{
    return (isGroup() ? (isColumn() ? 0 : GROUP_WIDTH) : d->width);
}

void Note::requestRelayout()
{
    prepareGeometryChange();

    d->width = 0;
    unbufferize();
    basket()->relayoutNotes(); // TODO: A signal that will relayout ONCE and DELAYED if called several times
}

void Note::setWidth(qreal width) // TODO: inline ?
{
    if (d->width != width)
        setWidthForceRelayout(width);
}

void Note::setWidthForceRelayout(qreal width)
{
    prepareGeometryChange();
    unbufferize();
    d->width = (width < minWidth() ? minWidth() : width);
    int contentWidth = width - contentX() - NOTE_MARGIN;
    if (m_content) { ///// FIXME: is this OK?
        if (contentWidth < 1)
            contentWidth = 1;
        if (contentWidth < m_content->minWidth())
            contentWidth = m_content->minWidth();
        setHeight(m_content->setWidthAndGetHeight(contentWidth /* < 1 ? 1 : contentWidth*/) + 2 * NOTE_MARGIN);
        if (d->height < 3 * INSERTION_HEIGHT) // Assure a minimal size...
            setHeight(3 * INSERTION_HEIGHT);
    }
}

qreal Note::minWidth() const
{
    if (m_content)
        return contentX() + m_content->minWidth() + NOTE_MARGIN;
    else
        return GROUP_WIDTH; ///// FIXME: is this OK?
}

qreal Note::minRight()
{
    if (isGroup()) {
        qreal right = x() + width();
        Note *child = firstChild();
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching())
                right = qMax(right, child->minRight());
            child = child->next();
            first = false;
        }
        if (isColumn()) {
            qreal minColumnRight = x() + 2 * HANDLE_WIDTH;
            if (right < minColumnRight)
                return minColumnRight;
        }
        return right;
    } else
        return x() + minWidth();
}

bool Note::toggleFolded()
{
    // Close the editor if it was editing a note that we are about to hide after collapsing:
    if (!m_isFolded && basket() && basket()->isDuringEdit()) {
        if (containsNote(basket()->editedNote()) && firstRealChild() != basket()->editedNote())
            basket()->closeEditor();
    }

    // Important to close the editor FIRST, because else, the last edited note would not show during folding animation (don't ask me why ;-) ):
    m_isFolded = !m_isFolded;

    unbufferize();

    return true;
}

Note *Note::noteAt(QPointF pos)
{
    if (matching() && hasResizer()) {
        int right = rightLimit();
        // TODO: This code is duplicated 3 times: !!!!
        if ((pos.x() >= right) && (pos.x() < right + RESIZER_WIDTH) && (pos.y() >= y()) && (pos.y() < y() + resizerHeight())) {
            if (!m_computedAreas)
                recomputeAreas();
            for (QList<QRectF>::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
                QRectF &rect = *it;
                if (rect.contains(pos.x(), pos.y()))
                    return this;
            }
        }
    }

    if (isGroup()) {
        if ((pos.x() >= x()) && (pos.x() < x() + width()) && (pos.y() >= y()) && (pos.y() < y() + d->height)) {
            if (!m_computedAreas)
                recomputeAreas();
            for (QList<QRectF>::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
                QRectF &rect = *it;
                if (rect.contains(pos.x(), pos.y()))
                    return this;
            }
            return nullptr;
        }
        Note *child = firstChild();
        Note *found;
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching()) {
                found = child->noteAt(pos);
                if (found)
                    return found;
            }
            child = child->next();
            first = false;
        }
    } else if (matching() && pos.y() >= y() && pos.y() < y() + d->height && pos.x() >= x() && pos.x() < x() + d->width) {
        if (!m_computedAreas)
            recomputeAreas();
        for (QList<QRectF>::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
            QRectF &rect = *it;
            if (rect.contains(pos.x(), pos.y()))
                return this;
        }
        return nullptr;
    }

    return nullptr;
}

QRectF Note::boundingRect() const
{
    if (hasResizer()) {
        return {0, 0, rightLimit() - x() + RESIZER_WIDTH, resizerHeight()};
    }
    return {0, 0, width(), height()};
}

QRectF Note::resizerRect()
{
    return {rightLimit(), y(), RESIZER_WIDTH, resizerHeight()};
}

bool Note::showSubNotes()
{
    return !m_isFolded || basket()->isFiltering();
}

void Note::relayoutChildren(qreal ax, qreal ay, bool animate)
{
    // Then, relayout sub-notes (only the first, if the group is folded) and so, assign an height to the group:
    if (isGroup()) {
        qreal h = 0;
        Note *child = firstChild();
        bool first = true;
        while (child) {
            if (child->matching() && (!m_isFolded || first || basket()->isFiltering())) { // Don't use showSubNotes() but use !m_isFolded because we don't want
                                                                                          // a relayout for the animated collapsing notes
                child->relayoutAt(ax + width(), ay + h, animate);
                h += child->height();
                if (!child->isVisible())
                    child->show();
            } else { // In case the user collapse a group, then move it and then expand it:
                child->setXRecursively(x() + width()); //  notes SHOULD have a good X coordinate, and not the old one!
                if (child->isVisible())
                    child->hideRecursively();
            }
            // For future animation when re-match, but on bottom of already matched notes!
            // Find parent primary note and set the Y to THAT y:
            if (!child->matching())
                child->setY(parentPrimaryNote()->y(), animate);
            child = child->next();
            first = false;
        }
        if (height() != h || d->height != h) {
            unbufferize();
            /*if (animate)
                addAnimation(0, 0, h - height());
            else {*/
            setHeight(h);
            unbufferize();
            //}
        }
    } else {
        // If rightLimit is exceeded, set the top-level right limit!!!
        // and NEED RELAYOUT
        setWidth(finalRightLimit() - x());
    }
}

void Note::relayoutAt(qreal ax, qreal ay, bool animate)
{
    if (!matching())
        return;
    qDebug() << "Note::relayoutAt: " << ax << ", " << ay << " : " << animate;
    m_computedAreas = false;
    m_areas.clear();

    // Don't relayout free notes one under the other, because by definition they are freely positioned!
    if (isFree()) {
        ax = targetX();
        ay = targetY();
        // If it's a column, it always have the same "fixed" position (no animation):
    } else if (isColumn()) {
        ax = (prev() ? prev()->rightLimit() + RESIZER_WIDTH : 0);
        ay = 0;
        setX(ax, animate);
        setY(ay, animate);
        // But relayout others vertically if they are inside such primary groups or if it is a "normal" basket:
    } else {
        setX(ax, animate);
        setY(ay, animate);
    }

    relayoutChildren(ax, ay, animate);

    // Set the basket area limits (but not for child notes: no need, because they will look for their parent note):
    if (!parentNote()) {
        if (basket()->tmpWidth < finalRightLimit() + (hasResizer() ? RESIZER_WIDTH : 0))
            basket()->tmpWidth = finalRightLimit() + (hasResizer() ? RESIZER_WIDTH : 0);
        if (basket()->tmpHeight < y() + height())
            basket()->tmpHeight = y() + height();
        // However, if the note exceed the allowed size, let it! :
    } else if (!isGroup()) {
        if (basket()->tmpWidth < x() + width() + (hasResizer() ? RESIZER_WIDTH : 0))
            basket()->tmpWidth = x() + width() + (hasResizer() ? RESIZER_WIDTH : 0);
        if (basket()->tmpHeight < y() + height())
            basket()->tmpHeight = y() + height();
    }
}

void Note::relayoutAt(QPointF pos, bool animate)
{
    relayoutAt(pos.rx(), pos.ry(), animate);
}

void Note::setX(qreal x, bool animate)
{
    if (!animate || !isAnimated()) {
        QGraphicsItemGroup::setX(x);
    } else {
        qDebug() << "Note::setX: " << x << " : " << animate;
        m_target_x = x;
        m_animX->setEndValue(x);
        m_animX->start();
    }
}

void Note::setY(qreal y, bool animate)
{
    if (!animate || !isAnimated()) {
        QGraphicsItemGroup::setY(y);
    } else {
        qDebug() << "Note::setY: " << y << " : " << animate;
        m_target_y = y;
        m_animY->setEndValue(y);
        m_animY->start();
    }
}

void Note::setXRecursively(qreal x, bool animate)
{
    qDebug() << "Note::setXRecursively: " << x << " : " << animate;
    setX(x, animate);

    FOR_EACH_CHILD(child)
    {
        child->setXRecursively(x + width(), animate);
    }
    qDebug() << "Xrecursive done";
}

void Note::setYRecursively(qreal y, bool animate)
{
    qDebug() << "Note::setYRecursively: " << y << " : " << animate;
    setY(y, animate);

    FOR_EACH_CHILD(child)
    {
        child->setYRecursively(y, animate);
    }
    qDebug() << "Yrecursive done";
}

void Note::xAnimated(const QVariant &x)
{
    return;
}

void Note::yAnimated(const QVariant &y)
{
    return;
}

void Note::hideRecursively()
{
    hide();

    FOR_EACH_CHILD(child)
    child->hideRecursively();
}
void Note::setGroupWidth(qreal width)
{
    m_groupWidth = width;
}

qreal Note::groupWidth() const
{
    if (hasResizer())
        return m_groupWidth;
    else
        return rightLimit() - x();
}

qreal Note::rightLimit() const
{
    if (isColumn() && d->next == nullptr) // The last column
        return qMax((x() + minWidth()), (qreal)basket()->graphicsView()->viewport()->width());
    else if (parentNote())
        return parentNote()->rightLimit();
    else
        return x() + m_groupWidth;
}

qreal Note::finalRightLimit() const
{
    if (isColumn() && d->next == nullptr) // The last column
        return qMax(x() + minWidth(), (qreal)basket()->graphicsView()->viewport()->width());
    else if (parentNote())
        return parentNote()->finalRightLimit();
    else
        return x() + m_groupWidth;
}

void Note::drawExpander(QPainter *painter, qreal x, qreal y, const QColor &background, bool expand, BasketScene *basket)
{
    QStyleOption opt;
    opt.state = (expand ? QStyle::State_On : QStyle::State_Off);
    opt.rect = QRect(x, y, 9, 9);
    opt.palette = basket->palette();
    opt.palette.setColor(QPalette::Base, background);

    painter->fillRect(opt.rect, background);

    QStyle *style = basket->style();
    if (!expand) {
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, painter, basket->graphicsView()->viewport());
    } else {
        style->drawPrimitive(QStyle::PE_IndicatorArrowRight, &opt, painter, basket->graphicsView()->viewport());
    }
}

QColor expanderBackground(qreal height, qreal y, const QColor &foreground)
{
    // We will divide height per two, subtract one and use that below a division bar:
    // To avoid division by zero error, height should be bigger than 3.
    // And to avoid y errors or if y is on the borders, we return the border color: the background color.
    if (height <= 3 || y <= 0 || y >= height - 1)
        return foreground;

    const QColor dark = foreground.darker(110); // 1/1.1 of brightness
    const QColor light = foreground.lighter(150); // 50% brighter

    float h1, h2, s1, s2, v1, v2;
    int ng;
    if (y <= (height - 2) / 2) {
        light.getHsvF(&h1, &s1, &v1);
        dark.getHsvF(&h2, &s2, &v2);
        ng = (height - 2) / 2;
        y -= 1;
    } else {
        dark.getHsvF(&h1, &s1, &v1);
        foreground.getHsvF(&h2, &s2, &v2);
        ng = (height - 2) - (height - 2) / 2;
        y -= 1 + (height - 2) / 2;
    }

    return QColor::fromHsvF(h1 + ((h2 - h1) * y) / (ng - 1), s1 + ((s2 - s1) * y) / (ng - 1), v1 + ((v2 - v1) * y) / (ng - 1));
}

void Note::drawHandle(QPainter *painter,
                      qreal x,
                      qreal y,
                      qreal width,
                      qreal height,
                      const QColor &background,
                      const QColor &foreground,
                      const QColor &lightForeground)
{
    const QPen backgroundPen(background);
    const QPen foregroundPen(foreground);

    // Draw the surrounding rectangle:
    painter->setPen(foregroundPen);
    painter->drawLine(0, 0, width - 1, 0);
    painter->drawLine(0, 0, 0, height - 1);
    painter->drawLine(0, height - 1, width - 1, height - 1);

    // Draw the gradients:
    painter->fillRect(1 + x, 1 + y, width - 2, height - 2, lightForeground);

    // Round the top corner with background color:
    painter->setPen(backgroundPen);
    painter->drawLine(0, 0, 0, 3);
    painter->drawLine(1, 0, 3, 0);
    painter->drawPoint(1, 1);
    // Round the bottom corner with background color:
    painter->drawLine(0, height - 1, 0, height - 4);
    painter->drawLine(1, height - 1, 3, height - 1);
    painter->drawPoint(1, height - 2);

    // Surrounding line of the rounded top-left corner:
    painter->setPen(foregroundPen);
    painter->drawLine(1, 2, 1, 3);
    painter->drawLine(2, 1, 3, 1);
    // Surrounding line of the rounded bottom-left corner:
    painter->drawLine(1, height - 3, 1, height - 4);
    painter->drawLine(2, height - 2, 3, height - 2);

    // Anti-aliased rounded top corner (1/2):
    painter->setPen(lightForeground);
    painter->drawPoint(0, 3);
    painter->drawPoint(3, 0);
    painter->drawPoint(2, 2);
    // Anti-aliased rounded bottom corner:
    painter->drawPoint(0, height - 4);
    painter->drawPoint(3, height - 1);
    painter->drawPoint(2, height - 3);

    // Draw the grips:
    const qreal middleHeight = (height - 1) / 2;
    const qreal middleWidth = width / 2;
    painter->fillRect(middleWidth - 2, middleHeight, 2, 2, foreground);
    painter->fillRect(middleWidth + 2, middleHeight, 2, 2, foreground);
    painter->fillRect(middleWidth - 2, middleHeight - 4, 2, 2, foreground);
    painter->fillRect(middleWidth + 2, middleHeight - 4, 2, 2, foreground);
    painter->fillRect(middleWidth - 2, middleHeight + 4, 2, 2, foreground);
    painter->fillRect(middleWidth + 2, middleHeight + 4, 2, 2, foreground);
}

void Note::drawResizer(QPainter *painter, qreal x, qreal y, qreal width, qreal height, const QColor &background, const QColor &foreground, bool rounded)
{
    const QPen backgroundPen(background);
    const QPen foregroundPen(foreground);

    const QColor lightForeground = Tools::mixColor(background, foreground, 2);

    // Draw the surrounding rectangle:
    painter->setPen(foregroundPen);
    painter->fillRect(0, 0, width, height, lightForeground);
    painter->drawLine(0, 0, width - 2, 0);
    painter->drawLine(0, height - 1, width - 2, height - 1);
    painter->drawLine(width - 1, 2, width - 1, height - 2);
    if (isColumn()) {
        painter->drawLine(0, 2, 0, height - 2);
    }

    if (rounded) {
        // Round the top corner with background color:
        painter->setPen(backgroundPen);
        painter->drawLine(width - 1, 0, width - 3, 0);
        painter->drawLine(width - 1, 1, width - 1, 2);
        painter->drawPoint(width - 2, 1);
        // Round the bottom corner with background color:
        painter->drawLine(width - 1, height - 1, width - 1, height - 4);
        painter->drawLine(width - 2, height - 1, width - 4, height - 1);
        painter->drawPoint(width - 2, height - 2);
        // Surrounding line of the rounded top-left corner:
        painter->setPen(foregroundPen);
        painter->drawLine(width - 2, 2, width - 2, 3);
        painter->drawLine(width - 3, 1, width - 4, 1);
        // Surrounding line of the rounded bottom-left corner:
        painter->drawLine(width - 2, height - 3, width - 2, height - 4);
        painter->drawLine(width - 3, height - 2, width - 4, height - 2);

        // Anti-aliased rounded top corner (1/2):
        painter->setPen(Tools::mixColor(foreground, background, 2));
        painter->drawPoint(width - 1, 3);
        painter->drawPoint(width - 4, 0);
        // Anti-aliased rounded bottom corner:
        painter->drawPoint(width - 1, height - 4);
        painter->drawPoint(width - 4, height - 1);
        // Anti-aliased rounded top corner (2/2):
        painter->setPen(foreground);
        painter->drawPoint(width - 3, 2);
        // Anti-aliased rounded bottom corner (2/2):
        painter->drawPoint(width - 3, height - 3);
    }

    // Draw the arrows:
    qreal xArrow = 2;
    qreal hMargin = 9;
    int countArrows = (height >= hMargin * 4 + 6 * 3 ? 3 : (height >= hMargin * 3 + 6 * 2 ? 2 : 1));
    for (int i = 0; i < countArrows; ++i) {
        qreal yArrow;
        switch (countArrows) {
        default:
        case 1:
            yArrow = (height - 6) / 2;
            break;
        case 2:
            yArrow = (i == 1 ? hMargin : height - hMargin - 6);
            break;
        case 3:
            yArrow = (i == 1 ? hMargin : (i == 2 ? (height - 6) / 2 : height - hMargin - 6));
            break;
        }
        /// Dark color:
        painter->setPen(foreground);
        // Left arrow:
        painter->drawLine(xArrow, yArrow + 2, xArrow + 2, yArrow);
        painter->drawLine(xArrow, yArrow + 2, xArrow + 2, yArrow + 4);
        // Right arrow:
        painter->drawLine(width - 1 - xArrow, yArrow + 2, width - 1 - xArrow - 2, yArrow);
        painter->drawLine(width - 1 - xArrow, yArrow + 2, width - 1 - xArrow - 2, yArrow + 4);
        /// Light color:
        painter->setPen(background);
        // Left arrow:
        painter->drawLine(xArrow, yArrow + 2 + 1, xArrow + 2, yArrow + 1);
        painter->drawLine(xArrow, yArrow + 2 + 1, xArrow + 2, yArrow + 4 + 1);
        // Right arrow:
        painter->drawLine(width - 1 - xArrow, yArrow + 2 + 1, width - 1 - xArrow - 2, yArrow + 1);
        painter->drawLine(width - 1 - xArrow, yArrow + 2 + 1, width - 1 - xArrow - 2, yArrow + 4 + 1);
    }
}

void Note::drawInactiveResizer(QPainter *painter, qreal x, qreal y, qreal height, const QColor &background, bool column)
{
    // If background color is too dark, we compute a lighter color instead of a darker:
    QColor darkBgColor = (Tools::tooDark(background) ? background.lighter(120) : background.darker(105));
    painter->fillRect(x, y, RESIZER_WIDTH, height, Tools::mixColor(background, darkBgColor, 2));
}

QPalette Note::palette() const
{
    return (m_basket ? m_basket->palette() : qApp->palette());
}

/* type: 1: topLeft
 *       2: bottomLeft
 *       3: topRight
 *       4: bottomRight
 *       5: fourCorners
 *       6: noteInsideAndOutsideCorners
 * (x,y) relate to the painter origin
 * (width,height) only used for 5:fourCorners type
 */
void Note::drawRoundings(QPainter *painter, qreal x, qreal y, int type, qreal width, qreal height)
{
    qreal right;

    switch (type) {
    case 1:
        x += this->x();
        y += this->y();
        basket()->blendBackground(*painter, QRectF(x, y, 4, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + 1, 2, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + 2, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + 3, 1, 1), this->x(), this->y());
        break;
    case 2:
        x += this->x();
        y += this->y();
        basket()->blendBackground(*painter, QRectF(x, y - 1, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + 1, 2, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + 2, 4, 1), this->x(), this->y());
        break;
    case 3:
        right = rightLimit();
        x += right;
        y += this->y();
        basket()->blendBackground(*painter, QRectF(x - 1, y, 4, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x + 1, y + 1, 2, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x + 2, y + 2, 1, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x + 2, y + 3, 1, 1), right, this->y());
        break;
    case 4:
        right = rightLimit();
        x += right;
        y += this->y();
        basket()->blendBackground(*painter, QRectF(x + 2, y - 1, 1, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x + 2, y, 1, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x + 1, y + 1, 2, 1), right, this->y());
        basket()->blendBackground(*painter, QRectF(x - 1, y + 2, 4, 1), right, this->y());
        break;
    case 5:
        // First make sure the corners are white (depending on the widget style):
        painter->setPen(basket()->backgroundColor());
        painter->drawPoint(x, y);
        painter->drawPoint(x + width - 1, y);
        painter->drawPoint(x + width - 1, y + height - 1);
        painter->drawPoint(x, y + height - 1);
        // And then blend corners:
        x += this->x();
        y += this->y();
        basket()->blendBackground(*painter, QRectF(x, y, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 1, y, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 1, y + height - 1, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x, y + height - 1, 1, 1), this->x(), this->y());
        break;
    case 6:
        x += this->x();
        y += this->y();
        // if (!isSelected()) {
        // Inside left corners:
        basket()->blendBackground(*painter, QRectF(x + HANDLE_WIDTH + 1, y + 1, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + HANDLE_WIDTH, y + 2, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + HANDLE_WIDTH + 1, y + height - 2, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + HANDLE_WIDTH, y + height - 3, 1, 1), this->x(), this->y());
        // Inside right corners:
        basket()->blendBackground(*painter, QRectF(x + width - 4, y + 1, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 3, y + 2, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 4, y + height - 2, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 3, y + height - 3, 1, 1), this->x(), this->y());
        //}
        // Outside right corners:
        basket()->blendBackground(*painter, QRectF(x + width - 1, y, 1, 1), this->x(), this->y());
        basket()->blendBackground(*painter, QRectF(x + width - 1, y + height - 1, 1, 1), this->x(), this->y());
        break;
    }
}

/// Blank Spaces Drawing:

void Note::setOnTop(bool onTop)
{
    setZValue(onTop ? 100 : 0);
    m_onTop = onTop;

    Note *note = firstChild();
    while (note) {
        note->setOnTop(onTop);
        note = note->next();
    }
}

void substractRectOnAreas(const QRectF &rectToSubstract, QList<QRectF> &areas, bool andRemove)
{
    for (int i = 0; i < areas.size();) {
        QRectF &rect = areas[i];
        // Split the rectangle if it intersects with rectToSubstract:
        if (rect.intersects(rectToSubstract)) {
            // Create the top rectangle:
            if (rectToSubstract.top() > rect.top()) {
                areas.insert(i++, QRectF(rect.left(), rect.top(), rect.width(), rectToSubstract.top() - rect.top()));
                rect.setTop(rectToSubstract.top());
            }
            // Create the bottom rectangle:
            if (rectToSubstract.bottom() < rect.bottom()) {
                areas.insert(i++, QRectF(rect.left(), rectToSubstract.bottom(), rect.width(), rect.bottom() - rectToSubstract.bottom()));
                rect.setBottom(rectToSubstract.bottom());
            }
            // Create the left rectangle:
            if (rectToSubstract.left() > rect.left()) {
                areas.insert(i++, QRectF(rect.left(), rect.top(), rectToSubstract.left() - rect.left(), rect.height()));
                rect.setLeft(rectToSubstract.left());
            }
            // Create the right rectangle:
            if (rectToSubstract.right() < rect.right()) {
                areas.insert(i++, QRectF(rectToSubstract.right(), rect.top(), rect.right() - rectToSubstract.right(), rect.height()));
                rect.setRight(rectToSubstract.right());
            }
            // Remove the rectangle if it's entirely contained:
            if (andRemove && rectToSubstract.contains(rect))
                areas.removeAt(i);
            else
                ++i;
        } else
            ++i;
    }
}

void Note::recomputeAreas()
{
    // Initialize the areas with the note rectangle(s):
    m_areas.clear();
    m_areas.append(visibleRect());
    if (hasResizer())
        m_areas.append(resizerRect());

    // Cut the areas where other notes are on top of this note:
    Note *note = basket()->firstNote();
    bool noteIsAfterThis = false;
    while (note) {
        noteIsAfterThis = recomputeAreas(note, noteIsAfterThis);
        note = note->next();
    }
}

bool Note::recomputeAreas(Note *note, bool noteIsAfterThis)
{
    if (note == this)
        noteIsAfterThis = true;
    // Only compute overlapping of notes AFTER this, or ON TOP this:
    // else if ( note->matching() && noteIsAfterThis && (!isOnTop() || (isOnTop() && note->isOnTop())) || (!isOnTop() && note->isOnTop()) ) {
    else if (note->matching() && noteIsAfterThis
             && ((!(isOnTop() || isEditing()) || ((isOnTop() || isEditing()) && (note->isOnTop() || note->isEditing())))
                 || (!(isOnTop() || isEditing()) && (note->isOnTop() || note->isEditing())))) {
        // if (!(isSelected() && !note->isSelected())) { // FIXME: FIXME: FIXME: FIXME: This last condition was added LATE, so we should look if it's ALWAYS
        // good:
        substractRectOnAreas(note->visibleRect(), m_areas, true);
        if (note->hasResizer())
            substractRectOnAreas(note->resizerRect(), m_areas, true);
        //}
    }

    if (note->isGroup()) {
        Note *child = note->firstChild();
        bool first = true;
        while (child) {
            if ((note->showSubNotes() || first) && note->matching())
                noteIsAfterThis = recomputeAreas(child, noteIsAfterThis);
            child = child->next();
            first = false;
        }
    }

    return noteIsAfterThis;
}

bool Note::isEditing()
{
    return basket()->editedNote() == this;
}

/* Drawing policy:
 * ==============
 * - Draw the note on a pixmap and then draw the pixmap on screen is faster and
 *   flicker-free, rather than drawing directly on screen
 * - The next time the pixmap can be directly redrawn on screen without
 *   (relatively low, for small texts) time-consuming text-drawing
 * - To keep memory footprint low, we can destruct the bufferPixmap because
 *   redrawing it offscreen and applying it onscreen is nearly as fast as just
 *   drawing the pixmap onscreen
 * - But as drawing the pixmap offscreen is little time consuming we can keep
 *   last visible notes buffered and then the redraw of the entire window is
 *   INSTANTANEOUS
 * - We keep buffered note/group draws BUT NOT the resizer: such objects are
 *   small and fast to draw, so we don't complexify code for that
 */

void Note::draw(QPainter *painter, const QRectF & /*clipRect*/)
{
    if (!matching())
        return;

    /** Compute visible areas: */
    if (!m_computedAreas)
        recomputeAreas();
    if (m_areas.isEmpty())
        return;

    /** Directly draw pixmap on screen if it is already buffered: */
    if (isBufferized()) {
        drawBufferOnScreen(painter, m_bufferedPixmap);
        return;
    }

    /** If pixmap is Null (size 0), no point in painting: **/
    if (!width() || !height())
        return;

    /** Initialise buffer painter: */
    m_bufferedPixmap = QPixmap(width(), height());
    Q_ASSERT(!m_bufferedPixmap.isNull());
    QPainter painter2(&m_bufferedPixmap);

    /** Initialise colors: */
    QColor baseColor(basket()->backgroundColor());
    QColor highColor(palette().color(QPalette::Highlight));
    QColor midColor = Tools::mixColor(baseColor, highColor, 2);

    /** Initialise brushes and pens: */
    QBrush baseBrush(baseColor);
    QBrush highBrush(highColor);
    QPen basePen(baseColor);
    QPen highPen(highColor);
    QPen midPen(midColor);

    /** Figure out the state of the note: */
    bool hovered = m_hovered && m_hoveredZone != TopInsert && m_hoveredZone != BottomInsert && m_hoveredZone != Resizer;

    /** And then draw the group: */
    if (isGroup()) {
        // Draw background or handle:
        if (hovered) {
            drawHandle(&painter2, 0, 0, width(), height(), baseColor, highColor, midColor);
            drawRoundings(&painter2, 0, 0, /*type=*/1);
            drawRoundings(&painter2, 0, height() - 3, /*type=*/2);
        } else {
            painter2.fillRect(0, 0, width(), height(), baseBrush);
            basket()->blendBackground(painter2, boundingRect().translated(x(), y()), -1, -1, /*opaque=*/true);
        }

        // Draw expander:
        qreal yExp = yExpander();
        drawExpander(&painter2, NOTE_MARGIN, yExp, hovered ? midColor : baseColor, m_isFolded, basket());
        // Draw expander rounded edges:
        if (hovered) {
            QColor color1 = expanderBackground(height(), yExp, highColor);
            QColor color2 = expanderBackground(height(), yExp + EXPANDER_HEIGHT - 1, highColor);
            painter2.setPen(color1);
            painter2.drawPoint(NOTE_MARGIN, yExp);
            painter2.drawPoint(NOTE_MARGIN + 9 - 1, yExp);
            painter2.setPen(color2);
            painter2.drawPoint(NOTE_MARGIN, yExp + 9 - 1);
            painter2.drawPoint(NOTE_MARGIN + 9 - 1, yExp + 9 - 1);
        } else
            drawRoundings(&painter2, NOTE_MARGIN, yExp, /*type=*/5, 9, 9);
        // Draw on screen:
        painter2.end();
        drawBufferOnScreen(painter, m_bufferedPixmap);

        return;
    }

    /** Or draw the note: */
    // What are the background colors:
    QColor background;
    if (m_computedState.backgroundColor().isValid()) {
        background = m_computedState.backgroundColor();
    } else {
        background = basket()->backgroundColor();
    }

    if (!hovered && !isSelected()) {
        // Draw background:
        painter2.fillRect(0, 0, width(), height(), background);
        basket()->blendBackground(painter2, boundingRect().translated(x(), y()));
    } else {
        // Draw selection background:
        painter2.fillRect(0, 0, width(), height(), midColor);
        // Top/Bottom lines:
        painter2.setPen(highPen);
        painter2.drawLine(0, height() - 1, width(), height() - 1);
        painter2.drawLine(0, 0, width(), 0);
        // The handle:
        drawHandle(&painter2, 0, 0, HANDLE_WIDTH, height(), baseColor, highColor, midColor);
        drawRoundings(&painter2, 0, 0, /*type=*/1);
        drawRoundings(&painter2, 0, height() - 3, /*type=*/2);
        painter2.setPen(midColor);
        drawRoundings(&painter2, 0, 0, /*type=*/6, width(), height());
    }

    // Draw the Emblems:
    qreal yIcon = (height() - EMBLEM_SIZE) / 2;
    qreal xIcon = HANDLE_WIDTH + NOTE_MARGIN;
    for (State::List::Iterator it = m_states.begin(); it != m_states.end(); ++it) {
        if (!(*it)->emblem().isEmpty()) {
            QPixmap stateEmblem =
                KIconLoader::global()->loadIcon((*it)->emblem(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, false);

            painter2.drawPixmap(xIcon, yIcon, stateEmblem);
            xIcon += NOTE_MARGIN + EMBLEM_SIZE;
        }
    }

    // Determine the colors (for the richText drawing) and the text color (for the tags arrow too):
    QPalette notePalette(basket()->palette());
    notePalette.setColor(QPalette::Text, (m_computedState.textColor().isValid() ? m_computedState.textColor() : basket()->textColor()));
    notePalette.setColor(QPalette::Base, background);
    if (isSelected())
        notePalette.setColor(QPalette::Text, palette().color(QPalette::HighlightedText));

    // Draw the Tags Arrow:
    if (hovered) {
        QColor textColor = notePalette.color(QPalette::Text);
        QColor light = Tools::mixColor(textColor, background);
        QColor mid = Tools::mixColor(textColor, light);
        painter2.setPen(light); // QPen(basket()->colorGroup().darker().lighter(150)));
        painter2.drawLine(xIcon, yIcon + 6, xIcon + 4, yIcon + 6);
        painter2.setPen(mid); // QPen(basket()->colorGroup().darker()));
        painter2.drawLine(xIcon + 1, yIcon + 7, xIcon + 3, yIcon + 7);
        painter2.setPen(textColor); // QPen(basket()->colorGroup().foreground()));
        painter2.drawPoint(xIcon + 2, yIcon + 8);
    } else if (m_haveInvisibleTags) {
        painter2.setPen(notePalette.color(QPalette::Text) /*QPen(basket()->colorGroup().foreground())*/);
        painter2.drawPoint(xIcon, yIcon + 7);
        painter2.drawPoint(xIcon + 2, yIcon + 7);
        painter2.drawPoint(xIcon + 4, yIcon + 7);
    }

    // Draw on screen:
    painter2.end();
    drawBufferOnScreen(painter, m_bufferedPixmap);
}

void Note::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    if (!m_basket->isLoaded())
        return;

    if (boundingRect().width() <= 0.1 || boundingRect().height() <= 0.1)
        return;

    draw(painter, boundingRect());

    if (hasResizer()) {
        qreal right = rightLimit() - x();
        QRectF resizerRect(0, 0, RESIZER_WIDTH, resizerHeight());
        // Prepare to draw the resizer:
        QPixmap pixmap(RESIZER_WIDTH, resizerHeight());
        QPainter painter2(&pixmap);
        // Draw gradient or resizer:
        if ((m_hovered && m_hoveredZone == Resizer) || ((m_hovered || isSelected()) && !isColumn())) {
            QColor baseColor(basket()->backgroundColor());
            QColor highColor(palette().color(QPalette::Highlight));
            drawResizer(&painter2, 0, 0, RESIZER_WIDTH, resizerHeight(), baseColor, highColor, /*rounded=*/!isColumn());
            if (!isColumn()) {
                drawRoundings(&painter2, RESIZER_WIDTH - 3, 0, /*type=*/3);
                drawRoundings(&painter2, RESIZER_WIDTH - 3, resizerHeight() - 3, /*type=*/4);
            }
        } else {
            drawInactiveResizer(&painter2, /*x=*/0, /*y=*/0, /*height=*/resizerHeight(), basket()->backgroundColor(), isColumn());
            resizerRect.translate(rightLimit(), y());
            basket()->blendBackground(painter2, resizerRect);
        }
        // Draw on screen:
        painter2.end();
        painter->drawPixmap(right, 0, pixmap);
    }
}

void Note::drawBufferOnScreen(QPainter *painter, const QPixmap &contentPixmap)
{
    for (QList<QRectF>::iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
        QRectF rect = (*it).translated(-x(), -y());

        if (rect.x() >= width()) // It's a rect of the resizer, don't draw it!
            continue;

        painter->drawPixmap(rect.x(), rect.y(), contentPixmap, rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void Note::setContent(NoteContent *content)
{
    m_content = content;
}

/*const */ State::List &Note::states() const
{
    return (State::List &)m_states;
}

void Note::addState(State *state, bool orReplace)
{
    if (!content())
        return;

    Tag *tag = state->parentTag();
    State::List::iterator itStates = m_states.begin();
    // Browse all tags, see if the note has it, increment itSates if yes, and then insert the state at this position...
    // For each existing tags:
    for (Tag::List::iterator it = Tag::all.begin(); it != Tag::all.end(); ++it) {
        // If the current tag isn't the one to assign or the current one on the note, go to the next tag:
        if (*it != tag && itStates != m_states.end() && *it != (*itStates)->parentTag())
            continue;
        // We found the tag to insert:
        if (*it == tag) {
            // And the note already have the tag:
            if (itStates != m_states.end() && *it == (*itStates)->parentTag()) {
                // We replace the state if wanted:
                if (orReplace) {
                    itStates = m_states.insert(itStates, state);
                    ++itStates;
                    m_states.erase(itStates);
                    recomputeStyle();
                }
            } else {
                m_states.insert(itStates, state);
                recomputeStyle();
            }
            return;
        }
        // The note has this tag:
        if (itStates != m_states.end() && *it == (*itStates)->parentTag())
            ++itStates;
    }
}

QFont Note::font()
{
    return m_computedState.font(basket()->QGraphicsScene::font());
}

QColor Note::backgroundColor()
{
    if (m_computedState.backgroundColor().isValid())
        return m_computedState.backgroundColor();
    else
        return basket()->backgroundColor();
}

QColor Note::textColor()
{
    if (m_computedState.textColor().isValid())
        return m_computedState.textColor();
    else
        return basket()->textColor();
}

void Note::recomputeStyle()
{
    State::merge(m_states, &m_computedState, &m_emblemsCount, &m_haveInvisibleTags, basket()->backgroundColor());
    //  unsetWidth();
    if (content()) {
        if (content()->graphicsItem())
            content()->graphicsItem()->setPos(contentX(), NOTE_MARGIN);
        content()->fontChanged();
    }
    //  requestRelayout(); // TODO!
}

void Note::recomputeAllStyles()
{
    if (content()) // We do the merge ourself, without calling recomputeStyle(), so there is no infinite recursion:
        // State::merge(m_states, &m_computedState, &m_emblemsCount, &m_haveInvisibleTags, basket()->backgroundColor());
        recomputeStyle();
    else if (isGroup())
        FOR_EACH_CHILD(child)
    child->recomputeAllStyles();
}

bool Note::removedStates(const QList<State *> &deletedStates)
{
    bool modifiedBasket = false;

    if (!states().isEmpty()) {
        for (QList<State *>::const_iterator it = deletedStates.begin(); it != deletedStates.end(); ++it)
            if (hasState(*it)) {
                removeState(*it);
                modifiedBasket = true;
            }
    }

    FOR_EACH_CHILD(child)
    if (child->removedStates(deletedStates))
        modifiedBasket = true;

    return modifiedBasket;
}

void Note::addTag(Tag *tag)
{
    addState(tag->states().first(), /*but do not replace:*/ false);
}

void Note::removeState(State *state)
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if (*it == state) {
            m_states.erase(it);
            recomputeStyle();
            return;
        }
}

void Note::removeTag(Tag *tag)
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if ((*it)->parentTag() == tag) {
            m_states.erase(it);
            recomputeStyle();
            return;
        }
}

void Note::removeAllTags()
{
    m_states.clear();
    recomputeStyle();
}

void Note::addTagToSelectedNotes(Tag *tag)
{
    if (content() && isSelected())
        addTag(tag);

    FOR_EACH_CHILD(child)
    child->addTagToSelectedNotes(tag);
}

void Note::removeTagFromSelectedNotes(Tag *tag)
{
    if (content() && isSelected()) {
        if (hasTag(tag))
            setWidth(0);
        removeTag(tag);
    }

    FOR_EACH_CHILD(child)
    child->removeTagFromSelectedNotes(tag);
}

void Note::removeAllTagsFromSelectedNotes()
{
    if (content() && isSelected()) {
        if (m_states.count() > 0)
            setWidth(0);
        removeAllTags();
    }

    FOR_EACH_CHILD(child)
    child->removeAllTagsFromSelectedNotes();
}

void Note::addStateToSelectedNotes(State *state, bool orReplace)
{
    if (content() && isSelected())
        addState(state, orReplace);

    FOR_EACH_CHILD(child)
    child->addStateToSelectedNotes(state, orReplace); // TODO: BasketScene::addStateToSelectedNotes() does not have orReplace
}

void Note::changeStateOfSelectedNotes(State *state)
{
    if (content() && isSelected() && hasTag(state->parentTag()))
        addState(state);

    FOR_EACH_CHILD(child)
    child->changeStateOfSelectedNotes(state);
}

bool Note::selectedNotesHaveTags()
{
    if (content() && isSelected() && m_states.count() > 0)
        return true;

    FOR_EACH_CHILD(child)
    if (child->selectedNotesHaveTags())
        return true;
    return false;
}

bool Note::hasState(State *state)
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if (*it == state)
            return true;
    return false;
}

bool Note::hasTag(Tag *tag)
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if ((*it)->parentTag() == tag)
            return true;
    return false;
}

State *Note::stateOfTag(Tag *tag)
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if ((*it)->parentTag() == tag)
            return *it;
    return nullptr;
}

bool Note::allowCrossReferences()
{
    for (State::List::iterator it = m_states.begin(); it != m_states.end(); ++it)
        if (!(*it)->allowCrossReferences())
            return false;
    return true;
}

State *Note::stateForEmblemNumber(int number) const
{
    int i = -1;
    for (State::List::const_iterator it = m_states.begin(); it != m_states.end(); ++it)
        if (!(*it)->emblem().isEmpty()) {
            ++i;
            if (i == number)
                return *it;
        }
    return nullptr;
}

bool Note::stateForTagFromSelectedNotes(Tag *tag, State **state)
{
    if (content() && isSelected()) {
        // What state is the tag on this note?
        State *stateOfTag = this->stateOfTag(tag);
        // This tag is not assigned to this note, the action will assign it, then:
        if (stateOfTag == nullptr)
            *state = nullptr;
        else {
            // Take the LOWEST state of all the selected notes:
            // Say the two selected notes have the state "Done" and "To Do".
            // The first note set *state to "Done".
            // When reaching the second note, we should recognize "To Do" is first in the tag states, then take it
            // Because pressing the tag shortcut key should first change state before removing the tag!
            if (*state == nullptr)
                *state = stateOfTag;
            else {
                bool stateIsFirst = true;
                for (State *nextState = stateOfTag->nextState(); nextState; nextState = nextState->nextState(/*cycle=*/false))
                    if (nextState == *state)
                        stateIsFirst = false;
                if (!stateIsFirst)
                    *state = stateOfTag;
            }
        }
        return true; // We encountered a selected note
    }

    bool encounteredSelectedNote = false;
    FOR_EACH_CHILD(child)
    {
        bool encountered = child->stateForTagFromSelectedNotes(tag, state);
        if (encountered && *state == nullptr)
            return true;
        if (encountered)
            encounteredSelectedNote = true;
    }
    return encounteredSelectedNote;
}

void Note::inheritTagsOf(Note *note)
{
    if (!note || !content())
        return;

    for (State::List::iterator it = note->states().begin(); it != note->states().end(); ++it)
        if ((*it)->parentTag() && (*it)->parentTag()->inheritedBySiblings())
            addTag((*it)->parentTag());
}

void Note::unbufferizeAll()
{
    unbufferize();

    if (isGroup()) {
        Note *child = firstChild();
        while (child) {
            child->unbufferizeAll();
            child = child->next();
        }
    }
}

QRectF Note::visibleRect()
{
    QList<QRectF> areas;
    areas.append(QRectF(x(), y(), width(), height()));

    // When we are folding a parent group, if this note is bigger than the first real note of the group, cut the top of this:
    /*Note *parent = parentNote();
    while (parent) {
        if (parent->expandingOrCollapsing())
            substractRectOnAreas(QRect(x(), parent->y() - height(), width(), height()), areas, true);
        parent = parent->parentNote();
    }*/

    if (areas.count() > 0)
        return areas.first();
    else
        return {};
}

void Note::recomputeBlankRects(QList<QRectF> &blankAreas)
{
    if (!matching())
        return;

    // visibleRect() instead of rect() because if we are folding/expanding a smaller parent group, then some part is hidden!
    // But anyway, a resizer is always a primary note and is never hidden by a parent group, so no visibleResizerRect() method!
    substractRectOnAreas(visibleRect(), blankAreas, true);
    if (hasResizer())
        substractRectOnAreas(resizerRect(), blankAreas, true);

    if (isGroup()) {
        Note *child = firstChild();
        bool first = true;
        while (child) {
            if ((showSubNotes() || first) && child->matching())
                child->recomputeBlankRects(blankAreas);
            child = child->next();
            first = false;
        }
    }
}

void Note::linkLookChanged()
{
    if (isGroup()) {
        Note *child = firstChild();
        while (child) {
            child->linkLookChanged();
            child = child->next();
        }
    } else
        content()->linkLookChanged();
}

Note *Note::noteForFullPath(const QString &path)
{
    if (content() && fullPath() == path)
        return this;

    Note *child = firstChild();
    Note *found;
    while (child) {
        found = child->noteForFullPath(path);
        if (found)
            return found;
        child = child->next();
    }
    return nullptr;
}

void Note::listUsedTags(QList<Tag *> &list)
{
    for (State::List::Iterator it = m_states.begin(); it != m_states.end(); ++it) {
        Tag *tag = (*it)->parentTag();
        if (!list.contains(tag))
            list.append(tag);
    }

    FOR_EACH_CHILD(child)
    child->listUsedTags(list);
}

void Note::usedStates(QList<State *> &states)
{
    if (content())
        for (State::List::Iterator it = m_states.begin(); it != m_states.end(); ++it)
            if (!states.contains(*it))
                states.append(*it);

    FOR_EACH_CHILD(child)
    child->usedStates(states);
}

Note *Note::nextInStack()
{
    // First, search in the children:
    if (firstChild()) {
        if (firstChild()->content())
            return firstChild();
        else
            return firstChild()->nextInStack();
    }
    // Then, in the next:
    if (next()) {
        if (next()->content())
            return next();
        else
            return next()->nextInStack();
    }
    // And finally, in the parent:
    Note *note = parentNote();
    while (note)
        if (note->next())
            if (note->next()->content())
                return note->next();
            else
                return note->next()->nextInStack();
        else
            note = note->parentNote();

    // Not found:
    return nullptr;
}

Note *Note::prevInStack()
{
    // First, search in the previous:
    if (prev() && prev()->content())
        return prev();

    // Else, it's a group, get the last item in that group:
    if (prev()) {
        Note *note = prev()->lastRealChild();
        if (note)
            return note;
    }

    if (parentNote())
        return parentNote()->prevInStack();
    else
        return nullptr;
}

Note *Note::nextShownInStack()
{
    Note *next = nextInStack();
    while (next && !next->isShown())
        next = next->nextInStack();
    return next;
}

Note *Note::prevShownInStack()
{
    Note *prev = prevInStack();
    while (prev && !prev->isShown())
        prev = prev->prevInStack();
    return prev;
}

bool Note::isAnimated()
{
    return m_basket && basket()->isAnimated();
}

bool Note::isShown()
{
    // First, the easy one: groups are always shown:
    if (isGroup())
        return true;

    // And another easy part: non-matching notes are hidden:
    if (!matching())
        return false;

    if (basket()->isFiltering()) // And isMatching() because of the line above!
        return true;

    // So, here we go to the complex case: if the note is inside a collapsed group:
    Note *group = parentNote();
    Note *child = this;
    while (group) {
        if (group->isFolded() && group->firstChild() != child)
            return false;
        child = group;
        group = group->parentNote();
    }
    return true;
}

void Note::debug()
{
    qDebug() << "Note@" << (quint64)this;
    if (!this) {
        qDebug();
        return;
    }

    if (isColumn())
        qDebug() << ": Column";
    else if (isGroup())
        qDebug() << ": Group";
    else
        qDebug() << ": Content[" << content()->lowerTypeName() << "]: " << toText(QString());
    qDebug();
}

Note *Note::firstSelected()
{
    if (isSelected())
        return this;

    Note *first = nullptr;
    FOR_EACH_CHILD(child)
    {
        first = child->firstSelected();
        if (first)
            break;
    }
    return first;
}

Note *Note::lastSelected()
{
    if (isSelected())
        return this;

    Note *last = nullptr, *tmp = nullptr;
    FOR_EACH_CHILD(child)
    {
        tmp = child->lastSelected();
        if (tmp)
            last = tmp;
    }
    return last;
}

Note *Note::selectedGroup()
{
    if (isGroup() && allSelected() && count() == basket()->countSelecteds())
        return this;

    FOR_EACH_CHILD(child)
    {
        Note *selectedGroup = child->selectedGroup();
        if (selectedGroup)
            return selectedGroup;
    }

    return nullptr;
}

void Note::groupIn(Note *group)
{
    if (this == group)
        return;

    if (allSelected() && !isColumn()) {
        basket()->unplugNote(this);
        basket()->insertNote(this, group, Note::BottomColumn);
    } else {
        Note *next;
        Note *child = firstChild();
        while (child) {
            next = child->next();
            child->groupIn(group);
            child = next;
        }
    }
}

bool Note::tryExpandParent()
{
    Note *parent = parentNote();
    Note *child = this;
    while (parent) {
        if (parent->firstChild() != child)
            return false;
        if (parent->isColumn())
            return false;
        if (parent->isFolded()) {
            parent->toggleFolded();
            basket()->relayoutNotes();
            return true;
        }
        child = parent;
        parent = parent->parentNote();
    }
    return false;
}

bool Note::tryFoldParent() // TODO: withCtrl  ? withShift  ?
{
    Note *parent = parentNote();
    Note *child = this;
    while (parent) {
        if (parent->firstChild() != child)
            return false;
        if (parent->isColumn())
            return false;
        if (!parent->isFolded()) {
            parent->toggleFolded();
            basket()->relayoutNotes();
            return true;
        }
        child = parent;
        parent = parent->parentNote();
    }
    return false;
}

qreal Note::distanceOnLeftRight(Note *note, int side)
{
    if (side == BasketScene::RIGHT_SIDE) {
        // If 'note' is on left of 'this': cannot switch from this to note by pressing Right key:
        if (x() > note->x() || finalRightLimit() > note->finalRightLimit())
            return -1;
    } else { /*LEFT_SIDE:*/
        // If 'note' is on left of 'this': cannot switch from this to note by pressing Right key:
        if (x() < note->x() || finalRightLimit() < note->finalRightLimit())
            return -1;
    }
    if (x() == note->x() && finalRightLimit() == note->finalRightLimit())
        return -1;

    qreal thisCenterX = x() + (side == BasketScene::LEFT_SIDE ? width() : /*RIGHT_SIDE:*/ 0);
    qreal thisCenterY = y() + height() / 2;
    qreal noteCenterX = note->x() + note->width() / 2;
    qreal noteCenterY = note->y() + note->height() / 2;

    if (thisCenterY > note->bottom())
        noteCenterY = note->bottom();
    else if (thisCenterY < note->y())
        noteCenterY = note->y();
    else
        noteCenterY = thisCenterY;

    qreal angle = 0;
    if (noteCenterX - thisCenterX != 0)
        angle = 1000 * ((noteCenterY - thisCenterY) / (noteCenterX - thisCenterX));
    if (angle < 0)
        angle = -angle;

    return sqrt(pow(noteCenterX - thisCenterX, 2) + pow(noteCenterY - thisCenterY, 2)) + angle;
}

qreal Note::distanceOnTopBottom(Note *note, int side)
{
    if (side == BasketScene::BOTTOM_SIDE) {
        // If 'note' is on left of 'this': cannot switch from this to note by pressing Right key:
        if (y() > note->y() || bottom() > note->bottom())
            return -1;
    } else { /*TOP_SIDE:*/
        // If 'note' is on left of 'this': cannot switch from this to note by pressing Right key:
        if (y() < note->y() || bottom() < note->bottom())
            return -1;
    }
    if (y() == note->y() && bottom() == note->bottom())
        return -1;

    qreal thisCenterX = x() + width() / 2;
    qreal thisCenterY = y() + (side == BasketScene::TOP_SIDE ? height() : /*BOTTOM_SIDE:*/ 0);
    qreal noteCenterX = note->x() + note->width() / 2;
    qreal noteCenterY = note->y() + note->height() / 2;

    if (thisCenterX > note->finalRightLimit())
        noteCenterX = note->finalRightLimit();
    else if (thisCenterX < note->x())
        noteCenterX = note->x();
    else
        noteCenterX = thisCenterX;

    qreal angle = 0;
    if (noteCenterX - thisCenterX != 0)
        angle = 1000 * ((noteCenterY - thisCenterY) / (noteCenterX - thisCenterX));
    if (angle < 0)
        angle = -angle;

    return sqrt(pow(noteCenterX - thisCenterX, 2) + pow(noteCenterY - thisCenterY, 2)) + angle;
}

Note *Note::parentPrimaryNote()
{
    Note *primary = this;
    while (primary->parentNote())
        primary = primary->parentNote();
    return primary;
}

void Note::deleteChilds()
{
    Note *child = firstChild();

    while (child) {
        Note *tmp = child->next();
        delete child;
        child = tmp;
    }
}

bool Note::saveAgain()
{
    bool result = true;

    if (content()) {
        if (!content()->saveToFile())
            result = false;
    }
    FOR_EACH_CHILD(child)
    {
        if (!child->saveAgain())
            result = false;
    }
    if (!result) {
        DEBUG_WIN << QStringLiteral("Note::saveAgain returned false for %1:%2")
                         .arg((content() != nullptr) ? content()->typeName() : QStringLiteral("null"), toText(QString()));
    }
    return result;
}

bool Note::convertTexts()
{
    bool convertedNotes = false;

    if (content() && content()->lowerTypeName() == QStringLiteral("text")) {
        QString text = ((TextContent *)content())->text();
        QString html =
            QStringLiteral(
                "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><meta name=\"qrichtext\" content=\"1\" /></head><body>")
            + Tools::textToHTMLWithoutP(text) + QStringLiteral("</body></html>");
        FileStorage::saveToFile(fullPath(), html);
        setContent(new HtmlContent(this, content()->fileName()));
        convertedNotes = true;
    }

    FOR_EACH_CHILD(child)
    if (child->convertTexts())
        convertedNotes = true;

    return convertedNotes;
}

/* vim: set et sts=4 sw=4 ts=16 tw=78 : */
/* kate: indent-width 4; replace-tabs on; */

#include "moc_note.cpp"
