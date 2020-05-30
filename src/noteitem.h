/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "note.h"
#include "notecontent.h"


Q_DECLARE_METATYPE(NoteContent *)

typedef Note * NotePtr;

/** NoteItem: Container that stores a Note object within a tree model
 * Eventually implement the managing functionallity here and use directly the `NoteContent` object
 */
class NoteItem
{
public:
    explicit NoteItem(NotePtr note);
    ~NoteItem();

    int row() const;

    NoteItem *parent() const;
    void setParent(NoteItem *parent);

    QVector<NoteItem *> children() const;
    int childrenCount() const;
    void insertAt(int row, NoteItem *item);
    void removeAt(int row);
    NoteItem *takeAt(int row);

    NotePtr note() const;
    void setNote(NotePtr note);

    QString displayText() const;
    QIcon decoration() const;

    // Find and debug Notes by its pointer address
    QString address() const;
    QString fullAddress() const;
    static QString formatAddress(void *ptr);

private:
    NoteItem *m_parent;
    QVector<NoteItem *> m_children;

    NotePtr m_note;
};

