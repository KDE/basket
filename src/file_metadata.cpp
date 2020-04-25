/**
 * SPDX-FileCopyrightText: (C) 2015 by Gleb Baryshev <gleb.baryshev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "file_metadata.h"
#include <KLocalizedString>

using namespace KFileMetaData::Property;

void MetaDataExtractionResult::add(Property property, const QVariant &value)
{
    m_groups[property] = value;
}

QList<QPair<QString, QString>> MetaDataExtractionResult::preferredGroups()
{
    // Text captions for some properties. Unspecified properties will get empty captions
    static const QMap<Property, QString> PROPERTY_TRANSLATIONS = {{Property::BitRate, i18n("Bit rate")},
                                                                  {Property::Channels, i18n("Channels")},
                                                                  {Property::Duration, i18n("Duration")},
                                                                  {Property::Genre, i18n("Genre")},
                                                                  {Property::SampleRate, i18n("Sample rate")},
                                                                  {Property::TrackNumber, i18n("Track number")},
                                                                  {Property::Comment, i18n("Comment")},
                                                                  {Property::Artist, i18n("Artist")},
                                                                  {Property::Album, i18n("Album")},
                                                                  {Property::Title, i18n("Title")},
                                                                  {Property::WordCount, i18n("Word count")},
                                                                  {Property::LineCount, i18n("Line count")},
                                                                  {Property::Copyright, i18n("Copyright")},
                                                                  {Property::CreationDate, i18n("Date")},
                                                                  {Property::FrameRate, i18n("Frame rate")}};

    static QList<Property> preferredItems;
    if (preferredItems.count() == 0) {
        // According to KDE 3 services/kfile_*.desktop

        // audio
        preferredItems << Property::Title << Property::Artist << Property::Album << Property::TrackNumber << Property::Genre << Property::BitRate << Property::Duration << Property::CreationDate << Property::Comment << Property::SampleRate
                       << Property::Channels << Property::Copyright;
        // video
        preferredItems /*<< Property::Duration*/ << Property::FrameRate;
        // text
        preferredItems << Property::LineCount << Property::WordCount;
    }

    QList<QPair<QString, QString>> result;
    if (m_groups.count() == 0)
        return result;

    KFileMetaData::PropertyMap groups = m_groups;

    for (int i = 0; i < preferredItems.count(); i++) {
        QVariant value = groups.take(preferredItems[i]);
        if (value.isValid())
            result.append({PROPERTY_TRANSLATIONS[preferredItems[i]], value.toString()});
    }

    // Remaining groups
    for (auto it = groups.begin(); it != groups.end(); it++) {
        result.append({PROPERTY_TRANSLATIONS[it.key()], it.value().toString()});
    }

    return result;
}
