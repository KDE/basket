/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "noteselection.h"

#include <QDebug>

#include "note.h"
#include "notecontent.h"

/** Class NoteSelection: */

NoteSelection *NoteSelection::nextStacked()
{
    // First, search in the children:
    if (firstChild) {
        if (firstChild->note && firstChild->note->content())
            return firstChild;
        else
            return firstChild->nextStacked();
    }
    // Then, in the next:
    if (next) {
        if (next->note && next->note->content())
            return next;
        else
            return next->nextStacked();
    }
    // And finally, in the parent:
    NoteSelection *node = parent;
    while (node) {
        if (node->next) {
            if (node->next->note && node->next->note->content())
                return node->next;
            else
                return node->next->nextStacked();
        } else
            node = node->parent;
    }
    // Not found:
    return 0;
}

NoteSelection *NoteSelection::firstStacked()
{
    if (!this)
        return 0;

    if (note && note->content())
        return this;
    else
        return nextStacked();
}

void NoteSelection::append(NoteSelection *node)
{
    if (!this || !node)
        return;

    if (firstChild) {
        NoteSelection *last = firstChild;
        while (last->next)
            last = last->next;
        last->next = node;
    } else
        firstChild = node;

    while (node) {
        node->parent = this;
        node = node->next;
    }
}

int NoteSelection::count()
{
    if (!this)
        return 0;

    int count = 0;

    for (NoteSelection *node = this; node; node = node->next)
        if (node->note && node->note->content())
            ++count;
        else
            count += node->firstChild->count();

    return count;
}

QList<Note *> NoteSelection::parentGroups()
{
    QList<Note *> groups;

    // For each note:
    for (NoteSelection *node = firstStacked(); node; node = node->nextStacked())
        // For each parent groups of the note:
        for (Note *note = node->note->parentNote(); note; note = note->parentNote())
            // Add it (if it was not already in the list):
            if (!note->isColumn() && !groups.contains(note))
                groups.append(note);

    return groups;
}

void debugSel(NoteSelection *sel, int n = 0)
{
    for (NoteSelection *node = sel; node; node = node->next) {
        for (int i = 0; i < n; i++)
            qDebug() << "-";
        qDebug() << (node->firstChild ? "Group" : node->note->content()->toText(QString()));
        if (node->firstChild)
            debugSel(node->firstChild, n + 1);
    }
}
