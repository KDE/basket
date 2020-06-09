/*
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketmodel.h"

#include <QDebug>
#include <QDomElement>

#include "basketscene.h"
#include "xmlwork.h"


BasketModel::BasketModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(new NoteItem())
{
    qRegisterMetaType<SimpleContent *>();
    qRegisterMetaType<NoteItem::EditionDates>();
}

BasketModel::~BasketModel()
{
    delete m_root;
}

int BasketModel::rowCount(const QModelIndex &parent) const
{
    return itemAtIndex(parent)->childrenCount();
}

int BasketModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex BasketModel::index(int row, int column, const QModelIndex &parent) const
{
    NoteItem *parentItem = itemAtIndex(parent);
    if (!parentItem || row < 0 || row >= parentItem->childrenCount()) {
        return QModelIndex();
    }
    return createIndex(row, column, parentItem->children().at(row));
}

QModelIndex BasketModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    NoteItem *parentItem = itemAtIndex(index)->parent();
    return indexOfItem(parentItem);
}

QVariant BasketModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return QStringLiteral("BasketSceneModel");

    return QVariant();
}

QHash<int, QByteArray> BasketModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { ContentRole,  "content"   },
        { TypeRole,     "type"      },
        { GeometryRole, "geometry"  },
        { EditionRole,  "edition"   },
    }.unite(QAbstractItemModel::roleNames());
    return roles;
}


QVariant BasketModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const NoteItem *item = itemAtIndex(index);

    switch (role) {
        case Qt::DisplayRole:
            return item->displayText();
        case Qt::DecorationRole:
            return item->decoration().name();
        case Qt::ToolTipRole:       // To ease debugging
            return item->toolTipInfo();
        case BasketModel::ContentRole:
            return QVariant::fromValue(item->content());
        case BasketModel::TypeRole:
            return item->type();
        case BasketModel::GeometryRole:
            return item->bounds();
        case BasketModel::EditionRole: {
            QVariant info;
            info.setValue(item->editionDates());
            return info;
        }
    }

    return QVariant();
}


NoteItem *BasketModel::itemAtIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return m_root;
    }
    return static_cast<NoteItem*>(index.internalPointer());
}

QModelIndex BasketModel::indexOfItem(NoteItem *item) const
{
    if (!item || item == m_root) {
        return QModelIndex();
    }
    return createIndex(item->row(), 0, item);
}


bool BasketModel::insertRows(int row, int count, const QModelIndex& parent)
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

bool BasketModel::removeRows(int row, int count, const QModelIndex& parent)
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

bool BasketModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
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


void BasketModel::clear()
{
    beginResetModel();
    qDeleteAll(m_root->children());
    endResetModel();
}

void BasketModel::loadFromXML(const QDomElement &doc)
{
    QDomElement properties = XMLWork::getElement(doc, "properties");
    QDomElement notes = XMLWork::getElement(doc, "notes");

    // Load basket properties
    QDomElement disposition = XMLWork::getElement(properties, "disposition");
    bool isFree = XMLWork::trueOrFalse(disposition.attribute("free", "true"));
    m_layout = (isFree) ? Layout::FreeLayout : Layout::ColumnLayout;
    Q_EMIT layoutChanged();

    // Load notes
    beginResetModel();
    qDeleteAll(m_root->children());
    NoteItem::setBasketPath(qobject_cast<BasketScene *>(qobject_cast<QObject *>(this)->parent())->fullPath());
    m_root->loadFromXMLNode(notes);
    endResetModel();
}


BasketModel::Layout BasketModel::layout() const
{
    return m_layout;
}
