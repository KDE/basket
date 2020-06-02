/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QMetaType>

#include "simplecontent.h"


class QDomElement;

/** NoteItem: Container that stores a Note object within a tree model
 * Eventually implement the managing functionallity here and use directly the `NoteContent` object
 */
class NoteItem
{
public:
    struct EditionDates
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

    static void setBasketPath(QString path);

    // NoteItem property getters
    QString displayText() const;
    QIcon decoration() const;
    NoteType::Id type() const;
    SimpleContent *content() const;
    QRect bounds() const;
    EditionDates editionDates() const;

    // Loader from XML node (.basket xml format)
    void loadFromXMLNode(const QDomElement &node);
    void loadPropertiesFromXMLNode(const QDomElement &node);

    // Useful methods to debug
    QString address() const;
    QString toolTipInfo() const;


private:
    NoteItem *m_parent;
    QVector<NoteItem *> m_children;

    static QString s_basketPath;

    SimpleContent *m_content;
    QRect m_bounds;
    EditionDates m_editInfo;
    QStringList m_tagIds;
};

Q_DECLARE_METATYPE(SimpleContent *)
Q_DECLARE_METATYPE(NoteItem::EditionDates)
