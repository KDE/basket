/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NOTEFACTORY_H
#define NOTEFACTORY_H

#include "notecontent.h" //For NoteType::Id

class QColor;
class QPixmap;
class QString;
// class QStringList;

class QUrl;
class QMimeType;

class BasketScene;
class Note;

/** Factory class to create (new, drop, paste) or load BasketIem, and eventually save them (?)
 * @author Sébastien Laoût
 */
namespace NoteFactory
{
/** Functions to create a new note from a content item.
 * Content, if any, is saved to file but the note is not inserted in the basket, and the basket is not saved.
 * Return 0 if the note has not been successfully created.
 * In some cases, the returned note can be a group containing several notes or the first note of a chained list.
 * The method BasketScene::TODO() can insert several grouped or chained notes without problem.
 */
Note *createNoteText(const QString &text, BasketScene *parent, bool reallyPlainText = false);
Note *createNoteHtml(const QString &html, BasketScene *parent);
Note *createNoteLink(const QUrl &url, BasketScene *parent);
Note *createNoteLink(const QUrl &url, const QString &title, BasketScene *parent);
Note *createNoteCrossReference(const QUrl &url, BasketScene *parent);
Note *createNoteCrossReference(const QUrl &url, const QString &title, BasketScene *parent);
Note *createNoteCrossReference(const QUrl &url, const QString &title, const QString &icon, BasketScene *parent);
Note *createNoteImage(const QPixmap &image, BasketScene *parent);
Note *createNoteColor(const QColor &color, BasketScene *parent);
Note *createNoteFromText(const QString &content, BasketScene *parent); // Find automatically the type from the text meaning  // TODO: Return Note::List?
Note *createNoteLauncher(const QUrl &url, BasketScene *parent);
Note *createNoteLauncher(const QString &command, const QString &name, const QString &icon, BasketScene *parent);
Note *createNoteUnknown(const QMimeData *source, BasketScene *parent);
/** Functions to create derived notes from a content */
Note *createNoteLinkOrLauncher(const QUrl &url, BasketScene *parent);
Note *copyFileAndLoad(const QUrl &url, BasketScene *parent);
Note *moveFileAndLoad(const QUrl &url, BasketScene *parent);
Note *loadFile(const QString &fileName,
               BasketScene *parent); /// << Determine the content of the file (the file SHOULD exists) and return a note of the good type.
Note *loadFile(const QString &fileName, NoteType::Id type, BasketScene *parent); /// <<  Create a note of type @p type. The file is not obliged to exist.
/** Functions to create a new note from a drop or paste event */
Note *dropNote(const QMimeData *source, BasketScene *parent, bool fromDrop = false, Qt::DropAction action = Qt::CopyAction, Note *noteSource = nullptr);
bool movingNotesInTheSameBasket(const QMimeData *source, BasketScene *parent, Qt::DropAction action);
Note *dropURLs(QList<QUrl> urls, BasketScene *parent, Qt::DropAction action, bool fromDrop);
Note *decodeContent(QDataStream &stream,
                    NoteType::Id type,
                    BasketScene *parent); /// << Decode the @p stream to a note or return 0 if a general loadFile() is sufficient.
void consumeContent(QDataStream &stream, NoteType::Id type); /// << Decode the @p stream to a note or return 0 if a general loadFile() is sufficient.
/** Functions to create a note file but not load it in a note object */
QString createNoteLauncherFile(const QString &command, const QString &name, const QString &icon, BasketScene *parent);
/** Other useful functions */
NoteType::Id typeForURL(const QUrl &url, BasketScene *parent);
bool maybeText(const QMimeType &mimeType);
bool maybeHtml(const QMimeType &mimeType);
bool maybeImage(const QMimeType &mimeType);
bool maybeAnimation(const QMimeType &mimeType);
bool maybeSound(const QMimeType &mimeType);
bool maybeLauncher(const QMimeType &mimeType);
QString fileNameForNewNote(BasketScene *parent, const QString &wantedName);
QString createFileForNewNote(BasketScene *parent, const QString &extension, const QString &wantedName = QString());
QUrl filteredURL(const QUrl &url);
QString titleForURL(const QUrl &url);
QString iconForURL(const QUrl &url);
QString iconForCommand(const QString &command);
bool isIconExist(const QString &icon);
QStringList textToURLList(const QString &text); // @Return { url1, title1, url2, title2, url3, title3... }
/** Insert GUI menu */
Note *createEmptyNote(NoteType::Id type, BasketScene *parent); // Insert empty if of type Note::Type
Note *importKMenuLauncher(BasketScene *parent);
Note *importIcon(BasketScene *parent);
Note *importFileContent(BasketScene *parent);

void loadNode(const QDomElement &content, const QString &lowerTypeName, Note *parent, bool lazyLoad);
}

#endif // NOTEFACTORY_H
