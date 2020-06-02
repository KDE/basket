/*
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "simplecontent.h"

#include <QDomElement>

#include "common.h"


SimpleContent::SimpleContent(NoteType::Id type, const QString &basketPath)
    : m_type(type)
    , m_basketPath(basketPath)
{
}

NoteType::Id SimpleContent::type() const
{
    return m_type;
}

QVariantMap SimpleContent::attributes() const
{
    return m_attributes;
}


QString SimpleContent::toText(const QString& unused) const
{
    Q_UNUSED(unused)

    switch (m_type) {
    case NoteType::Text:
    case NoteType::Html:
        return QString::fromUtf8(m_rawData);
    default:
        return m_filePath;
    }
    return QString();
    //return m_filePath;
}

// Indicates special attributes in the XML for some types
QVariantMap SimpleContent::defaultAttributes() const
{
    switch (m_type) {
    case NoteType::Group:  return {
        { QStringLiteral("folded"), false },
    };
    case NoteType::Link: return {
        { QStringLiteral("title"),     QString() },
        { QStringLiteral("icon"),      QString() },
        { QStringLiteral("autoTitle"), true      },
        { QStringLiteral("autoIcon"),  true      }
    };
    default: return {};
    }
    return {};
}

void SimpleContent::loadFromXMLNode(QDomElement node)
{
    // Get filename
    m_filePath = node.text();

    //Load data
    m_rawData.clear();
    FileStorage::loadFromFile(m_basketPath + m_filePath, &m_rawData);

    // Load Attributes
    m_attributes = defaultAttributes();
    for (const QString &attrName : m_attributes.keys()) {
        if (node.hasAttribute(attrName)) {
            const QString attrValue = node.attribute(attrName);
            m_attributes.insert(attrName, attrValue);
        }
    }
}
