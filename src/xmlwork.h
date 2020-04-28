/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef XMLWORKXMLWORK_H
#define XMLWORKXMLWORK_H

#include <QXmlStreamWriter>
#include <QtCore/QString>

class QDomDocument;
class QDomElement;

/** All related functions to manage XML files and trees
 * @author Sébastien Laoût
 */
namespace XMLWork
{
// Manage XML files :
QDomDocument *openFile(const QString &name, const QString &filePath);
// Manage XML trees :
QDomElement getElement(const QDomElement &startElement, const QString &elementPath);
QString getElementText(const QDomElement &startElement, const QString &elementPath, const QString &defaultTxt = QString());
void addElement(QDomDocument &document, QDomElement &parent, const QString &name, const QString &text);
QString innerXml(QDomElement &element);
void setupXmlStream(QXmlStreamWriter &stream, QString startElement); ///< Set XML options and write document start
// Not directly related to XML :
bool trueOrFalse(const QString &value, bool defaultValue = true);
QString trueOrFalse(bool value);
}

#endif // XMLWORKXMLWORK_H
