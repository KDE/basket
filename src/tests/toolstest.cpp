/**
 * SPDX-FileCopyrightText: (C) 2014 Gleb Baryshev <gleb.baryshev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>
#include <QtTest/QtTest>

#include <tools.h>

class ToolsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testHtmlToText();

private:
    bool readAll(QString fileName, QString &text);
};

QTEST_MAIN(ToolsTest)

void ToolsTest::testHtmlToText()
{
    // Test the function on files from htmltotext/

    for (int i = 1; i <= 5; i++) {
        QString html, text;
        QString basename = QFINDTESTDATA("htmltotext/");
        QVERIFY2(QDir(basename).exists(), "Test data file not found");
        basename += QString::number(i);

        if (readAll(basename + QStringLiteral(".html"), html) && readAll(basename + QStringLiteral(".txt"), text))
            QCOMPARE(Tools::htmlToText(html), text);
    }
}

bool ToolsTest::readAll(QString fileName, QString &text)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << QStringLiteral("Failed to open data file %1 - skipping").arg(fileName).toUtf8();
        return false;
    }
    QTextStream filestream(&f);
    text = filestream.readAll();
    return true;
}

#include "toolstest.moc"
