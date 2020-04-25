/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NOTESELECTION_H
#define NOTESELECTION_H

#include <QtCore/QList>
#include <QtCore/QString>

class Note;

/** This represent a hierarchy of the selected classes.
 * If this is null, then there is no selected note.
 */
class NoteSelection
{
public:
    NoteSelection()
        : note(0)
        , parent(0)
        , firstChild(0)
        , next(0)
        , fullPath()
    {
    }
    NoteSelection(Note *n)
        : note(n)
        , parent(0)
        , firstChild(0)
        , next(0)
        , fullPath()
    {
    }

    Note *note;
    NoteSelection *parent;
    NoteSelection *firstChild;
    NoteSelection *next;
    QString fullPath; // Needeed for 'Cut' code to store temporary path of the cut note.

    NoteSelection *firstStacked();
    NoteSelection *nextStacked();
    void append(NoteSelection *node);
    int count();

    QList<Note *> parentGroups();
};

#endif // NOTESELECTION_H
