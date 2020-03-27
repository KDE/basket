/*
 *   Copyright (C) 2015 by Gleb Baryshev <gleb.baryshev@gmail.com>
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

#ifndef FILE_METADATA_H
#define FILE_METADATA_H

#include <QList>

#include <KFileMetaData/KFileMetaData/ExtractionResult>
#include <KFileMetaData/KFileMetaData/ExtractorCollection>

/// Store and retrieve metadata extraction results
class MetaDataExtractionResult : public KFileMetaData::ExtractionResult
{
public:
    MetaDataExtractionResult(const QString& url, const QString& mimetype) :
        KFileMetaData::ExtractionResult(url, mimetype, KFileMetaData::ExtractionResult::ExtractMetaData) { }

    void append(const QString&) override { } //not used
    void add(KFileMetaData::Property::Property property, const QVariant& value) override;
    void addType(KFileMetaData::Type::Type) override { } //not used

    /// Get preferred metadata as "property-value" pairs
    QList<QPair<QString, QString> > preferredGroups();

private:
    KFileMetaData::PropertyMap m_groups;
};

#endif // FILE_METADATA_H
