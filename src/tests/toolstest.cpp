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
    void testHtmlToText_data();
    void testHtmlToText();

private:
    bool readAll(QString fileName, QString &text);
};

QTEST_MAIN(ToolsTest)

void ToolsTest::testHtmlToText_data()
{
    QTest::addColumn<QString>("filename");

    const QDir datadir(QFINDTESTDATA("htmltotext/"));
    for (const QString &entry : datadir.entryList({QStringLiteral("*.html")}, QDir::Files, QDir::Name)) {
        QTest::newRow(qPrintable(entry)) << entry;
    }
}

void ToolsTest::testHtmlToText()
{
    // Test the function on files from htmltotext/

    QFETCH(QString, filename);

    const QString basename = QFINDTESTDATA("htmltotext/");
    QString html, text;
    QVERIFY(readAll(basename + filename, html));
    QVERIFY(readAll(basename + filename.chopped(4) + QStringLiteral("txt"), text));
    QCOMPARE(Tools::htmlToText(html), text);
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
