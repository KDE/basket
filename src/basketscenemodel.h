/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "basket_export.h"
#include "noteitem.h"

#include <QAbstractItemModel>
#include <QItemSelection>


class BasketScene;


class BASKET_EXPORT BasketSceneModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum AdditionalRoles {
        ContentRole = Qt::UserRole + 1, // NoteContent * : the internal NoteContent
        TypeRole,           // NoteContent::Type: the type of the NoteContent
        GeometryRole,       // QRect: the bounds of the Note
        EditionRole,        // NoteItem::EditInfo: the edition information about the Note
    };

public:
    explicit BasketSceneModel(QObject *parent = nullptr);
    ~BasketSceneModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
//    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;

    void clear();

public Q_SLOTS:
    void loadFromXML(const QDomElement &node);

private:
    NoteItem *itemAtIndex(const QModelIndex &index) const;
    QModelIndex indexOfItem(NoteItem *item) const;

private:
    NoteItem *m_root;
};

Q_DECLARE_METATYPE(BasketSceneModel *)
