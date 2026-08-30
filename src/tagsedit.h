/**
 * SPDX-FileCopyrightText: (C) 2005 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TAGEDIT_H
#define TAGEDIT_H

#include <QDialog>
#include <QList>
#include <QTreeWidgetItem>

#include "tag.h"

class QTreeWidget;

class QKeyEvent;
class QMouseEvent;

class QKeySequence;

class FontSizeCombo;

namespace Ui
{
class TagsEditDialog;
}

class StateCopy
{
public:
    using List = QList<StateCopy *>;
    explicit StateCopy(State *old = nullptr);
    ~StateCopy();
    State *oldState;
    State *newState;
    void copyBack();
};

class TagCopy
{
public:
    using List = QList<TagCopy *>;
    explicit TagCopy(Tag *old = nullptr);
    ~TagCopy();
    Tag *oldTag;
    Tag *newTag;
    StateCopy::List stateCopies;
    void copyBack();
    bool isMultiState();
};

class TagListViewItem : public QTreeWidgetItem
{
public:
    TagListViewItem(QTreeWidget *parent, TagCopy *tagCopy);
    TagListViewItem(QTreeWidgetItem *parent, TagCopy *tagCopy);
    TagListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, TagCopy *tagCopy);
    TagListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, TagCopy *tagCopy);
    TagListViewItem(QTreeWidget *parent, StateCopy *stateCopy);
    TagListViewItem(QTreeWidgetItem *parent, StateCopy *stateCopy);
    TagListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, StateCopy *stateCopy);
    TagListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, StateCopy *stateCopy);
    ~TagListViewItem() override;
    TagCopy *tagCopy()
    {
        return m_tagCopy;
    }
    StateCopy *stateCopy()
    {
        return m_stateCopy;
    }
    bool isEmblemObligatory();
    TagListViewItem *lastChild();
    TagListViewItem *prevSibling();
    TagListViewItem *nextSibling();
    TagListViewItem *parent() const; // Reimplemented to cast the return value
    int width(const QFontMetrics &fontMetrics, const QTreeWidget *listView, int column) const;
    void setup();

private:
    TagCopy *m_tagCopy;
    StateCopy *m_stateCopy;
};

Q_DECLARE_METATYPE(TagListViewItem *);

class TagListView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit TagListView(QWidget *parent = nullptr);
    ~TagListView() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    TagListViewItem *currentItem() const; // Reimplemented to cast the return value
    TagListViewItem *firstChild() const; // Reimplemented to cast the return value
    TagListViewItem *lastItem() const; // Reimplemented to cast the return value
Q_SIGNALS:
    void deletePressed();
    void doubleClickedItem();
};

/**
 * @author Sébastien Laoût
 */
class TagsEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TagsEditDialog(QWidget *parent = nullptr, State *stateToEdit = nullptr, bool addNewTag = false);
    ~TagsEditDialog() override;
    State::List deletedStates()
    {
        return m_deletedStates;
    }
    State::List addedStates()
    {
        return m_addedStates;
    }
    TagListViewItem *itemForState(State *state);

    void accept() override;
    void reject() override;

private Q_SLOTS:
    void newTag();
    void newState();
    void moveUp();
    void moveDown();
    void deleteTag();
    void renameIt();
    void capturedShortcut(const QKeySequence &shortcut);
    void removeShortcut();
    void removeEmblem();
    void modified();
    void currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *next = nullptr);
    void selectUp();
    void selectDown();
    void selectLeft();
    void selectRight();
    void resetTreeSizeHint();

private:
    void loadBlankState();
    void loadStateFrom(State *state);
    void loadTagFrom(Tag *tag);
    void saveStateTo(State *state);
    void saveTagTo(Tag *tag);
    void ensureCurrentItemVisible();
    Ui::TagsEditDialog *m_ui;
    FontSizeCombo *m_fontSize;

    TagCopy::List m_tagCopies;
    State::List m_deletedStates;
    State::List m_addedStates;

    bool m_loading;
};

#endif // TAGEDIT_H
