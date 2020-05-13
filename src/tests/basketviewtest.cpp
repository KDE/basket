/**
 * SPDX-FileCopyrightText: (C) 2009 Matt Rogers <mattr@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>

#include <basketview.h>
#include <note.h>

class BasketViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCreation();
};

QTEST_MAIN(BasketViewTest)

void BasketViewTest::testCreation()
{
}
#include "basketviewtest.moc"
/* vim: set et sts=4 sw=4 ts=8 tw=0 : */
