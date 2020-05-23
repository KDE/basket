/*
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "basketscenemodel.h"

#include "notefactory.h"
#include "xmlwork.h"

#include <QDomElement>


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
    , m_helperNote(new Note(s_basket))
{
}

NoteItem::~NoteItem()
{
    delete m_helperNote;
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


NotePtr NoteItem::note() const
{
    return m_helperNote;
}

void NoteItem::setBasketParent(BasketScene* basket)
{
    s_basket = basket;
}


QString NoteItem::displayText() const
{
    if (!note()) { return QStringLiteral("NULL NOTE"); }
    if (type() == NoteType::Group) { return QStringLiteral("GROUP"); }
    return content()->toText(QString());
}

QIcon NoteItem::decoration() const
{
    if (!note()) { return QIcon::fromTheme("data-error"); }
    if (type() == NoteType::Group) { return QIcon::fromTheme("package"); }
    if (!content()) { return QIcon::fromTheme("empty"); }

    return QIcon::fromTheme(iconNameForType(content()->type()));
}

NoteContent *NoteItem::content() const
{
    if (!note()) {
        return nullptr;
    }
    return note()->content();
}

NoteType::Id NoteItem::type() const
{
    if (!note()) {
        return NoteType::Unknown;
    }
    if (!note()->content()) {
        return NoteType::Group;
    }
    return note()->content()->type();
}

QRect NoteItem::bounds() const
{
    return QRect(
        note()->x(),
        note()->y(),
        note()->width(),
        note()->height()
    );
}

NoteItem::EditInfo NoteItem::editInformation() const
{
    return EditInfo {
        note()->addedDate(),
        note()->lastModificationDate()
    };
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
    toolTip << QStringLiteral("Type: %1").arg(content() ? content()->typeName() : "Group");

    // geometry
    const QRect geometry = bounds();
    toolTip << QStringLiteral("x:%1 y:%2 w:%3 h:%4")
                                .arg(geometry.x()).arg(geometry.y())
                                .arg(geometry.width()).arg(geometry.height());

    // edition information
    const EditInfo info = editInformation();
    toolTip << QStringLiteral("created: %1\nmodified: %2")
                                .arg(info.created.toString())
                                .arg(info.modified.toString());

    return toolTip.join(QStringLiteral("\n"));
}


void NoteItem::loadFromXMLNode(const QDomElement& node)
{
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();
        if (e.isNull()) {
            continue;
        }

        NoteItem *noteItem = new NoteItem();
        noteItem->setParent(this);
        m_children.append(noteItem);

        NotePtr note = noteItem->note();    // Helper Note object

        if (e.tagName() == "group") {
            // Node is a group. Recursively load from this element
            noteItem->loadFromXMLNode(e);
        } else if (e.tagName() == "note" || e.tagName() == "item") {      // "item" is to keep compatible with 0.6.0 Alpha 1 (required?)
            // Load note content
            NoteFactory::loadNode(XMLWork::getElement(e, "content"),
                                  e.attribute("type"),
                                  note,
                                  true);  //lazyload
        }

        // Load dates
        if (e.hasAttribute("added")) {
            note->setAddedDate(QDateTime::fromString(e.attribute("added"), Qt::ISODate));
        }
        if (e.hasAttribute("lastModification")) {
            note->setLastModificationDate(QDateTime::fromString(e.attribute("lastModification"), Qt::ISODate));
        }
        // Free Note Properties:
        if (note->isFree()) {
            int x = e.attribute("x").toInt();
            int y = e.attribute("y").toInt();
            note->setX(x < 0 ? 0 : x);
            note->setY(y < 0 ? 0 : y);
        }
        // Resizeable Note Properties:
        if (note->hasResizer() || note->isColumn()) {
            note->setGroupWidth(e.attribute("width", "200").toInt());
        }
        // Group Properties:
        if (note->isGroup() && !note->isColumn() && XMLWork::trueOrFalse(e.attribute("folded", "false"))) {
            note->toggleFolded();
        }
        // Tags:
        if (note->content()) {
            const QString tagsString = XMLWork::getElementText(e, QStringLiteral("tags"), QString());
            const QStringList tagsId = tagsString.split(';');
            for (const QString &tagId : tagsId) {
                State *state = Tag::stateForId(tagId);
                if (state) {
                    note->addState(state, /*orReplace=*/true);
                }
            }
        }
    }
}
