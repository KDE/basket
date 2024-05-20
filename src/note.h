/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NOTE_H
#define NOTE_H

#include <QGraphicsItemGroup>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QSet>

#include "animation.h"
#include "basket_export.h"
#include "tag.h"

class BasketScene;
struct FilterData;

class NoteContent;
class NoteSelection;

class QPainter;
class QPixmap;
class QString;

class NotePrivate;

/** Handle basket notes and groups!\n
 * After creation, the note is a group. You should create a NoteContent with this Note
 * as constructor parameter to transform it to a note with content. eg:
 * @code
 * Note *note = new Note(basket);   // note is a group!
 * new TextContent(note, fileName); // note is now a note with a text content!
 * new ColorContent(note, Qt::red); // Should never be done!!!!! the old Content should be deleted...
 * @endcode
 * @author Sébastien Laoût
 */
class BASKET_EXPORT Note : public QObject, public QGraphicsItemGroup
{
    Q_OBJECT
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    
    /// CONSTRUCTOR AND DESTRUCTOR:
public:
    explicit Note(BasketScene *parent = nullptr);
    ~Note() override;

private:
    NotePrivate *d;

    /// DOUBLY LINKED LIST:
public:
    void setNext(Note *next);
    void setPrev(Note *prev);
    Note *next() const;
    Note *prev() const;

private:
    qreal m_target_x;
    qreal m_target_y;
public:
    qreal targetX() const;
    qreal targetY() const;
    void setWidth(qreal width);
    void setWidthForceRelayout(qreal width);
    //! Do not use it unless you know what you do!
    void setInitialHeight(qreal height);

    void relayoutChildren(qreal ax, qreal ay, bool animate=false);
    void relayoutAt(QPointF pos, bool animate=false);
    void relayoutAt(qreal ax, qreal ay, bool animate=false);
    void setX(qreal ax, bool animate=false);
    void setY(qreal ay, bool animate=false);
    void setXRecursively(qreal ax, bool animate=false);
    void setYRecursively(qreal ay, bool animate=false);
    
Q_SIGNALS:
    void xChanged();
    void yChanged();
    
private Q_SLOTS:
    void xAnimated(const QVariant &x);
    void yAnimated(const QVariant &y);
    
public:
    void hideRecursively();
    qreal width() const;
    qreal height() const;
    qreal bottom() const;
    QRectF rect();
    QRectF resizerRect();
    QRectF visibleRect();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
    qreal contentX() const;
    qreal minWidth() const;
    qreal minRight();
    void unsetWidth();
    void requestRelayout();
    /** << DO NEVER USE IT!!! Only available when moving notes, groups should be recreated with the exact same state as before! */
    void setHeight(qreal height);

    /// FREE AND COLUMN LAYOUTS MANAGEMENT:
private:
    qreal m_groupWidth;

public:
    qreal groupWidth() const;
    void setGroupWidth(qreal width);
    qreal rightLimit() const;
    qreal finalRightLimit() const;
    bool isFree() const;
    bool isColumn() const;
    bool hasResizer() const;
    qreal resizerHeight() const;

    /// GROUPS MANAGEMENT:
private:
    bool m_isFolded;
    Note *m_firstChild;
    Note *m_parentNote;

public:
    inline bool isGroup() const
    {
        return m_content == nullptr;
    }
    inline bool isFolded()
    {
        return m_isFolded;
    }
    inline Note *firstChild()
    {
        return m_firstChild;
    }
    inline Note *parentNote() const
    {
        return m_parentNote;
    }
    /*inline*/ bool showSubNotes(); //            { return !m_isFolded || !m_collapseFinished; }
    inline void setParentNote(Note *note)
    {
        m_parentNote = note;
    }
    inline void setFirstChild(Note *note)
    {
        m_firstChild = note;
    }
    bool isShown();
    bool toggleFolded();

    Note *noteAt(QPointF pos);
    Note *firstRealChild();
    Note *lastRealChild();
    Note *lastChild();
    Note *lastSibling();
    qreal yExpander();
    bool isAfter(Note *note);
    bool containsNote(Note *note);

    /// NOTES VARIOUS PROPERTIES:       // CONTENT MANAGEMENT?
private:
    BasketScene *m_basket;
    NoteContent *m_content;
    QDateTime m_addedDate;
    QDateTime m_lastModificationDate;

public:
    inline BasketScene *basket() const
    {
        return m_basket;
    }
    inline NoteContent *content()
    {
        return m_content;
    }
    inline void setAddedDate(const QDateTime &dateTime)
    {
        m_addedDate = dateTime;
    }
    inline void setLastModificationDate(const QDateTime &dateTime)
    {
        m_lastModificationDate = dateTime;
    }

    void setParentBasket(BasketScene *basket);

    QDateTime addedDate()
    {
        return m_addedDate;
    }
    QDateTime lastModificationDate()
    {
        return m_lastModificationDate;
    }
    QString addedStringDate();
    QString lastModificationStringDate();
    QString toText(const QString &cuttedFullPath);
    bool saveAgain();
    void deleteChilds();

protected:
    void setContent(NoteContent *content);
    friend class NoteContent;
    friend class AnimationContent;

    /// DRAWING:
private:
    QPixmap m_bufferedPixmap;
    QPixmap m_bufferedSelectionPixmap;

public:
    void draw(QPainter *painter, const QRectF &clipRect);
    void drawBufferOnScreen(QPainter *painter, const QPixmap &contentPixmap);
    static void getGradientColors(const QColor &originalBackground, QColor *colorTop, QColor *colorBottom);
    static void drawExpander(QPainter *painter, qreal x, qreal y, const QColor &background, bool expand, BasketScene *basket);
    void drawHandle(QPainter *painter, qreal x, qreal y, qreal width, qreal height, const QColor &background, const QColor &foreground, const QColor &lightForeground);
    void drawResizer(QPainter *painter, qreal x, qreal y, qreal width, qreal height, const QColor &background, const QColor &foreground, bool rounded);
    void drawRoundings(QPainter *painter, qreal x, qreal y, int type, qreal width = 0, qreal height = 0);
    void unbufferizeAll();
    inline void unbufferize()
    {
        m_bufferedPixmap = QPixmap();
        m_bufferedSelectionPixmap = QPixmap();
    }
    inline bool isBufferized()
    {
        return !m_bufferedPixmap.isNull();
    }
    void recomputeBlankRects(QList<QRectF> &blankAreas);
    static void drawInactiveResizer(QPainter *painter, qreal x, qreal y, qreal height, const QColor &background, bool column);
    QPalette palette() const;

    /// VISIBLE AREAS COMPUTATION:
private:
    QList<QRectF> m_areas;
    bool m_computedAreas;
    bool m_onTop;
    void recomputeAreas();
    bool recomputeAreas(Note *note, bool noteIsAfterThis);

public:
    void setOnTop(bool onTop);
    inline bool isOnTop()
    {
        return m_onTop;
    }
    bool isEditing();

    /// MANAGE ANIMATION:
private:
    NoteAnimation *m_animX;
    NoteAnimation *m_animY;

public:
    // bool initAnimationLoad(QTimeLine *timeLine);
    // void animationFinished();

    /// USER INTERACTION:
public:
    enum Zone {
        None = 0,
        Handle,
        TagsArrow,
        Custom0,
        /*CustomContent1, CustomContent2, */ Content,
        Link,
        TopInsert,
        TopGroup,
        BottomInsert,
        BottomGroup,
        BottomColumn,
        Resizer,
        Group,
        GroupExpander,
        Emblem0
    }; // Emblem0 should be at end, because we add 1 to get Emblem1, 2 to get Emblem2...
    inline void setHovered(bool hovered)
    {
        m_hovered = hovered;
        unbufferize();
    }
    void setHoveredZone(Zone zone);
    inline bool hovered()
    {
        return m_hovered;
    }
    inline Zone hoveredZone()
    {
        return m_hoveredZone;
    }
    Zone zoneAt(const QPointF &pos, bool toAdd = false);
    QRectF zoneRect(Zone zone, const QPointF &pos);
    Qt::CursorShape cursorFromZone(Zone zone) const;
    QString linkAt(const QPointF &pos);

private:
    bool m_hovered;
    Zone m_hoveredZone;

    /// SELECTION:
public:
    NoteSelection *selectedNotes();
    void setSelected(bool selected);
    void setSelectedRecursively(bool selected);
    void invertSelectionRecursively();
    void selectIn(const QRectF &rect, bool invertSelection, bool unselectOthers = true);
    void setFocused(bool focused);
    inline bool isFocused()
    {
        return m_focused;
    }
    inline bool isSelected()
    {
        return m_selected;
    }
    bool allSelected();
    void resetWasInLastSelectionRect();
    void unselectAllBut(Note *toSelect);
    void invertSelectionOf(Note *toSelect);
    Note *theSelectedNote();

private:
    bool m_focused;
    bool m_selected;
    bool m_wasInLastSelectionRect;

    /// TAGS:
private:
    State::List m_states;
    State m_computedState;
    int m_emblemsCount;
    bool m_haveInvisibleTags;

public:
    /*const */ State::List &states() const;
    inline int emblemsCount()
    {
        return m_emblemsCount;
    }
    void addState(State *state, bool orReplace = true);
    void addTag(Tag *tag);
    void removeState(State *state);
    void removeTag(Tag *tag);
    void removeAllTags();
    void addTagToSelectedNotes(Tag *tag);
    void removeTagFromSelectedNotes(Tag *tag);
    void removeAllTagsFromSelectedNotes();
    void addStateToSelectedNotes(State *state, bool orReplace = true);
    void changeStateOfSelectedNotes(State *state);
    bool selectedNotesHaveTags();
    void inheritTagsOf(Note *note);
    bool hasTag(Tag *tag);
    bool hasState(State *state);
    State *stateOfTag(Tag *tag);
    State *stateForEmblemNumber(int number) const;
    bool stateForTagFromSelectedNotes(Tag *tag, State **state);
    void recomputeStyle();
    void recomputeAllStyles();
    bool removedStates(const QList<State *> &deletedStates);
    QFont font();             // Computed!
    QColor backgroundColor(); // Computed!
    QColor textColor();       // Computed!
    bool allowCrossReferences();

    /// FILTERING:
private:
    bool m_matching;

public:
    bool computeMatching(const FilterData &data);
    int newFilter(const FilterData &data);
    bool matching()
    {
        return m_matching;
    }

    /// ADDED:
public:
    /**
     * @return true if this note could be deleted
     **/
    void deleteSelectedNotes(bool deleteFilesToo = true, QSet<Note *> *notesToBeDeleted = nullptr);
    int count();
    int countDirectChilds();

    QString fullPath();
    Note *noteForFullPath(const QString &path);

    // void update();
    void linkLookChanged();

    void usedStates(QList<State *> &states);

    void listUsedTags(QList<Tag *> &list);

    Note *nextInStack();
    Note *prevInStack();
    Note *nextShownInStack();
    Note *prevShownInStack();

    Note *parentPrimaryNote(); // TODO: There are places in the code where this methods is hand-copied / hand-inlined instead of called.

    Note *firstSelected();
    Note *lastSelected();
    Note *selectedGroup();
    void groupIn(Note *group);

    bool tryExpandParent();
    bool tryFoldParent();

    qreal distanceOnLeftRight(Note *note, int side);
    qreal distanceOnTopBottom(Note *note, int side);

    bool convertTexts();

    void debug();

    /// SPEED OPTIMIZATION
public:
    void finishLazyLoad();

public:
    // Values are provided here as info:
    // Please see Settings::setBigNotes() to know whats values are assigned.
    static qreal NOTE_MARGIN /*= 2*/;
    static qreal INSERTION_HEIGHT /*= 5*/;
    static qreal EXPANDER_WIDTH /*= 9*/;
    static qreal EXPANDER_HEIGHT /*= 9*/;
    static qreal GROUP_WIDTH /*= 2*NOTE_MARGIN + EXPANDER_WIDTH*/;
    static qreal HANDLE_WIDTH /*= GROUP_WIDTH*/;
    static qreal RESIZER_WIDTH /*= GROUP_WIDTH*/;
    static qreal TAG_ARROW_WIDTH /*= 5*/;
    static qreal EMBLEM_SIZE /*= 16*/;
    static qreal MIN_HEIGHT /*= 2*NOTE_MARGIN + EMBLEM_SIZE*/;
};

/*
 * Convenience functions:
 */

extern void substractRectOnAreas(const QRectF &rectToSubstract, QList<QRectF> &areas, bool andRemove = true);

#endif // NOTE_H
