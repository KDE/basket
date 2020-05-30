/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "basketscenemodel.h"


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


NoteItem::NoteItem(Note* note)
    : m_parent(nullptr)
    , m_note(note)
{
}

NoteItem::~NoteItem()
{
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
    return m_note;
}

void NoteItem::setNote(NotePtr note)
{
    m_note = note;
}

QString NoteItem::displayText() const
{
    if (!m_note) { return QStringLiteral("NULL NOTE"); }
    if (m_note->isGroup()) { return QStringLiteral("GROUP"); }
    return m_note->toText(QString());
}

QIcon NoteItem::decoration() const
{
    if (!m_note) { return QIcon::fromTheme("data-error"); }
    if (m_note->isGroup()) { return QIcon::fromTheme("package"); }
    if (!m_note->content()) { return QIcon::fromTheme("empty"); }

    return QIcon::fromTheme(iconNameForType(m_note->content()->type()));
}


QString NoteItem::formatAddress(void *ptr) {
    return QString::number(reinterpret_cast<long>(ptr), 16);
}

QString NoteItem::address() const
{
    return formatAddress(m_note);
}

QString NoteItem::fullAddress() const
{
    return QStringLiteral("%1 (@ <%2>[%3])").arg(formatAddress(m_note))
                                            .arg(m_parent ? m_parent->address() : QStringLiteral("root"))
                                            .arg(row());
}
