/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "note.h"
#include "notecontent.h"

#include <QMetaType>


class QDomElement;

typedef Note * NotePtr;

/** NoteItem: Container that stores a Note object within a tree model
 * Eventually implement the managing functionallity here and use directly the `NoteContent` object
 */
class NoteItem
{
public:
    struct EditInfo
    {
        QDateTime created;
        QDateTime modified;
    };

public:
    explicit NoteItem();
    ~NoteItem();

    // Tree structure
    int row() const;

    NoteItem *parent() const;
    void setParent(NoteItem *parent);

    QVector<NoteItem *> children() const;
    int childrenCount() const;
    void insertAt(int row, NoteItem *item);
    void removeAt(int row);
    NoteItem *takeAt(int row);

    // Accesors for compatibility with current code
    NotePtr note() const;
    static void setBasketParent(BasketScene *basket);

    // NoteItem property getters
    QString displayText() const;
    QIcon decoration() const;
    NoteContent *content() const;
    NoteType::Id type() const;
    QRect bounds() const;
    EditInfo editInformation() const;

    // Recursive loader from an XML node
    void loadFromXMLNode(const QDomElement &node);

    // Useful methods to debug
    QString address() const;
    QString toolTipInfo() const;


private:
    NoteItem *m_parent;
    QVector<NoteItem *> m_children;

    NotePtr m_helperNote;         // Dummy note to help with the code transition.
    static BasketScene *s_basket; // Stored to set a parent to the notes (and avoid crashes)

//     NoteContent *m_content;
//     QRect m_bounds;
//     EditInfo m_editInfo;
//     QVector<State> m_tags;
};

Q_DECLARE_METATYPE(NoteContent *)
Q_DECLARE_METATYPE(NoteItem::EditInfo)
