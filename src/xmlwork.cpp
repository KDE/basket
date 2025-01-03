/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "xmlwork.h"

#include <QFile>
#include <QString>
#include <QStringList>
#include <QtXml/QDomDocument>

QDomDocument *XMLWork::openFile(const QString &name, const QString &filePath)
{
    auto *doc = new QDomDocument(name);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        // QMessageBox::information(this, "Load an XML file", "Error : un-openable file");
        delete doc;
        return nullptr;
    }
    if (!doc->setContent(&file)) {
        // QMessageBox::information(this, "Load an XML file", "Error : malformed content");
        file.close();
        delete doc;
        return nullptr;
    }
    file.close();
    return doc;
}

QDomElement XMLWork::getElement(const QDomElement &startElement, const QString &elementPath)
{
    QStringList elements = elementPath.split(QLatin1Char('/'));
    QDomNode n = startElement.firstChild();
    for (int i = 0; i < elements.count(); ++i) { // For each elements
        while (!n.isNull()) { // Browse their  sub elements
            QDomElement e = n.toElement(); //  and search the good one
            if ((!e.isNull()) && e.tagName() == elements.at(i)) { // If found
                if (i + 1 == elements.count()) // And if it is the asked element
                    return e; // Return the first corresponding
                else { // Or if it is an intermediate element
                    n = e.firstChild(); // Continue with the next sub element
                    break;
                }
            }
            n = n.nextSibling();
        }
    }
    return {}; // Not found !
}

QString XMLWork::getElementText(const QDomElement &startElement, const QString &elementPath, const QString &defaultTxt)
{
    QDomElement e = getElement(startElement, elementPath);
    if (e.isNull())
        return defaultTxt;
    else
        return e.text();
}

void XMLWork::addElement(QDomDocument &document, QDomElement &parent, const QString &name, const QString &text)
{
    QDomElement tag = document.createElement(name);
    parent.appendChild(tag);
    QDomText content = document.createTextNode(text);
    tag.appendChild(content);
}

bool XMLWork::trueOrFalse(const QString &value, bool defaultValue)
{
    if (value == QStringLiteral("true") || value == QStringLiteral("1") || value == QStringLiteral("on") || value == QStringLiteral("yes"))
        return true;
    if (value == QStringLiteral("false") || value == QStringLiteral("0") || value == QStringLiteral("off") || value == QStringLiteral("no"))
        return false;
    return defaultValue;
}

QString XMLWork::trueOrFalse(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString XMLWork::innerXml(QDomElement &element)
{
    QString inner;
    for (QDomNode n = element.firstChild(); !n.isNull(); n = n.nextSibling())
        if (n.isCharacterData())
            inner += n.toCharacterData().data();
        else if (n.isElement()) {
            QDomElement e = n.toElement();
            inner += QLatin1Char('<') + e.tagName() + QLatin1Char('>') + innerXml(e) + QStringLiteral("</") + e.tagName() + QLatin1Char('>');
        }
    return inner;
}

void XMLWork::setupXmlStream(QXmlStreamWriter &stream, QString startElement)
{
    stream.setAutoFormatting(true);
    stream.setAutoFormattingIndent(1);
    stream.writeStartDocument();
    stream.writeDTD(QStringLiteral("<!DOCTYPE ") + startElement + QLatin1Char('>'));
    stream.writeStartElement(startElement);
}
