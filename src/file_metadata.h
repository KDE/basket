/**
 * SPDX-FileCopyrightText: (C) 2015 by Gleb Baryshev <gleb.baryshev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
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
    MetaDataExtractionResult(const QString &url, const QString &mimetype)
        : KFileMetaData::ExtractionResult(url, mimetype, KFileMetaData::ExtractionResult::ExtractMetaData)
    {
    }

    void append(const QString &) override
    {
    } // not used
    void add(KFileMetaData::Property::Property property, const QVariant &value) override;
    void addType(KFileMetaData::Type::Type) override
    {
    } // not used

    /// Get preferred metadata as "property-value" pairs
    QList<QPair<QString, QString>> preferredGroups();

private:
    KFileMetaData::PropertyMap m_groups;
};

#endif // FILE_METADATA_H
