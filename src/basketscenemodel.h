/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "basket_export.h"
#include "noteitem.h"

#include <QAbstractItemModel>
#include <QItemSelection>


class BASKET_EXPORT BasketSceneModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum NoteRoles {
        ContentRole = Qt::UserRole + 1,
        TypeRole,
        GroupRole,
        AddressRole         // String representation of the internal NotePtr address
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

    // Implemented model operations
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count,
                  const QModelIndex &destinationParent, int destinationChild) override;

    // Specific methods to call from current code at BasketScene
    void insertNote(NotePtr note, NotePtr parentNote, int row = -1);
    void removeNote(NotePtr note);
    void moveNoteTo(NotePtr note, NotePtr parentNote, int row = -1);

    void clear();

public Q_SLOTS:
    // Interaction between the model and BasketScene, mainly for demo and debuggint purposes
    void expandItem(const QModelIndex &index);
    void collapseItem(const QModelIndex &index);
    void changeSelection(const QItemSelection &selected, const QItemSelection &deselected);

private:
    NoteItem *itemAtIndex(const QModelIndex &index) const;
    QModelIndex indexOfItem(NoteItem *item) const;
    QModelIndex findNote(NotePtr note) const;

private:
    NoteItem *m_root;
};
