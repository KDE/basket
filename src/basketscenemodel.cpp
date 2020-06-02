/*
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketscenemodel.h"

#include <QDebug>

#include "basketscene.h"


BasketSceneModel::BasketSceneModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new NoteItem())
{
    qRegisterMetaType<SimpleContent *>();
    qRegisterMetaType<NoteItem::EditionDates>();
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
        { GeometryRole, "geometry"  },
        { EditionRole,  "edition"   }
    }.unite(QAbstractItemModel::roleNames());
    return roles;
}


QVariant BasketSceneModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const NoteItem *item = itemAtIndex(index);

    switch (role) {
        case Qt::DisplayRole:
            return item->displayText();
        case Qt::DecorationRole:
            return item->decoration();
        case Qt::ToolTipRole:       // To ease debugging
            return item->toolTipInfo();
        case BasketSceneModel::ContentRole:
            return QVariant::fromValue(item->content());
        case BasketSceneModel::TypeRole:
            return item->type();
        case BasketSceneModel::GeometryRole:
            return item->bounds();
        case BasketSceneModel::EditionRole: {
            QVariant info;
            info.setValue(item->editionDates());
            return info;
        }
    }

    return QVariant();
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


bool BasketSceneModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if(!checkIndex(parent)) {
        return false;
    }

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; i++) {
        itemAtIndex(parent)->insertAt(row + i, new NoteItem());
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


void BasketSceneModel::clear()
{
    beginResetModel();
    qDeleteAll(m_root->children());
    endResetModel();
}

void BasketSceneModel::loadFromXML(const QDomElement &node)
{
    beginResetModel();
    qDeleteAll(m_root->children());
    NoteItem::setBasketPath(qobject_cast<BasketScene *>(qobject_cast<QObject *>(this)->parent())->fullPath());
    m_root->loadFromXMLNode(node);
    endResetModel();
}
