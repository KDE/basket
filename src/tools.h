/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TOOLS_H
#define TOOLS_H

#include "basket_export.h"

#include <QtCore/QVector>

class QColor;
class QFont;
class QMimeData;
class QObject;
class QPixmap;
class QString;
class QStringList;
class QTime;
class QTextDocument;

class HTMLExporter;

class StopWatch
{
public:
    static void start(int id);
    static void check(int id);

private:
    static QVector<QTime> starts;
    static QVector<double> totals;
    static QVector<uint> counts;
};

/** Some useful functions for that application.
 * @author Sébastien Laoût
 */
namespace Tools
{
// Text <-> HTML Conversions and Tools:
BASKET_EXPORT QString textToHTML(const QString &text);
BASKET_EXPORT QString textToHTMLWithoutP(const QString &text);
BASKET_EXPORT QString htmlToParagraph(const QString &html);
BASKET_EXPORT QString htmlToText(const QString &html);
BASKET_EXPORT QString textDocumentToMinimalHTML(QTextDocument *document); //!< Avoid unneeded spans and style attributes

BASKET_EXPORT QString tagURLs(const QString &test);
BASKET_EXPORT QString cssFontDefinition(const QFont &font, bool onlyFontFamily = false);

// Cross Reference tools:
BASKET_EXPORT QString tagCrossReferences(const QString &text, bool userLink = false, HTMLExporter *exporter = nullptr);
// private functions:
BASKET_EXPORT QString crossReferenceForBasket(QStringList linkParts);
BASKET_EXPORT QString crossReferenceForHtml(QStringList linkParts, HTMLExporter *exporter);
BASKET_EXPORT QString crossReferenceForConversion(QStringList linkParts);

// String Manipulations:
BASKET_EXPORT QString stripEndWhiteSpaces(const QString &string);
BASKET_EXPORT QString makeStandardCaption(const QString &userCaption); //!< Replacement for KDialog::makeStandardCaption

// Pixmap Manipulations:
/** @Return the CSS color name for the given @p colorHex in #rrggbb format, or empty string if none matches
 */
BASKET_EXPORT QString cssColorName(const QString &colorHex);
/** @Return true if it is a Web color
 */
BASKET_EXPORT bool isWebColor(const QColor &color);
/** @Return a color that is 50% of @p color1 and 50% of @p color2.
 */
BASKET_EXPORT QColor mixColor(const QColor &color1, const QColor &color2, const float ratio = 1);
/** @Return true if the color is too dark to be darkened one more time.
 */
BASKET_EXPORT bool tooDark(const QColor &color);
/** Make sure the @p pixmap is of the size (@p width, @p height) and @return a pixmap of this size.
 * If @p height <= 0, then width will be used to make the picture square.
 */
BASKET_EXPORT QPixmap normalizePixmap(const QPixmap &pixmap, int width, int height = 0);
/** @Return the pixmap @p source with depth*deltaX transparent pixels added to the left.\n
 * If @p deltaX is <= 0, then an indent delta is computed depending on the @p source width.
 */
BASKET_EXPORT QPixmap indentPixmap(const QPixmap &source, int depth, int deltaX = 0);

// File and Folder Manipulations:
/** Delete the folder @p folderOrFile recursively (to remove sub-folders and child files too).
 */
BASKET_EXPORT void deleteRecursively(const QString &folderOrFile);
/** Trash the folder @p folderOrFile recursively (to move sub-folders and child files to the Trash, too).
 */
BASKET_EXPORT void trashRecursively(const QString &folderOrFile);
/** Delete the metadata of file or folder @p folderOrFile from Nepomuk, recursively.
 */
BASKET_EXPORT void deleteMetadataRecursively(const QString &folderOrFile);
/** @Return a new filename that doesn't already exist in @p destFolder.
 * If @p wantedName already exist in @p destFolder, a dash and a number will be added before the extension.
 * Id there were already such a number in @p wantedName, it is incremented until a free filename is found.
 */
BASKET_EXPORT QString fileNameForNewFile(const QString &wantedName, const QString &destFolder);

//! @returns Total size in bytes of all files and subdirectories
BASKET_EXPORT qint64 computeSizeRecursively(const QString &path);

// Other:
// void iconForURL(const QUrl &url);
/** @Return true if the source is from a file cutting in Konqueror.
 * @Return false if it was just a copy or if it was a drag.
 */
BASKET_EXPORT bool isAFileCut(const QMimeData *source);

/// Implementation of system encoding detection from KDE 4
BASKET_EXPORT QByteArray systemCodeset();

// Debug
BASKET_EXPORT void printChildren(QObject *parent);
}

#endif // TOOLS_H
