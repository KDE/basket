/**
 * SPDX-FileCopyrightText: (C) 2006 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "basket_export.h"

#include <QList>
#include <QMap>

class BasketScene;
class Tag;

class QString;
// class QStringList;
class QDomNode;
class QProgressDialog;
class QDomElement;

class KTar;

/**
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class Archive
{
public:
    static void save(BasketScene *basket, bool withSubBaskets, const QString &destination);
    static void open(const QString &path);

    /**
     * @brief The IOErrorCode enum granularly indicates whether encoding or decoding of baskets archive succeded
     */
    enum class IOErrorCode : quint8 {
        NoError,
        NotABasketArchive,
        CorruptedBasketArchive,
        DestinationExists,
        IncompatibleBasketVersion,
        PossiblyCompatibleBasketVersion,
        FailedToOpenResource,
    };

    /**
     * @brief extractArchive decodes .baskets files
     * @param path to the .baskets file
     * @param destination into which the basket archive should be extracted
     * @param protectDestination decides whether the destination will be replaced if it has been present
     * \todo protectDestination likely should be an enum, too, to be more descriptive
     * @return NoError indicates a sucessful extraction. All other enum states indicate something went wrong
     */
    BASKET_EXPORT static IOErrorCode extractArchive(const QString &path, const QString &destination, const bool protectDestination = true);

    /**
     * @brief createArchiveFromSource encodes a basket directory into a .baskets file
     *
     * Be aware, this function currently does not validate the sourcePath's structure. The caller of this function needs to make sure the basket directory is a
     * valid source.
     *
     * @param sourcePath specifies the directory to be encoded
     * @param previewImage specifies an optional .png image
     * @param destination specifies where the encoded .basket file should be saved
     * @param protectDestination decides whether the destination will be replaced if it has been present
     * @return NoError indicates a sucessful extraction. All other enum states indicate something went wrong
     */
    BASKET_EXPORT static IOErrorCode createArchiveFromSource(const QString &sourcePath,
                                                             const QString &previewImage,
                                                             const QString &destination = QString(),
                                                             const bool protectDestination = true);

private:
    // Convenient Methods for Saving:
    static void
    saveBasketToArchive(BasketScene *basket, bool recursive, KTar *tar, QStringList &backgrounds, const QString &tempFolder, QProgressDialog *progress);
    static void listUsedTags(BasketScene *basket, bool recursive, QList<Tag *> &list);
    // Convenient Methods for Loading:
    static void renameBasketFolders(const QString &extractionFolder, QMap<QString, QString> &mergedStates);
    static void
    renameBasketFolder(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, QMap<QString, QString> &mergedStates);
    static void renameMergedStatesAndBasketIcon(const QString &fullPath, QMap<QString, QString> &mergedStates, const QString &extractionFolder);
    static void renameMergedStates(QDomNode notes, QMap<QString, QString> &mergedStates);
    static void importBasketIcon(QDomElement properties, const QString &extractionFolder);
    static void loadExtractedBaskets(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, BasketScene *parent);
    static void importTagEmblems(const QString &extractionFolder);
    static void importArchivedBackgroundImages(const QString &extractionFolder);
};

#endif
