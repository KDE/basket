/**
 * SPDX-FileCopyrightText: (C) 2009 Matt Rogers <mattr@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>

#include <basketview.h>
#include <note.h>

class NoteTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCreation();
};

QTEST_MAIN(NoteTest)

void NoteTest::testCreation()
{
    Note *n = new Note(nullptr);
    QVERIFY(n->basket() == nullptr);
    QVERIFY(n->next() == nullptr);
    QVERIFY(n->prev() == nullptr);
    QVERIFY(n->content() == nullptr);
    QCOMPARE(n->x(), 0.0);
    QCOMPARE(n->y(), 0.0);
    QCOMPARE(n->width(), Note::GROUP_WIDTH);
    QCOMPARE(n->height(), Note::MIN_HEIGHT);
    delete n;
}

#include "notetest.moc"
/* vim: set et sts=4 sw=4 ts=8 tw=0 : */
