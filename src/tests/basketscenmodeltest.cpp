/**
 * SPDX-FileCopyrightText: (C) 2020 Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>
#include <QAbstractItemModelTester>

#include <basketscenemodel.h>

class BasketSceneModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBasketSceneModel();
};

QTEST_MAIN(BasketSceneModelTest)

void BasketSceneModelTest::testBasketSceneModel()
{
    BasketSceneModel *model = new BasketSceneModel(nullptr);
    auto tester = new QAbstractItemModelTester(model);
}

#include "basketscenmodeltest.moc"
