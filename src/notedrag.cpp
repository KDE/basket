/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "notedrag.h"

#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QDragEnterEvent>
#include <QList>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QStringEncoder>
#include <QTextStream>

#include <KIO/CopyJob>

#include "basketscene.h"
#include "global.h"
#include "notefactory.h"
#include "noteselection.h"
#include "tools.h"

/** NoteDrag */

const char *NoteDrag::NOTE_MIME_STRING = "application/x-basket-note";
QList<Note *> NoteDrag::selectedNotes;

void NoteDrag::createAndEmptyCuttingTmpFolder()
{
    Tools::deleteRecursively(Global::tempCutFolder());
    QDir dir;
    dir.mkdir(Global::tempCutFolder());
}

QDrag *NoteDrag::dragObject(NoteSelection *noteList, bool cutting, QWidget *source)
{
    if (noteList->count() <= 0)
        return nullptr;

    auto *multipleDrag = new QDrag(source);

    // The MimeSource:
    auto *mimeData = new QMimeData;

    // Make sure the temporary folder exists and is empty (we delete previously moved file(s) (if exists)
    // since we override the content of the clipboard and previous file willn't be accessable anymore):
    createAndEmptyCuttingTmpFolder();

    // The "Native Format" Serialization:
    QBuffer buffer;
    if (buffer.open(QIODevice::WriteOnly)) {
        QDataStream stream(&buffer);
        // First append a pointer to the basket:
        stream << (quint64)(noteList->firstStacked()->note->basket());

        // And finally the notes themselves:
        serializeNotes(noteList, stream, cutting);
        // Append the object:
        buffer.close();
        mimeData->setData(QString::fromUtf8(NOTE_MIME_STRING), buffer.buffer());
    }

    // The "Other Flavors" Serialization:
    serializeText(noteList, mimeData);
    serializeHtml(noteList, mimeData);
    serializeImage(noteList, mimeData);
    serializeLinks(noteList, mimeData, cutting);

    // The Alternate Flavors:
    if (noteList->count() == 1)
        noteList->firstStacked()->note->content()->addAlternateDragObjects(mimeData);

    multipleDrag->setMimeData(mimeData);

    // If it is a drag, and not a copy/cut, add the feedback pixmap:
    if (source)
        setFeedbackPixmap(noteList, multipleDrag);

    return multipleDrag;
}

void NoteDrag::serializeNotes(NoteSelection *noteList, QDataStream &stream, bool cutting)
{
    for (NoteSelection *node = noteList; node; node = node->next) {
        stream << (quint64)(node->note);
        if (node->firstChild) {
            stream << (quint64)(NoteType::Group) << (quint64)(node->note->groupWidth()) << (quint64)(node->note->isFolded());
            serializeNotes(node->firstChild, stream, cutting);
        } else {
            NoteContent *content = node->note->content();
            stream << (quint64)(content->type()) << (quint64)(node->note->groupWidth());
            // Serialize file name, and move the file to a temporary place if the note is to be cut.
            // If note does not have file name, we append empty string to be able to easily decode the notes later:
            stream << content->fileName();
            if (content->shouldSerializeFile()) {
                if (cutting) {
                    // Move file in a temporary place:
                    QString fullPath = Global::tempCutFolder() + Tools::fileNameForNewFile(content->fileName(), Global::tempCutFolder());
                    KIO::move(QUrl::fromLocalFile(content->fullPath()), QUrl::fromLocalFile(fullPath), KIO::HideProgressInfo);
                    node->fullPath = fullPath;
                    stream << fullPath;
                } else
                    stream << content->fullPath();
            } else
                stream << QString(QString());
            stream << content->note()->addedDate() << content->note()->lastModificationDate();
            content->serialize(stream);
            State::List states = node->note->states();
            for (State::List::Iterator it = states.begin(); it != states.end(); ++it)
                stream << (quint64)(*it);
            stream << (quint64)0;
        }
    }
    stream << (quint64)0; // Mark the end of the notes in this group/hierarchy.
}

void NoteDrag::serializeText(NoteSelection *noteList, QMimeData *mimeData)
{
    QString textEquivalent;
    QString text;
    for (NoteSelection *node = noteList->firstStacked(); node; node = node->nextStacked()) {
        text = node->note->toText(node->fullPath); // note->toText() and not note->content()->toText() because the first one will also export the tags as text.
        if (!text.isEmpty())
            textEquivalent += (!textEquivalent.isEmpty() ? QStringLiteral("\n") : QString()) + text;
    }
    if (!textEquivalent.isEmpty()) {
        mimeData->setText(textEquivalent);
    }
}

void NoteDrag::serializeHtml(NoteSelection *noteList, QMimeData *mimeData)
{
    QString htmlEquivalent;
    QString html;
    for (NoteSelection *node = noteList->firstStacked(); node; node = node->nextStacked()) {
        html = node->note->content()->toHtml(QString(), node->fullPath);
        if (!html.isEmpty())
            htmlEquivalent += (!htmlEquivalent.isEmpty() ? QStringLiteral("<br>\n") : QString()) + html;
    }
    if (!htmlEquivalent.isEmpty()) {
        // Add HTML flavour:
        mimeData->setHtml(htmlEquivalent);

        // But also QTextEdit flavour, to be able to paste several notes to a text edit:
        QByteArray byteArray = (QStringLiteral("<!--StartFragment--><p>") + htmlEquivalent).toLocal8Bit();
        mimeData->setData(QStringLiteral("application/x-qrichtext"), byteArray);
    }
}

void NoteDrag::serializeImage(NoteSelection *noteList, QMimeData *mimeData)
{
    QList<QPixmap> pixmaps;
    QPixmap pixmap;
    for (NoteSelection *node = noteList->firstStacked(); node; node = node->nextStacked()) {
        pixmap = node->note->content()->toPixmap();
        if (!pixmap.isNull())
            pixmaps.append(pixmap);
    }
    if (!pixmaps.isEmpty()) {
        QPixmap pixmapEquivalent;
        if (pixmaps.count() == 1)
            pixmapEquivalent = pixmaps.first();
        else {
            // Search the total size:
            int height = 0;
            int width = 0;
            for (QList<QPixmap>::iterator it = pixmaps.begin(); it != pixmaps.end(); ++it) {
                height += (*it).height();
                if ((*it).width() > width)
                    width = (*it).width();
            }
            // Create the image by painting all image into one big image:
            pixmapEquivalent = QPixmap(width, height);
            pixmapEquivalent.fill(Qt::white);
            QPainter painter(&pixmapEquivalent);
            height = 0;
            for (QList<QPixmap>::iterator it = pixmaps.begin(); it != pixmaps.end(); ++it) {
                painter.drawPixmap(0, height, *it);
                height += (*it).height();
            }
        }
        mimeData->setImageData(pixmapEquivalent.toImage());
    }
}

void NoteDrag::serializeLinks(NoteSelection *noteList, QMimeData *mimeData, bool cutting)
{
    QList<QUrl> urls;
    QStringList titles;
    QUrl url;
    QString title;
    for (NoteSelection *node = noteList->firstStacked(); node; node = node->nextStacked()) {
        node->note->content()->toLink(&url, &title, node->fullPath);
        if (!url.isEmpty()) {
            urls.append(url);
            titles.append(title);
        }
    }
    if (!urls.isEmpty()) {
        // First, the standard text/uri-list MIME format:
        mimeData->setUrls(urls);

        // Then, also provide it in the Mozilla proprietary format (that also allow to add titles to URLs):
        // A version for Mozilla applications (convert to "theUrl\ntheTitle", into UTF-16):
        // FIXME: Does Mozilla support the drag of several URLs at once?
        // FIXME: If no, only provide that if there is only ONE URL.
        QString xMozUrl;
        for (int i = 0; i < urls.count(); ++i)
            xMozUrl += (xMozUrl.isEmpty() ? QString() : QStringLiteral("\n")) + urls[i].toDisplayString() + QLatin1Char('\n') + titles[i];
        /*      Code for only one: ===============
                xMozUrl = note->title() + "\n" + note->url().toDisplayString();*/
        QByteArray baMozUrl;
        QTextStream stream(baMozUrl, QIODevice::WriteOnly);

        // It's UTF16 (aka UCS2), but with the first two order bytes
        // stream.setEncoding(QTextStream::RawUnicode); // It's UTF16 (aka UCS2), but with the first two order bytes
        // FIXME: find out if this is really equivalent, as https://doc.qt.io/archives/3.3/qtextstream.html pretends

        stream << xMozUrl;

        mimeData->setData(QStringLiteral("text/x-moz-url"), baMozUrl);

        if (cutting) {
            QByteArray arrayCut;
            arrayCut.resize(2);
            arrayCut[0] = '1';
            arrayCut[1] = 0;
            mimeData->setData(QStringLiteral("application/x-kde-cutselection"), arrayCut);
        }
    }
}

void NoteDrag::setFeedbackPixmap(NoteSelection *noteList, QDrag *multipleDrag)
{
    QPixmap pixmap = feedbackPixmap(noteList);
    if (!pixmap.isNull()) {
        multipleDrag->setPixmap(pixmap);
        multipleDrag->setHotSpot(QPoint(-8, -8));
    }
}

QPixmap NoteDrag::feedbackPixmap(NoteSelection *noteList)
{
    if (noteList == nullptr)
        return {};

    static const int MARGIN = 2;
    static const int SPACING = 1;

    QColor textColor = noteList->firstStacked()->note->basket()->textColor();
    QColor backgroundColor = noteList->firstStacked()->note->basket()->backgroundColor().darker(NoteContent::FEEDBACK_DARKING);

    QList<QPixmap> pixmaps;
    QList<QColor> backgrounds;
    QList<bool> spaces;
    QPixmap pixmap;
    int height = 0;
    int width = 0;
    int i = 0;
    bool elipsisImage = false;
    QColor bgColor;
    bool needSpace;
    for (NoteSelection *node = noteList->firstStacked(); node; node = node->nextStacked(), ++i) {
        if (elipsisImage) {
            pixmap = QPixmap(7, 2);
            pixmap.fill(backgroundColor);
            QPainter painter(&pixmap);
            painter.setPen(textColor);
            painter.drawPoint(1, 1);
            painter.drawPoint(3, 1);
            painter.drawPoint(5, 1);
            painter.end();
            bgColor = node->note->basket()->backgroundColor();
            needSpace = false;
        } else {
            pixmap = node->note->content()->feedbackPixmap(/*maxWidth=*/qApp->primaryScreen()->geometry().width() / 2, /*maxHeight=*/96);
            bgColor = node->note->backgroundColor();
            needSpace = node->note->content()->needSpaceForFeedbackPixmap();
        }
        if (!pixmap.isNull()) {
            if (pixmap.width() > width)
                width = pixmap.width();
            pixmaps.append(pixmap);
            backgrounds.append(bgColor);
            spaces.append(needSpace);
            height += (i > 0 && needSpace ? 1 : 0) + pixmap.height() + SPACING + (needSpace ? 1 : 0);
            if (elipsisImage)
                break;
            if (height > qApp->primaryScreen()->geometry().height() / 2)
                elipsisImage = true;
        }
    }
    if (!pixmaps.isEmpty()) {
        QPixmap result(MARGIN + width + MARGIN, MARGIN + height - SPACING + MARGIN - (spaces.last() ? 1 : 0));
        QPainter painter(&result);
        // Draw all the images:
        height = MARGIN;
        QList<QPixmap>::iterator it;
        QList<QColor>::iterator it2;
        QList<bool>::iterator it3;
        int i = 0;
        for (it = pixmaps.begin(), it2 = backgrounds.begin(), it3 = spaces.begin(); it != pixmaps.end(); ++it, ++it2, ++it3, ++i) {
            if (i != 0 && (*it3)) {
                painter.fillRect(MARGIN, height, result.width() - 2 * MARGIN, SPACING, (*it2).darker(NoteContent::FEEDBACK_DARKING));
                ++height;
            }
            painter.drawPixmap(MARGIN, height, *it);
            if ((*it).width() < width)
                painter.fillRect(MARGIN + (*it).width(), height, width - (*it).width(), (*it).height(), (*it2).darker(NoteContent::FEEDBACK_DARKING));
            if (*it3) {
                painter.fillRect(MARGIN, height + (*it).height(), result.width() - 2 * MARGIN, SPACING, (*it2).darker(NoteContent::FEEDBACK_DARKING));
                ++height;
            }
            painter.fillRect(MARGIN, height + (*it).height(), result.width() - 2 * MARGIN, SPACING, Tools::mixColor(textColor, backgroundColor));
            height += (*it).height() + SPACING;
        }
        // Draw the border:
        painter.setPen(textColor);
        painter.drawLine(0, 0, result.width() - 1, 0);
        painter.drawLine(0, 0, 0, result.height() - 1);
        painter.drawLine(0, result.height() - 1, result.width() - 1, result.height() - 1);
        painter.drawLine(result.width() - 1, 0, result.width() - 1, result.height() - 1);
        // Draw the "lightly rounded" border:
        painter.setPen(Tools::mixColor(textColor, backgroundColor));
        painter.drawPoint(0, 0);
        painter.drawPoint(0, result.height() - 1);
        painter.drawPoint(result.width() - 1, result.height() - 1);
        painter.drawPoint(result.width() - 1, 0);
        // Draw the background in the margin (the inside will be painted above, anyway):
        painter.setPen(backgroundColor);
        painter.drawLine(1, 1, result.width() - 2, 1);
        painter.drawLine(1, 1, 1, result.height() - 2);
        painter.drawLine(1, result.height() - 2, result.width() - 2, result.height() - 2);
        painter.drawLine(result.width() - 2, 1, result.width() - 2, result.height() - 2);
        // And assign the feedback pixmap to the drag object:
        // multipleDrag->setPixmap(result, QPoint(-8, -8));
        return result;
    }
    return {};
}

bool NoteDrag::canDecode(const QMimeData *source)
{
    return source->hasFormat(QString::fromUtf8(NOTE_MIME_STRING));
}

BasketScene *NoteDrag::basketOf(const QMimeData *source)
{
    QByteArray srcData = source->data(QString::fromUtf8(NOTE_MIME_STRING));
    QBuffer buffer(&srcData);
    if (buffer.open(QIODevice::ReadOnly)) {
        QDataStream stream(&buffer);
        // Get the parent basket:
        quint64 basketPointer;
        stream >> (quint64 &)basketPointer;
        return (BasketScene *)basketPointer;
    } else
        return nullptr;
}

QList<Note *> NoteDrag::notesOf(QGraphicsSceneDragDropEvent *source)
{
    /* FIXME: this code does not parse the stream properly (see NoteDrag::decode).
       Thus m_draggedNotes will contain many invalid pointer values.
       As a workaround, we use NoteDrag::selectedNotes now. */

    QByteArray srcData = source->mimeData()->data(QString::fromUtf8(NOTE_MIME_STRING));
    QBuffer buffer(&srcData);
    if (buffer.open(QIODevice::ReadOnly)) {
        QDataStream stream(&buffer);
        // Get the parent basket:
        quint64 basketPointer;
        stream >> (quint64 &)basketPointer;
        // Get the note list:
        quint64 notePointer;
        QList<Note *> notes;
        do {
            stream >> notePointer;
            if (notePointer != 0)
                notes.append((Note *)notePointer);
        } while (notePointer);
        // Done:
        return notes;
    } else
        return {};
}

void NoteDrag::saveNoteSelectionToList(NoteSelection *selection)
{
    for (NoteSelection *sel = selection->firstStacked(); sel != nullptr; sel = sel->nextStacked()) {
        if (sel->note->isGroup())
            saveNoteSelectionToList(sel);
        else
            selectedNotes.append(sel->note);
    }
}

Note *NoteDrag::decode(const QMimeData *source, BasketScene *parent, bool moveFiles, bool moveNotes)
{
    QByteArray srcData = source->data(QString::fromUtf8(NOTE_MIME_STRING));
    QBuffer buffer(&srcData);
    if (buffer.open(QIODevice::ReadOnly)) {
        QDataStream stream(&buffer);
        // Get the parent basket:
        quint64 basketPointer;
        stream >> (quint64 &)basketPointer;
        auto *basket = (BasketScene *)basketPointer;
        // Decode the note hierarchy:
        Note *hierarchy = decodeHierarchy(stream, parent, moveFiles, moveNotes, basket);
        // In case we moved notes from one basket to another, save the source basket where notes were removed:
        basket->filterAgainDelayed(); // Delayed, because if a note is moved to the same basket, the note is not at its
        basket->save(); //  new position yet, and the call to ensureNoteVisible would make the interface flicker!!
        return hierarchy;
    } else
        return nullptr;
}

Note *NoteDrag::decodeHierarchy(QDataStream &stream, BasketScene *parent, bool moveFiles, bool moveNotes, BasketScene *originalBasket)
{
    quint64 notePointer;
    quint64 type;
    QString fileName;
    QString fullPath;
    QDateTime addedDate;
    QDateTime lastModificationDate;

    Note *firstNote = nullptr; // TODO: class NoteTreeChunk
    Note *lastInserted = nullptr;

    do {
        stream >> notePointer;
        if (notePointer == 0)
            return firstNote;
        Note *oldNote = (Note *)notePointer;

        Note *note = nullptr;
        quint64 groupWidth;
        stream >> type >> groupWidth;
        if (type == NoteType::Group) {
            note = new Note(parent);
            note->setZValue(-1);
            note->setGroupWidth(groupWidth);
            quint64 isFolded;
            stream >> isFolded;
            if (isFolded)
                note->toggleFolded();
            if (moveNotes) {
                qDebug() << "move notes";
                note->setX(oldNote->targetX()); // We don't move groups but re-create them (every children can to not be selected)
                note->setY(oldNote->targetY()); // We just set the position of the copied group so the animation seems as if the group is the same as (or a copy
                                                // of) the old.
                note->setHeight(oldNote->height()); // Idem: the only use of Note::setHeight()
                parent->removeItem(oldNote);
            }
            Note *children = decodeHierarchy(stream, parent, moveFiles, moveNotes, originalBasket);
            if (children) {
                for (Note *n = children; n; n = n->next())
                    n->setParentNote(note);
                note->setFirstChild(children);
            }
        } else {
            stream >> fileName >> fullPath >> addedDate >> lastModificationDate;
            if (moveNotes) {
                originalBasket->unplugNote(oldNote);
                note = oldNote;
                if (note->basket() != parent && (!fileName.isEmpty() && !fullPath.isEmpty())) {
                    QString newFileName = Tools::fileNameForNewFile(fileName, parent->fullPath());
                    note->content()->setFileName(newFileName);

                    KIO::CopyJob *copyJob = KIO::move(QUrl::fromLocalFile(fullPath),
                                                      QUrl::fromLocalFile(parent->fullPath() + newFileName),
                                                      KIO::Overwrite | KIO::Resume | KIO::HideProgressInfo);
                    parent->connect(copyJob, &KIO::CopyJob::copyingDone, parent, &BasketScene::slotCopyingDone2);
                }
                note->setGroupWidth(groupWidth);
                note->setParentNote(nullptr);
                note->setPrev(nullptr);
                note->setNext(nullptr);
                note->setParentBasket(parent);
                NoteFactory::consumeContent(stream, (NoteType::Id)type);
            } else if ((note = NoteFactory::decodeContent(stream, (NoteType::Id)type, parent))) {
                note->setGroupWidth(groupWidth);
                note->setAddedDate(addedDate);
                note->setLastModificationDate(lastModificationDate);
            } else if (!fileName.isEmpty()) {
                // Here we are CREATING a new EMPTY file, so the name is RESERVED
                // (while dropping several files at once a filename cannot be used by two of them).
                // Later on, file_copy/file_move will copy/move the file to the new location.
                QString newFileName = Tools::fileNameForNewFile(fileName, parent->fullPath());
                // NoteFactory::createFileForNewNote(parent, QString(), fileName);
                KIO::CopyJob *copyJob;
                if (moveFiles) {
                    copyJob = KIO::move(QUrl::fromLocalFile(fullPath),
                                        QUrl::fromLocalFile(parent->fullPath() + newFileName),
                                        KIO::Overwrite | KIO::Resume | KIO::HideProgressInfo);
                } else {
                    copyJob = KIO::copy(QUrl::fromLocalFile(fullPath),
                                        QUrl::fromLocalFile(parent->fullPath() + newFileName),
                                        KIO::Overwrite | KIO::Resume | KIO::HideProgressInfo);
                }
                parent->connect(copyJob, &KIO::CopyJob::copyingDone, parent, &BasketScene::slotCopyingDone2);

                note = NoteFactory::loadFile(newFileName, (NoteType::Id)type, parent);
                note->setGroupWidth(groupWidth);
                note->setAddedDate(addedDate);
                note->setLastModificationDate(lastModificationDate);
            }
        }
        // Retrieve the states (tags) and assign them to the note:
        if (note && note->content()) {
            quint64 statePointer;
            do {
                stream >> statePointer;
                if (statePointer)
                    note->addState((State *)statePointer);
            } while (statePointer);
        }
        // Now that we have created the note, insert it:
        if (note) {
            if (!firstNote)
                firstNote = note;
            else {
                lastInserted->setNext(note);
                note->setPrev(lastInserted);
            }
            lastInserted = note;
        }
    } while (true);

    // We've done: return!
    return firstNote;
}

/** ExtendedTextDrag */

bool ExtendedTextDrag::decode(const QMimeData *e, QString &str)
{
    QString subtype(QStringLiteral("plain"));
    return decode(e, str, subtype);
}

bool ExtendedTextDrag::decode(const QMimeData *e, QString &str, QString &subtype)
{
    // Get the string:
    bool ok;
    QString mime = e->text();
    ok = !str.isNull();

    // Test if it was a UTF-16 string (from eg. Mozilla):
    if (str.length() >= 2) {
        if ((mime[0] == QLatin1Char(0xFF) && mime[1] == QLatin1Char(0xFE)) || (mime[0] == QLatin1Char(0xFE) && mime[1] == QLatin1Char(0xFF))) {
            auto fromUtf16 = QStringEncoder(QStringEncoder::Utf8);
            QByteArray encodedString = fromUtf16(str);
            str = QString::fromUtf8(encodedString);
            return true;
        }
    }

    // Test if it was empty (sometimes, from GNOME or Mozilla)
    if (str.length() == 0 && subtype == QStringLiteral("plain")) {
        if (e->hasFormat(QStringLiteral("UTF8_STRING"))) {
            auto fromUtf8 = QStringEncoder(QStringEncoder::Utf16);
            QByteArray encodedString = fromUtf8(str);
            str = QString::fromUtf8(encodedString);
            return true;
        }
        if (e->hasFormat(QStringLiteral("text/unicode"))) { // FIXME: It's UTF-16 without order bytes!!!
            auto fromUtf16 = QStringEncoder(QStringEncoder::Utf8);
            QByteArray encodedString = fromUtf16(str);
            str = QString::fromUtf8(encodedString);
            return true;
        }
        if (e->hasFormat(QStringLiteral("TEXT"))) { // local encoding
            QByteArray text = e->data(QStringLiteral("TEXT"));
            str = QString::fromUtf8(text);
            return true;
        }
        if (e->hasFormat(QStringLiteral("COMPOUND_TEXT"))) { // local encoding
            QByteArray text = e->data(QStringLiteral("COMPOUND_TEXT"));
            str = QString::fromUtf8(text);
            return true;
        }
    }
    return ok;
}

#include "moc_notedrag.cpp"
