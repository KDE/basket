/*
 *   Copyright (C) 2014 by Gleb Baryshev <gleb.baryshev@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <QObject>
#include <QtTest/QtTest>

#include "tools.h"

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

        if (readAll(basename + ".html", html) && readAll(basename + ".txt", text))
            QCOMPARE(Tools::htmlToText(html), text);
    }
}

bool ToolsTest::readAll(QString fileName, QString &text)
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        QWARN(QString("Failed to open data file %1 - skipping").arg(fileName).toUtf8());
        return false;
    }
    QTextStream filestream(&f);
    text = filestream.readAll();
    return true;
}

#include "toolstest.moc"
