/**
 * SPDX-FileCopyrightText: (C) 2006 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <QtCore/QList>
#include <QtCore/QMap>

class BasketScene;
class Tag;

class QString;
class QStringList;
class QDomNode;
class QProgressDialog;
class QDomElement;

class KTar;
class KProgress;

/**
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class Archive
{
public:
    static void save(BasketScene *basket, bool withSubBaskets, const QString &destination);
    static void open(const QString &path);

private:
    // Convenient Methods for Saving:
    static void saveBasketToArchive(BasketScene *basket, bool recursive, KTar *tar, QStringList &backgrounds, const QString &tempFolder, QProgressDialog *progress);
    static void listUsedTags(BasketScene *basket, bool recursive, QList<Tag *> &list);
    // Convenient Methods for Loading:
    static void renameBasketFolders(const QString &extractionFolder, QMap<QString, QString> &mergedStates);
    static void renameBasketFolder(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, QMap<QString, QString> &mergedStates);
    static void renameMergedStatesAndBasketIcon(const QString &fullPath, QMap<QString, QString> &mergedStates, const QString &extractionFolder);
    static void renameMergedStates(QDomNode notes, QMap<QString, QString> &mergedStates);
    static void importBasketIcon(QDomElement properties, const QString &extractionFolder);
    static void loadExtractedBaskets(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, BasketScene *parent);
    static void importTagEmblems(const QString &extractionFolder);
    static void importArchivedBackgroundImages(const QString &extractionFolder);
};

#endif
