/**
 * SPDX-FileCopyrightText: (C) 2026 Pino Toscano <pino@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>

#include <notefactory.h>

class NoteFactoryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testTextToURLList_data();
    void testTextToURLList();
};

QTEST_GUILESS_MAIN(NoteFactoryTest)

void NoteFactoryTest::testTextToURLList_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QStringList>("result");

    // text only or invalid email/URLs -> nothing
    QTest::newRow("empty") << QString() << QStringList();
    QTest::newRow("text") << QStringLiteral("some text") << QStringList();
    QTest::newRow("text-multiline") << QStringLiteral("some text\nnew line") << QStringList();
    QTest::newRow("text-multiline-emptylines") << QStringLiteral("some text\n\nnew line") << QStringList();
    QTest::newRow("invalid-email") << QStringLiteral("email@") << QStringList();
    QTest::newRow("invalid-url") << QStringLiteral("protocol://host/path") << QStringList();

    // valid emails or URLs only
    QTest::newRow("valid-email") << QStringLiteral("email@example.org") << QStringList{QStringLiteral("mailto:email@example.org"), QString()};
    QTest::newRow("valid-email+name") << QStringLiteral("Some nice Example <email@example.org>")
                                      << QStringList{QStringLiteral("mailto:email@example.org"), QStringLiteral("Some nice Example")};
    QTest::newRow("valid-https") << QStringLiteral("https://kde.org") << QStringList{QStringLiteral("https://kde.org"), QString()};
    QTest::newRow("mixed-email-https") << QStringLiteral("email@example.org\nhttps://kde.org")
                                       << QStringList{QStringLiteral("mailto:email@example.org"), QString(), QStringLiteral("https://kde.org"), QString()};
    QTest::newRow("mixed-email-https-newline-1") << QStringLiteral("email@example.org\n\nhttps://kde.org")
                                                 << QStringList{QStringLiteral("mailto:email@example.org"),
                                                                QString(),
                                                                QStringLiteral("https://kde.org"),
                                                                QString()};
    QTest::newRow("mixed-email-https-newline-2") << QStringLiteral("email@example.org\nhttps://kde.org\n")
                                                 << QStringList{QStringLiteral("mailto:email@example.org"),
                                                                QString(),
                                                                QStringLiteral("https://kde.org"),
                                                                QString()};

    // emails and/or URLs with text -> nothing
    QTest::newRow("mixed-email-text") << QStringLiteral("email@example.org\nsome text") << QStringList();
    QTest::newRow("mixed-https-text") << QStringLiteral("https://kde.org\nsome text") << QStringList();
}

void NoteFactoryTest::testTextToURLList()
{
    QFETCH(QString, input);
    QFETCH(QStringList, result);
    QCOMPARE(NoteFactory::textToURLList(input), result);
}

#include "notefactorytest.moc"
