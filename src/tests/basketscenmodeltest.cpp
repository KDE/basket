/**
 * SPDX-FileCopyrightText: (C) 2020 Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>
#include <QAbstractItemModelTester>

#include <basketmodel.h>

class BasketModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBasketModel();
};

QTEST_MAIN(BasketModelTest)

void BasketModelTest::testBasketModel()
{
    BasketModel *model = new BasketModel(nullptr);
    auto tester = new QAbstractItemModelTester(model);
}

#include "basketscenmodeltest.moc"
