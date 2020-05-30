/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketscenemodel.h"

#include <QDebug>


BasketSceneModel::BasketSceneModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new NoteItem(nullptr))
{
}

BasketSceneModel::~BasketSceneModel()
{
    delete m_root;
}

int BasketSceneModel::rowCount(const QModelIndex &parent) const
{
    return itemAtIndex(parent)->childrenCount();
}

int BasketSceneModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex BasketSceneModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    NoteItem *item = itemAtIndex(parent)->children().at(row);
    if (item) {
        return createIndex(row, column, item);
    }
    return QModelIndex();
}

QModelIndex BasketSceneModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    NoteItem *parentItem = itemAtIndex(index)->parent();
    return indexOfItem(parentItem);
}

QVariant BasketSceneModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return QStringLiteral("BasketSceneModel");

    return QVariant();
}

QHash<int, QByteArray> BasketSceneModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { ContentRole,  "content"   },
        { TypeRole,     "type"      },
        { GroupRole,    "isGroup"   },
        { AddressRole,  "address"   }
    }.unite(QAbstractItemModel::roleNames());
    return roles;
}


QVariant BasketSceneModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const NoteItem *item = itemAtIndex(index);
    const NotePtr note = item->note();

    switch (role) {
        case Qt::DisplayRole:
            return item->displayText();
        case Qt::DecorationRole:
            return item->decoration();
        case Qt::ToolTipRole:
            return item->fullAddress();  // To ease debugging
        case BasketSceneModel::ContentRole:
            return QVariant::fromValue(note->content());
        case BasketSceneModel::TypeRole:
            return note->content()->type();
        case BasketSceneModel::GroupRole:
            return note->isGroup();
        case BasketSceneModel::AddressRole:
            return item->address();
    }

    return QVariant();
}


void BasketSceneModel::clear()
{
    qDeleteAll(m_root->children());
}

NoteItem *BasketSceneModel::itemAtIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return m_root;
    }
    return static_cast<NoteItem*>(index.internalPointer());
}

QModelIndex BasketSceneModel::indexOfItem(NoteItem *item) const
{
    if (!item || item == m_root) {
        return QModelIndex();
    }
    return createIndex(item->row(), 0, item);
}

QModelIndex BasketSceneModel::findNote(NotePtr note) const
{
    if (!note || m_root->childrenCount() == 0) {
        return QModelIndex();
    }
    const QModelIndexList matchResult = match(indexOfItem(m_root->children().at(0)),
                                              BasketSceneModel::AddressRole,
                                              NoteItem::formatAddress(note), 1,
                                              Qt::MatchExactly | Qt::MatchWrap | Qt::MatchRecursive);
    if (matchResult.isEmpty()) {
        return QModelIndex();
    }
    return matchResult[0];
}


bool BasketSceneModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if(!checkIndex(parent)) {
        return false;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++) {
        itemAtIndex(parent)->insertAt(row + i, new NoteItem(nullptr));
    }
    endInsertRows();
    return true;
}

bool BasketSceneModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if(!checkIndex(parent)) {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++) {
        itemAtIndex(parent)->removeAt(row);
    }
    endRemoveRows();
    return true;
}

bool BasketSceneModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                                const QModelIndex& destinationParent, int destinationChild)
{
    const bool isMoveDown = (destinationParent == sourceParent  && destinationChild > sourceRow);

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count -1,
                       destinationParent, (isMoveDown) ? destinationChild + 1 : destinationChild)) {
        return false;
    }

    for (int i = 0; i < count; i++) {
        const int oldRow = (isMoveDown) ? sourceRow : sourceRow + i;
        itemAtIndex(destinationParent)->insertAt(destinationChild + i,
                                                 itemAtIndex(sourceParent)->takeAt(oldRow));
    }
    endMoveRows();
    return true;
}


void BasketSceneModel::insertNote(NotePtr note, NotePtr parentNote, int row)
{
    if (!note) {
        return;
    }

    const QModelIndex parentIndex = findNote(parentNote);
    if (!checkIndex(parentIndex)) {
        return;
    }

    if (row < 0 || row >= rowCount(parentIndex)) {  // row == -1 : insert at the end
        row = rowCount(parentIndex);
    }

    qDebug() << "BasketSceneModel::insertNote " << NoteItem::formatAddress(note)
             << " at " << itemAtIndex(parentIndex)->address()
             << "[" << row << "]";

    // Protection against adding the same Note * twice (same pointer)
    // This should not be necessary once the old code in BasketScene has been cleaned-up
    if (findNote(note).isValid()) {
        QModelIndex prevIndex = findNote(note);
        qDebug() << " · Note " << itemAtIndex(prevIndex)->address()
                 << " was alreadyPresent at " << itemAtIndex(parentIndex)->address()
                 << "[" << prevIndex.row() << "]"
                 << ". Moving it instead to new location";
        moveNoteTo(note, parentNote, row);
        return;
    }

    if (insertRow(row, parentIndex)) {
        itemAtIndex(parentIndex)->children().at(row)->setNote(note);
        connect(note, &QObject::destroyed, this, [=](QObject *note) {
            BasketSceneModel::removeNote(static_cast<Note *>(note));
        });
    }
}

void BasketSceneModel::removeNote(NotePtr note)
{
    if (!note) {
        return;
    }
    const QModelIndex index = findNote(note);
    const QModelIndex parentIndex = index.parent();

    qDebug() << "BasketSceneModel::removeNote " << itemAtIndex(index)->address();

    removeRow(index.row(), parentIndex);
}

void BasketSceneModel::moveNoteTo(NotePtr note, NotePtr parentNote, int row)
{
    if (!note) {
        return;
    }
    const QModelIndex currentIndex = findNote(note);
    const QModelIndex srcParentIndex = currentIndex.parent();
    const QModelIndex destParentIndex = findNote(parentNote);

    if (!checkIndex(currentIndex, CheckIndexOption::IndexIsValid)) {
        return;
    }

    if (row < 0 || row >= rowCount(destParentIndex)) {
        row = rowCount(destParentIndex);    // row == -1 : insert at the end
    }

    qDebug() << "BasketSceneModel::moveNote " << itemAtIndex(currentIndex)->address()
             << "\n  from: " << itemAtIndex(srcParentIndex)->address() << "[" << currentIndex.row() << "]"
             << "\n    to: " << itemAtIndex(destParentIndex)->address() << "[" << row << "]";

    moveRow(srcParentIndex, currentIndex.row(), destParentIndex, row);
}


void BasketSceneModel::collapseItem(const QModelIndex &index)
{
    if (hasChildren(index)) {
        itemAtIndex(index)->children().at(0)->note()->tryFoldParent();
    }
}

void BasketSceneModel::expandItem(const QModelIndex &index)
{
    if (hasChildren(index)) {
        itemAtIndex(index)->children().at(0)->note()->tryExpandParent();
    }
}

void BasketSceneModel::changeSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
    for (const auto index : deselected.indexes()) {
        itemAtIndex(index)->note()->setSelected(false);
    }
    for (const auto index : selected.indexes()) {
        itemAtIndex(index)->note()->setSelected(true);
    }
}
