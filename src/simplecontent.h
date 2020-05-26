/*
 * SPDX-FileCopyrightText: 2020 by Ismael Asensio <isma.af@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "notecontent.h"    // To use NoteType

class QDomElement;


class SimpleContent {
public:
    SimpleContent (NoteType::Id type, const QString &basketPath = QString());

    NoteType::Id type() const;
    QVariantMap attributes() const;
    QString toText(const QString &unused) const;

    void loadFromXMLNode(QDomElement node);

private:
    QVariantMap defaultAttributes() const;    // TODO: Implement later as subclass override

private:
    NoteType::Id m_type = NoteType::Unknown;
    QString m_basketPath;
    QString m_filePath;
    QByteArray m_rawData;
    QVariantMap m_attributes;
};
