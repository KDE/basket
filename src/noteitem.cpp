/*
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "noteitem.h"

#include <QDomElement>

#include "basketscene.h"     // TEMP: to get basket->fullPath()
#include "xmlwork.h"


namespace {

QString iconNameForType(NoteType::Id type)
{
    switch (type) {
        case NoteType::Group:  return "package";
        case NoteType::Text: return "text-x-plain";
        case NoteType::Html: return "text-html";
        case NoteType::Image: return "folder-pictures-symbolic";
        case NoteType::Animation: return "folder-video-symbolic";
        case NoteType::Sound: return "folder-music-symbolic";
        case NoteType::File: return "application-x-zerosize";
        case NoteType::Link: return "edit-link";
        case NoteType::CrossReference: return "basket";
        case NoteType::Launcher: return "run-build";
        case NoteType::Color: return "color-profile";
        case NoteType::Unknown: return "application-octet-stream";
    }
    return QString();
}

}

BasketScene *NoteItem::s_basket;

NoteItem::NoteItem()
    : m_parent(nullptr)
    , m_content(nullptr)
{
}

NoteItem::~NoteItem()
{
    delete m_content;
    qDeleteAll(m_children);
}


int NoteItem::row() const
{
    if (!m_parent) {
        return 0;
    }
    return m_parent->m_children.indexOf(const_cast<NoteItem *>(this));
}

NoteItem *NoteItem::parent() const
{
    return m_parent;
}

void NoteItem::setParent(NoteItem *parent)
{
    m_parent = parent;
}

QVector<NoteItem *> NoteItem::children() const
{
    return m_children;
}

int NoteItem::childrenCount() const
{
    return m_children.count();
}

void NoteItem::insertAt(int row, NoteItem *item)
{
    if (!item) return;

    item->setParent(this);
    if (row >= 0 && row < m_children.count() ) {
        m_children.insert(row, item);
    } else {
        m_children.append(item);
    }
}

void NoteItem::removeAt(int row)
{
    if (row < 0 || row >= m_children.count()) {
        return;
    }
    delete m_children[row];
    m_children.remove(row);
}

NoteItem *NoteItem::takeAt(int row)
{
    if (row < 0 || row >= m_children.count()) {
        return nullptr;
    }
    return m_children.takeAt(row);
}


void NoteItem::setBasketParent(BasketScene* basket)
{
    s_basket = basket;
}


QString NoteItem::displayText() const
{
    if (type() == NoteType::Group) { return QStringLiteral("GROUP"); }
    if (!content()) { return QStringLiteral("<no content>"); }
    return content()->toText(QString());
}

QIcon NoteItem::decoration() const
{
    return QIcon::fromTheme(iconNameForType(type()));
}

SimpleContent *NoteItem::content() const
{
    return m_content;
}

NoteType::Id NoteItem::type() const
{
    return content()->type();
}

QRect NoteItem::bounds() const
{
    return m_bounds;
}

NoteItem::EditionDates NoteItem::editionDates() const
{
    return m_editInfo;
}

QString NoteItem::address() const
{
    if (!m_parent) {
        return QStringLiteral("root");
    }
    return QString::number(reinterpret_cast<long>(this), 16);
}

QString NoteItem::toolTipInfo() const
{
    QStringList toolTip;

    // fullAddress and position within parent (debug)
    toolTip << QStringLiteral("<%1> @<%2>[%3]")
                                .arg(address())
                                .arg(m_parent->address())
                                .arg(row());

    // type
    toolTip << QStringLiteral("Type: %1").arg(NoteType::typeToName(type()));

    // geometry
    const QRect geometry = bounds();
    toolTip << QStringLiteral("x:%1 y:%2 w:%3 h:%4")
                                .arg(geometry.x()).arg(geometry.y())
                                .arg(geometry.width()).arg(geometry.height());

    // edition information
    const EditionDates info = editionDates();
    toolTip << QStringLiteral("created: %1").arg(info.created.toString())
            << QStringLiteral("modified: %1").arg(info.modified.toString());

    // tags
    toolTip << QStringLiteral("Tags: %1").arg(m_tagIds.join(QStringLiteral(", ")));

    //attributes
    const auto attributes = m_content->attributes();
    for (const QString &attrName : attributes.keys()) {
        toolTip << QStringLiteral("%1: %2").arg(attrName)
                                           .arg(attributes.value(attrName).toString());
    }

    return toolTip.join(QStringLiteral("\n"));
}


void NoteItem::loadFromXMLNode(const QDomElement& node)
{
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();
        if (e.isNull()) {
            continue;
        }

        NoteItem *child = new NoteItem();
        this->insertAt(-1, child);

        child->loadPropertiesFromXMLNode(e);
        if (child->type() == NoteType::Group) {
            child->loadFromXMLNode(e);
        }
    }
}

void NoteItem::loadPropertiesFromXMLNode(const QDomElement& node)
{
    if (node.tagName() == "group") {
        m_content = new SimpleContent(NoteType::Group);
        m_content->loadFromXMLNode(node);
    }
    if (node.tagName() == "note" || node.tagName() == "item") {      // "item" is to keep compatible with 0.6.0 Alpha 1 (required?)
        const QDomElement content = XMLWork::getElement(node, "content");
        NoteType::Id type = NoteType::typeFromLowerName(node.attribute("type"));
        m_content = new SimpleContent(type, s_basket->fullPath());
        m_content->loadFromXMLNode(content);
    }

    // Load dates
    if (node.hasAttribute("added")) {
        m_editInfo.created = QDateTime::fromString(node.attribute("added"), Qt::ISODate);
    }
    if (node.hasAttribute("lastModification")) {
        m_editInfo.modified = QDateTime::fromString(node.attribute("lastModification"), Qt::ISODate);
    }

    m_bounds = QRect(node.attribute("x", "0").toInt(),
                     node.attribute("y", "0").toInt(),
                     node.attribute("width", "200").toInt(),
                     node.attribute("height", "200").toInt());

    // Tags:
    const QString tagsString = XMLWork::getElementText(node, QStringLiteral("tags"), QString());
    m_tagIds = tagsString.split(';');
}
