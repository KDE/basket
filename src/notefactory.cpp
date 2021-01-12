/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "notefactory.h"

#include <QDomElement>
#include <QFileDialog>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QLocale>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeData>
#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QVector>
#include <QtCore/qnamespace.h>
#include <QtGui/QBitmap> //For createHeuristicMask
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QImageReader>
#include <QtGui/QMovie>
#include <QtGui/QPixmap>
#include <QtGui/QTextDocument> //For Qt::mightBeRichText(...)

#include <KAboutData> //For KGlobal::mainComponent().aboutData(...)
#include <KIconDialog>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KModifierKeyInfo>
#include <KOpenWithDialog>
#include <KUriFilter>

#include <KIO/CopyJob>

#include "basketlistview.h"
#include "basketscene.h"
#include "file_mimetypes.h"
#include "global.h"
#include "note.h"
#include "notedrag.h"
#include "settings.h"
#include "tools.h"
#include "variouswidgets.h" //For IconSizeDialog
#include "xmlwork.h"

#include "debugwindow.h"

/** Create notes from scratch (just a content) */

Note *NoteFactory::createNoteText(const QString &text, BasketScene *parent, bool reallyPlainText /* = false*/)
{
    QList<State*> tags; int tagsLength = 0; Note* note;

    if (Settings::detectTextTags())
    {
        tags = Tools::detectTags(text, tagsLength);
    }
    QString textConverted = text.mid(tagsLength);

    if (reallyPlainText)
    {
        note = new Note(parent);
        TextContent *content = new TextContent(note, createFileForNewNote(parent, "txt"));
        content->setText(text);
        content->saveToFile();
    }
        else
    {
        textConverted = QString("<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><meta name=\"qrichtext\" content=\"1\" /></head><body>%1</body></html>")
            .arg(Tools::textToHTMLWithoutP(textConverted));

        note = createNoteHtml(textConverted, parent);
    }

    if (note)
    for (State* state: tags)
    {
        note->addState(state, true);
    }

    return note;
}

Note *NoteFactory::createNoteHtml(const QString &html, BasketScene *parent)
{
    Note *note = new Note(parent);
    HtmlContent *content = new HtmlContent(note, createFileForNewNote(parent, "html"));
    content->setHtml(html);
    content->saveToFile();
    return note;
}

Note *NoteFactory::createNoteLink(const QUrl &url, BasketScene *parent)
{
    Note *note = new Note(parent);
    new LinkContent(note, url, titleForURL(url), iconForURL(url), /*autoTitle=*/true, /*autoIcon=*/true);
    return note;
}

Note *NoteFactory::createNoteLink(const QUrl &url, const QString &title, BasketScene *parent)
{
    Note *note = new Note(parent);
    new LinkContent(note, url, title, iconForURL(url), /*autoTitle=*/false, /*autoIcon=*/true);
    return note;
}

Note *NoteFactory::createNoteCrossReference(const QUrl &url, BasketScene *parent)
{
    Note *note = new Note(parent);
    new CrossReferenceContent(note, url, titleForURL(url), iconForURL(url));
    return note;
}

Note *NoteFactory::createNoteCrossReference(const QUrl &url, const QString &title, BasketScene *parent)
{
    Note *note = new Note(parent);
    new CrossReferenceContent(note, url, title, iconForURL(url));
    return note;
}

Note *NoteFactory::createNoteCrossReference(const QUrl &url, const QString &title, const QString &icon, BasketScene *parent)
{
    Note *note = new Note(parent);
    new CrossReferenceContent(note, url, title, icon);
    return note;
}

Note *NoteFactory::createNoteImage(const QPixmap &image, BasketScene *parent)
{
    Note *note = new Note(parent);
    ImageContent *content = new ImageContent(note, createFileForNewNote(parent, "png"));
    content->setPixmap(image);
    content->saveToFile();
    return note;
}

Note *NoteFactory::createNoteColor(const QColor &color, BasketScene *parent)
{
    Note *note = new Note(parent);
    new ColorContent(note, color);
    return note;
}

/** Return a string list containing {url1, title1, url2, title2, url3, title3...}
 */
QStringList NoteFactory::textToURLList(const QString &text)
{
    // List to return:
    QStringList list;

    // Split lines:
    QStringList texts = text.split('\n');

    // For each lines:
    QStringList::iterator it;
    for (it = texts.begin(); it != texts.end(); ++it) {
        // Strip white spaces:
        (*it) = (*it).trimmed();

        // Don't care of empty entries:
        if ((*it).isEmpty())
            continue;

        // Compute lower case equivalent:
        QString ltext = (*it).toLower();

        /* Search for mail address ("*@*.*" ; "*" can contain '_', '-', or '.') and add protocol to it */
        QString mailExpString = "[\\w-\\.]+@[\\w-\\.]+\\.[\\w]+";
        QRegExp mailExp("^" + mailExpString + '$');
        if (mailExp.exactMatch(ltext)) {
            ltext.insert(0, "mailto:");
            (*it).insert(0, "mailto:");
        }

        // TODO: Recognize "<link>" (link between '<' and '>')
        // TODO: Replace " at " by "@" and " dot " by "." to look for e-mail addresses

        /* Search for mail address like "Name <address@provider.net>" */
        QRegExp namedMailExp("^([\\w\\s]+)\\s<(" + mailExpString + ")>$");
        // namedMailExp.setCaseSensitive(true); // For the name to be keeped with uppercases // DOESN'T WORK !
        if (namedMailExp.exactMatch(ltext)) {
            QString name = namedMailExp.cap(1);
            QString address = "mailto:" + namedMailExp.cap(2);
            // Threat it NOW, as it's an exception (it have a title):
            list.append(address);
            list.append(name);
            continue;
        }

        /* Search for an url and create an URL note */
        if ((ltext.startsWith('/') && ltext[1] != '/' && ltext[1] != '*') || // Take files but not C/C++/... comments !
            ltext.startsWith(QLatin1String("file:")) || ltext.startsWith(QLatin1String("http://")) || ltext.startsWith(QLatin1String("https://")) || ltext.startsWith(QLatin1String("www.")) || ltext.startsWith(QLatin1String("ftp.")) ||
            ltext.startsWith(QLatin1String("ftp://")) || ltext.startsWith(QLatin1String("mailto:"))) {
            // First, correct the text to use the good format for the url
            if (ltext.startsWith('/'))
                (*it).insert(0, "file:");
            if (ltext.startsWith(QLatin1String("www.")))
                (*it).insert(0, "http://");
            if (ltext.startsWith(QLatin1String("ftp.")))
                (*it).insert(0, "ftp://");

            // And create the Url note (or launcher if URL point a .desktop file)
            list.append(*it);
            list.append(QString()); // We don't have any title
        } else
            return QStringList(); // FAILED: treat the text as a text, and not as a URL list!
    }
    return list;
}

Note *NoteFactory::createNoteFromText(const QString &text, BasketScene *parent)
{
    /* Search for a color (#RGB , #RRGGBB , #RRRGGGBBB , #RRRRGGGGBBBB) and create a color note */
    QRegExp exp("^#(?:[a-fA-F\\d]{3}){1,4}$");
    if (exp.exactMatch(text))
        return createNoteColor(QColor(text), parent);

    /* Try to convert the text as a URL or a list of URLs */
    QStringList uriList = textToURLList(text);
    if (!uriList.isEmpty()) {
        // TODO: This code is almost duplicated from fropURLs()!
        Note *note;
        Note *firstNote = nullptr;
        Note *lastInserted = nullptr;
        QStringList::iterator it;
        for (it = uriList.begin(); it != uriList.end(); ++it) {
            QString url = (*it);
            ++it;
            QString title = (*it);
            if (title.isEmpty())
                note = createNoteLinkOrLauncher(QUrl::fromUserInput(url), parent);
            else
                note = createNoteLink(QUrl::fromUserInput(url), title, parent);

            // If we got a new note, insert it in a linked list (we will return the first note of that list):
            if (note) {
                //              qDebug() << "Drop URL: " << (*it).toDisplayString();
                if (!firstNote)
                    firstNote = note;
                else {
                    lastInserted->setNext(note);
                    note->setPrev(lastInserted);
                }
                lastInserted = note;
            }
        }
        return firstNote; // It don't return ALL inserted notes !
    }

    // QString newText = text.trimmed(); // The text for a new note, without useless spaces
    /* Else, it's a text or an HTML note, so, create it */
    if (Qt::mightBeRichText(/*newT*/ text))
        return createNoteHtml(/*newT*/ text, parent);
    else
        return createNoteText(/*newT*/ text, parent);
}

Note *NoteFactory::createNoteLauncher(const QUrl &url, BasketScene *parent)
{
    if (url.isEmpty())
        return createNoteLauncher(QString(), QString(), QString(), parent);
    else
        return copyFileAndLoad(url, parent);
}

Note *NoteFactory::createNoteLauncher(const QString &command, const QString &name, const QString &icon, BasketScene *parent)
{
    QString fileName = createNoteLauncherFile(command, name, icon, parent);
    if (fileName.isEmpty())
        return nullptr;
    else
        return loadFile(fileName, parent);
}

QString NoteFactory::createNoteLauncherFile(const QString &command, const QString &name, const QString &icon, BasketScene *parent)
{
    QString content = QString(
                          "[Desktop Entry]\n"
                          "Exec=%1\n"
                          "Name=%2\n"
                          "Icon=%3\n"
                          "Encoding=UTF-8\n"
                          "Type=Application\n")
                          .arg(command, name, icon.isEmpty() ? QString("exec") : icon);
    QString fileName = fileNameForNewNote(parent, "launcher.desktop");
    QString fullPath = parent->fullPathForFileName(fileName);
    //  parent->dontCareOfCreation(fullPath);
    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << content;
        file.close();
        return fileName;
    } else
        return QString();
}

Note *NoteFactory::createNoteLinkOrLauncher(const QUrl &url, BasketScene *parent)
{
    // IMPORTANT: we create the service ONLY if the extension is ".desktop".
    //            Otherwise, KService take a long time to analyze all the file
    //            and output such things to stdout:
    //            "Invalid entry (missing '=') at /my/file.ogg:11984"
    //            "Invalid entry (missing ']') at /my/file.ogg:11984"...
    KService::Ptr service;
    if (url.fileName().endsWith(QLatin1String(".desktop")))
        service = new KService(url.path());

    // If link point to a .desktop file then add a launcher, otherwise it's a link
    if (service && service->isValid())
        return createNoteLauncher(url, parent);
    else
        return createNoteLink(url, parent);
}

bool NoteFactory::movingNotesInTheSameBasket(const QMimeData *source, BasketScene *parent, Qt::DropAction action)
{
    if (NoteDrag::canDecode(source))
        return action == Qt::MoveAction && NoteDrag::basketOf(source) == parent;
    else
        return false;
}

Note *NoteFactory::dropNote(const QMimeData *source, BasketScene *parent, bool fromDrop, Qt::DropAction action, Note * /*noteSource*/)
{
    if (source == nullptr) {
        return nullptr;
    }

    Note *note = nullptr;

    QStringList formats = source->formats();
    /* No data */
    if (formats.size() == 0) {
        // TODO: add a parameter to say if it's from a clipboard paste, a selection paste, or a drop
        //       To be able to say "The clipboard/selection/drop is empty".
        //      KMessageBox::error(parent, i18n("There is no data to insert."), i18n("No Data"));
        return nullptr;
    }

    /* Debug */
    if (Global::debugWindow) {
        *Global::debugWindow << "<b>Drop :</b>";
        for (int i = 0; i < formats.size(); ++i)
            *Global::debugWindow << "\t[" + QString::number(i) + "] " + formats[i];
        switch (action) { // The source want that we:
        case Qt::CopyAction:
            *Global::debugWindow << ">> Drop action: Copy";
            break;
        case Qt::MoveAction:
            *Global::debugWindow << ">> Drop action: Move";
            break;
        case Qt::LinkAction:
            *Global::debugWindow << ">> Drop action: Link";
            break;
        default:
            *Global::debugWindow << ">> Drop action: Unknown"; //  supported by Qt!
        }
    }

    /* Copy or move a Note */
    if (NoteDrag::canDecode(source)) {
        bool moveFiles = fromDrop && action == Qt::MoveAction;
        bool moveNotes = moveFiles;
        return NoteDrag::decode(source, parent, moveFiles, moveNotes); // Filename will be kept
    }

    /* Else : Drop object to note */

    QImage image = qvariant_cast<QImage>(source->imageData());
    if (!image.isNull())
        return createNoteImage(QPixmap::fromImage(image), parent);

    if (source->hasColor()) {
        return createNoteColor(qvariant_cast<QColor>(source->colorData()), parent);
    }

    // And then the hack (if provide color MIME type or a text that contains color), using createNote Color RegExp:
    QString hack;
    QRegExp exp("^#(?:[a-fA-F\\d]{3}){1,4}$");
    hack = source->text();
    if (source->hasFormat("application/x-color") || (!hack.isNull() && exp.exactMatch(hack))) {
        QColor color = qvariant_cast<QColor>(source->colorData());
        if (color.isValid())
            return createNoteColor(color, parent);
        //          if ( (note = createNoteColor(color, parent)) )
        //              return note;
        //          // Theoretically it should be returned. If not, continue by dropping other things
    }

    QList<QUrl> urls = source->urls();
    if (!urls.isEmpty()) {
        // If it's a Paste, we should know if files should be copied (copy&paste) or moved (cut&paste):
        if (!fromDrop && Tools::isAFileCut(source))
            action = Qt::MoveAction;
        return dropURLs(urls, parent, action, fromDrop);
    }

    // FIXME: use dropURLs() also from Mozilla?

    /*
     * Mozilla's stuff sometimes uses utf-16-le - little-endian UTF-16.
     *
     * This has the property that for the ASCII subset case (And indeed, the
     * ISO-8859-1 subset, I think), if you treat it as a C-style string,
     * it'll come out to one character long in most cases, since it looks
     * like:
     *
     * "<\0H\0T\0M\0L\0>\0"
     *
     * A strlen() call on that will give you 1, which simply isn't correct.
     * That might, I suppose, be the answer, or something close.
     *
     * Also, Mozilla's drag/drop code predates the use of MIME types in XDnD
     * - hence it'll throw about STRING and UTF8_STRING quite happily, hence
     * the odd named types.
     *
     * Thanks to Dave Cridland for having said me that.
     */
    if (source->hasFormat("text/x-moz-url")) { // FOR MOZILLA
        // Get the array and create a QChar array of 1/2 of the size
        QByteArray mozilla = source->data("text/x-moz-url");
        QVector<QChar> chars(mozilla.count() / 2);
        // A small debug work to know the value of each bytes
        if (Global::debugWindow)
            for (int i = 0; i < mozilla.count(); i++)
                *Global::debugWindow << QString("'") + QChar(mozilla[i]) + "' " + QString::number(int(mozilla[i]));
        // text/x-moz-url give the URL followed by the link title and separated by OxOA (10 decimal: new line?)
        uint size = 0;
        QChar *name = nullptr;
        // For each little endian mozilla chars, copy it to the array of QChars
        for (int i = 0; i < mozilla.count(); i += 2) {
            chars[i / 2] = QChar(mozilla[i], mozilla[i + 1]);
            if (mozilla.at(i) == 0x0A) {
                size = i / 2;
                name = &(chars[i / 2 + 1]);
            }
        }
        // Create a QString that take the address of the first QChar and a length
        if (name == nullptr) { // We haven't found name (FIXME: Is it possible ?)
            QString normalHtml(&(chars[0]), chars.size());
            return createNoteLink(normalHtml, parent);
        } else {
            QString normalHtml(&(chars[0]), size);
            QString normalTitle(name, chars.size() - size - 1);
            return createNoteLink(normalHtml, normalTitle, parent);
        }
    }

    if (source->hasFormat("text/html")) {
        QString html;
        QString subtype("html");
        // If the text/html comes from Mozilla or GNOME it can be UTF-16 encoded: we need ExtendedTextDrag to check that
        ExtendedTextDrag::decode(source, html, subtype);
        return createNoteHtml(html, parent);
    }

    QString text;
    // If the text/plain comes from GEdit or GNOME it can be empty: we need ExtendedTextDrag to check other MIME types
    if (ExtendedTextDrag::decode(source, text))
        return createNoteFromText(text, parent);

    /* Create a cross reference note */

    if (source->hasFormat(BasketTreeListView::TREE_ITEM_MIME_STRING)) {
        QByteArray data = source->data(BasketTreeListView::TREE_ITEM_MIME_STRING);
        QDataStream stream(&data, QIODevice::ReadOnly);
        QString basketName, folderName, icon;

        while (!stream.atEnd())
            stream >> basketName >> folderName >> icon;

        return createNoteCrossReference(QUrl("basket://" + folderName), basketName, icon, parent);
    }

    /* Unsuccessful drop */
    note = createNoteUnknown(source, parent);
    QString message = i18n(
        "<p>%1 doesn't support the data you've dropped.<br>"
        "It however created a generic note, allowing you to drag or copy it to an application that understand it.</p>"
        "<p>If you want the support of these data, please contact developer.</p>",
        QGuiApplication::applicationDisplayName());
    KMessageBox::information(parent->graphicsView()->viewport(), message, i18n("Unsupported MIME Type(s)"), "unsupportedDropInfo", KMessageBox::AllowLink);
    return note;
}

Note *NoteFactory::createNoteUnknown(const QMimeData *source, BasketScene *parent /*, const QString &annotations*/)
{
    // Save the MimeSource in a file: create and open the file:
    QString fileName = createFileForNewNote(parent, "unknown");
    QFile file(parent->fullPath() + fileName);
    if (!file.open(QIODevice::WriteOnly))
        return nullptr;
    QDataStream stream(&file);

    // Echo MIME types:
    QStringList formats = source->formats();
    for (int i = 0; formats.size(); ++i)
        stream << QString(formats[i]); // Output the '\0'-terminated format name string

    // Echo end of MIME types list delimiter:
    stream << QString();

    // Echo the length (in bytes) and then the data, and then same for next MIME type:
    for (int i = 0; formats.size(); ++i) {
        QByteArray data = source->data(formats[i]);
        stream << (quint32)data.count();
        stream.writeRawData(data.data(), data.count());
    }
    file.close();

    Note *note = new Note(parent);
    new UnknownContent(note, fileName);
    return note;
}

Note *NoteFactory::dropURLs(QList<QUrl> urls, BasketScene *parent, Qt::DropAction action, bool fromDrop)
{
    KModifierKeyInfo keyinfo;
    int shouldAsk = 0; // shouldAsk==0: don't ask ; shouldAsk==1: ask for "file" ; shouldAsk>=2: ask for "files"
    bool shiftPressed = keyinfo.isKeyPressed(Qt::Key_Shift);
    bool ctrlPressed = keyinfo.isKeyPressed(Qt::Key_Control);
    bool modified = fromDrop && (shiftPressed || ctrlPressed);

    if (modified)        // Then no menu + modified action
        ;                // action is already set: no work to do
    else if (fromDrop) { // Compute if user should be asked or not
        for (QList<QUrl>::iterator it = urls.begin(); it != urls.end(); ++it)
            if ((*it).scheme() != "mailto") { // Do not ask when dropping mail address :-)
                shouldAsk++;
                if (shouldAsk == 1 /*2*/) // Sufficient
                    break;
            }
        if (shouldAsk) {
            QMenu menu(parent->graphicsView());
            QList<QAction *> actList;
            actList << new QAction(QIcon::fromTheme("go-jump"), i18n("&Move Here\tShift"), &menu) << new QAction(QIcon::fromTheme("edit-copy"), i18n("&Copy Here\tCtrl"), &menu)
                    << new QAction(QIcon::fromTheme("insert-link"), i18n("&Link Here\tCtrl+Shift"), &menu);

            Q_FOREACH (QAction *a, actList)
                menu.addAction(a);

            menu.addSeparator();
            menu.addAction(QIcon::fromTheme("dialog-cancel"), i18n("C&ancel\tEscape"));
            int id = actList.indexOf(menu.exec(QCursor::pos()));
            switch (id) {
            case 0:
                action = Qt::MoveAction;
                break;
            case 1:
                action = Qt::CopyAction;
                break;
            case 2:
                action = Qt::LinkAction;
                break;
            default:
                return nullptr;
            }
            modified = true;
        }
    } else { // fromPaste
        ;
    }

    /* Policy of drops of URL:
    *   Email: [Modifier keys: Useless]
    +    - Link mail address
    *   Remote URL: [Modifier keys: {Copy,Link}]
    +    - Download as Image, Animation and Launcher
    +    - Link other URLs
    *   Local URL: [Modifier keys: {Copy,Move,Link}]
    *    - Copy as Image, Animation and Launcher [Modifier keys: {Copy,Move,Link}]
    *    - Link folder [Modifier keys: Useless]
    *    - Make Launcher of executable [Modifier keys: {Copy_exec,Move_exec,Link_Launcher}]
    *    - Ask for file (if use want to copy and it is a sound: make Sound)
    * Policy of pastes of URL: [NO modifier keys]
    *   - Same as drops
    *   - But copy when ask should be done
    *   - Unless cut-selection is true: move files instead
    * Policy of file created in the basket dir: [NO modifier keys]
    *   - View as Image, Animation, Sound, Launcher
    *   - View as File
    */
    Note *note;
    Note *firstNote = nullptr;
    Note *lastInserted = nullptr;
    for (QList<QUrl>::iterator it = urls.begin(); it != urls.end(); ++it) {
        if (((*it).scheme() == "mailto") || (action == Qt::LinkAction))
            note = createNoteLinkOrLauncher(*it, parent);
        //         else if (!(*it).isLocalFile()) {
        //             if (action != Qt::LinkAction && (maybeImageOrAnimation(*it)/* || maybeSound(*it)*/))
        //                 note = copyFileAndLoad(*it, parent);
        //             else
        //                 note = createNoteLinkOrLauncher(*it, parent);
        //         }
        else {
            if (action == Qt::CopyAction)
                note = copyFileAndLoad(*it, parent);
            else if (action == Qt::MoveAction)
                note = moveFileAndLoad(*it, parent);
            else
                note = createNoteLinkOrLauncher(*it, parent);
        }

        // If we got a new note, insert it in a linked list (we will return the first note of that list):
        if (note) {
            DEBUG_WIN << "Drop URL: " + (*it).toDisplayString();
            if (!firstNote)
                firstNote = note;
            else {
                lastInserted->setNext(note);
                note->setPrev(lastInserted);
            }
            lastInserted = note;
        }
    }
    return firstNote;
}

void NoteFactory::consumeContent(QDataStream &stream, NoteType::Id type)
{
    if (type == NoteType::Link) {
        QUrl url;
        QString title, icon;
        quint64 autoTitle64, autoIcon64;
        stream >> url >> title >> icon >> autoTitle64 >> autoIcon64;
    } else if (type == NoteType::CrossReference) {
        QUrl url;
        QString title, icon;
        stream >> url >> title >> icon;
    } else if (type == NoteType::Color) {
        QColor color;
        stream >> color;
    }
}

Note *NoteFactory::decodeContent(QDataStream &stream, NoteType::Id type, BasketScene *parent)
{
    /*  if (type == NoteType::Text) {
        QString text;
        stream >> text;
        return NoteFactory::createNoteText(text, parent);
    } else if (type == NoteType::Html) {
        QString html;
        stream >> html;
        return NoteFactory::createNoteHtml(html, parent);
    } else if (type == NoteType::Image) {
        QPixmap pixmap;
        stream >> pixmap;
        return NoteFactory::createNoteImage(pixmap, parent);
    } else */
    if (type == NoteType::Link) {
        QUrl url;
        QString title, icon;
        quint64 autoTitle64, autoIcon64;
        bool autoTitle, autoIcon;
        stream >> url >> title >> icon >> autoTitle64 >> autoIcon64;
        autoTitle = (bool)autoTitle64;
        autoIcon = (bool)autoIcon64;
        Note *note = NoteFactory::createNoteLink(url, parent);
        ((LinkContent *)(note->content()))->setLink(url, title, icon, autoTitle, autoIcon);
        return note;
    } else if (type == NoteType::CrossReference) {
        QUrl url;
        QString title, icon;
        stream >> url >> title >> icon;
        Note *note = NoteFactory::createNoteCrossReference(url, parent);
        ((CrossReferenceContent *)(note->content()))->setCrossReference(url, title, icon);
        return note;
    } else if (type == NoteType::Color) {
        QColor color;
        stream >> color;
        return NoteFactory::createNoteColor(color, parent);
    } else
        return nullptr; // NoteFactory::loadFile() is sufficient
}

bool NoteFactory::maybeText(const QMimeType &mimeType)
{
    return mimeType.inherits(MimeTypes::TEXT);
}

bool NoteFactory::maybeHtml(const QMimeType &mimeType)
{
    return mimeType.inherits(MimeTypes::HTML);
}

bool NoteFactory::maybeImage(const QMimeType &mimeType)
{
    return mimeType.name().startsWith(MimeTypes::IMAGE);
}

bool NoteFactory::maybeAnimation(const QMimeType &mimeType)
{
    return mimeType.inherits(MimeTypes::ANIMATION) || mimeType.name() == MimeTypes::ANIMATION_MNG;
}

bool NoteFactory::maybeSound(const QMimeType &mimeType)
{
    return mimeType.name().startsWith(MimeTypes::AUDIO);
}

bool NoteFactory::maybeLauncher(const QMimeType &mimeType)
{
    return mimeType.inherits(MimeTypes::LAUNCHER);
}

////////////// NEW:

Note *NoteFactory::copyFileAndLoad(const QUrl &url, BasketScene *parent)
{
    QString fileName = fileNameForNewNote(parent, url.fileName());
    QString fullPath = parent->fullPathForFileName(fileName);

    if (Global::debugWindow)
        *Global::debugWindow << "copyFileAndLoad: " + url.toDisplayString() + " to " + fullPath;

    //  QString annotations = i18n("Original file: %1", url.toDisplayString());
    //  parent->dontCareOfCreation(fullPath);

    KIO::CopyJob *copyJob = KIO::copy(url, QUrl::fromLocalFile(fullPath), KIO::Overwrite | KIO::Resume);
    parent->connect(copyJob, &KIO::CopyJob::copyingDone, parent, &BasketScene::slotCopyingDone2);

    NoteType::Id type = typeForURL(url, parent); // Use the type of the original file because the target doesn't exist yet
    return loadFile(fileName, type, parent);
}

Note *NoteFactory::moveFileAndLoad(const QUrl &url, BasketScene *parent)
{
    // Globally the same as copyFileAndLoad() but move instead of copy (KIO::move())
    QString fileName = fileNameForNewNote(parent, url.fileName());
    QString fullPath = parent->fullPathForFileName(fileName);

    if (Global::debugWindow)
        *Global::debugWindow << "moveFileAndLoad: " + url.toDisplayString() + " to " + fullPath;

    //  QString annotations = i18n("Original file: %1", url.toDisplayString());
    //  parent->dontCareOfCreation(fullPath);

    KIO::CopyJob *copyJob = KIO::move(url, QUrl::fromLocalFile(fullPath), KIO::Overwrite | KIO::Resume);

    parent->connect(copyJob, &KIO::CopyJob::copyingDone, parent, &BasketScene::slotCopyingDone2);

    NoteType::Id type = typeForURL(url, parent); // Use the type of the original file because the target doesn't exist yet
    return loadFile(fileName, type, parent);
}

Note *NoteFactory::loadFile(const QString &fileName, BasketScene *parent)
{
    // The file MUST exists
    QFileInfo file(QUrl::fromLocalFile(parent->fullPathForFileName(fileName)).path());
    if (!file.exists())
        return nullptr;

    NoteType::Id type = typeForURL(parent->fullPathForFileName(fileName), parent);
    Note *note = loadFile(fileName, type, parent);
    return note;
}

Note *NoteFactory::loadFile(const QString &fileName, NoteType::Id type, BasketScene *parent)
{
    Note *note = new Note(parent);
    switch (type) {
    case NoteType::Text:
        new TextContent(note, fileName);
        break;
    case NoteType::Html:
        new HtmlContent(note, fileName);
        break;
    case NoteType::Image:
        new ImageContent(note, fileName);
        break;
    case NoteType::Animation:
        new AnimationContent(note, fileName);
        break;
    case NoteType::Sound:
        new SoundContent(note, fileName);
        break;
    case NoteType::File:
        new FileContent(note, fileName);
        break;
    case NoteType::Launcher:
        new LauncherContent(note, fileName);
        break;
    case NoteType::Unknown:
        new UnknownContent(note, fileName);
        break;

    default:
    case NoteType::Link:
    case NoteType::CrossReference:
    case NoteType::Color:
        return nullptr;
    }

    return note;
}

NoteType::Id NoteFactory::typeForURL(const QUrl &url, BasketScene * /*parent*/)
{
    bool viewText = Settings::viewTextFileContent();
    bool viewHTML = Settings::viewHtmlFileContent();
    bool viewImage = Settings::viewImageFileContent();
    bool viewSound = Settings::viewSoundFileContent();

    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForUrl(url);

    if (Global::debugWindow) {
        if (mimeType.isValid())
            *Global::debugWindow << "typeForURL: " + url.toDisplayString() + " ; MIME type = " + mimeType.name();
        else
            *Global::debugWindow << "typeForURL: mimeType is empty for " + url.toDisplayString();
    }

    // Go from specific to more generic
    if (maybeLauncher(mimeType))
        return NoteType::Launcher;
    else if (viewHTML && (maybeHtml(mimeType)))
        return NoteType::Html;
    else if (viewText && maybeText(mimeType))
        return NoteType::Text;
    else if (viewImage && maybeAnimation(mimeType))
        return NoteType::Animation; // See Note::movieStatus(int)
    else if (viewImage && maybeImage(mimeType))
        return NoteType::Image; //  for more explanations
    else if (viewSound && maybeSound(mimeType))
        return NoteType::Sound;
    else
        return NoteType::File;
}

QString NoteFactory::fileNameForNewNote(BasketScene *parent, const QString &wantedName)
{
    return Tools::fileNameForNewFile(wantedName, parent->fullPath());
}

// Create a file to store a new note in Basket parent and with extension extension.
// If wantedName is provided, the function will first try to use this file name, or derive it if it's impossible
//  (extension willn't be used for that case)
QString NoteFactory::createFileForNewNote(BasketScene *parent, const QString &extension, const QString &wantedName)
{
    QString fileName;
    QString fullName;

    if (wantedName.isEmpty()) { // TODO: fileNameForNewNote(parent, "note1."+extension);
        QDir dir;
        int nb = parent->count() + 1;
        QString time = QTime::currentTime().toString("hhmmss");

        for (;; ++nb) {
            fileName = QString("note%1-%2.%3").arg(nb).arg(time).arg(extension);
            fullName = parent->fullPath() + fileName;
            dir = QDir(fullName);
            if (!dir.exists(fullName))
                break;
        }
    } else {
        fileName = fileNameForNewNote(parent, wantedName);
        fullName = parent->fullPath() + fileName;
    }

    // Create the file
    //  parent->dontCareOfCreation(fullName);
    QFile file(fullName);
    file.open(QIODevice::WriteOnly);
    file.close();

    return fileName;
}

QUrl NoteFactory::filteredURL(const QUrl &url)
{
    // KURIFilter::filteredURI() is slow if the URL contains only letters, digits and '-' or '+'.
    // So, we don't use that function is that case:
    bool isSlow = true;
    for (int i = 0; i < url.url().length(); ++i) {
        QChar c = url.url()[i];
        if (!c.isLetterOrNumber() && c != '-' && c != '+') {
            isSlow = false;
            break;
        }
    }
    if (isSlow)
        return url;
    else {
        QStringList list;
        list << QLatin1String("kshorturifilter") << QLatin1String("kuriikwsfilter") << QLatin1String("kurisearchfilter") << QLatin1String("localdomainfilter") << QLatin1String("fixuphosturifilter");
        return KUriFilter::self()->filteredUri(url, list);
    }
}

QString NoteFactory::titleForURL(const QUrl &url)
{
    QString title = url.toDisplayString();
    QString home = "file:" + QDir::homePath() + '/';

    if (title.startsWith(QLatin1String("mailto:")))
        return title.remove(0, 7);

    if (title.startsWith(home))
        title = "~/" + title.remove(0, home.length());

    if (title.startsWith(QLatin1String("file://")))
        title = title.remove(0, 7); // 7 == QString("file://").length() - 1
    else if (title.startsWith(QLatin1String("file:")))
        title = title.remove(0, 5); // 5 == QString("file:").length() - 1
    else if (title.startsWith(QLatin1String("http://www.")))
        title = title.remove(0, 11); // 11 == QString("http://www.").length() - 1
    else if (title.startsWith(QLatin1String("https://www.")))
        title = title.remove(0, 12); // 12 == QString("https://www.").length() - 1
    else if (title.startsWith(QLatin1String("http://")))
        title = title.remove(0, 7); // 7 == QString("http://").length() - 1
    else if (title.startsWith(QLatin1String("https://")))
        title = title.remove(0, 8); // 8 == QString("https://").length() - 1

    if (!url.isLocalFile()) {
        if (title.endsWith(QLatin1String("/index.html")) && title.length() > 11)
            title.truncate(title.length() - 11); // 11 == QString("/index.html").length()
        else if (title.endsWith(QLatin1String("/index.htm")) && title.length() > 10)
            title.truncate(title.length() - 10); // 10 == QString("/index.htm").length()
        else if (title.endsWith(QLatin1String("/index.xhtml")) && title.length() > 12)
            title.truncate(title.length() - 12); // 12 == QString("/index.xhtml").length()
        else if (title.endsWith(QLatin1String("/index.php")) && title.length() > 10)
            title.truncate(title.length() - 10); // 10 == QString("/index.php").length()
        else if (title.endsWith(QLatin1String("/index.asp")) && title.length() > 10)
            title.truncate(title.length() - 10); // 10 == QString("/index.asp").length()
        else if (title.endsWith(QLatin1String("/index.php3")) && title.length() > 11)
            title.truncate(title.length() - 11); // 11 == QString("/index.php3").length()
        else if (title.endsWith(QLatin1String("/index.php4")) && title.length() > 11)
            title.truncate(title.length() - 11); // 11 == QString("/index.php4").length()
        else if (title.endsWith(QLatin1String("/index.php5")) && title.length() > 11)
            title.truncate(title.length() - 11); // 11 == QString("/index.php5").length()
    }

    if (title.length() > 2 && title.endsWith('/')) // length > 2 because "/" and "~/" shouldn't be transformed to QString() and "~"
        title.truncate(title.length() - 1);        // eg. transform "www.kde.org/" to "www.kde.org"

    return title;
}

QString NoteFactory::iconForURL(const QUrl &url)
{
    if (url.scheme() == "mailto") {
        return QStringLiteral("message");
    }
    // return KMimeType::iconNameForUrl(url.url());
    return QString();
}

// TODO: Can I add "autoTitle" and "autoIcon" entries to .desktop files? or just store them in basket, as now...

/* Try our better to find an icon suited to the command line
 * eg. "/usr/bin/kwrite-3.2 ~/myfile.txt /home/other/file.xml"
 * will give the "kwrite" icon!
 */
QString NoteFactory::iconForCommand(const QString &command)
{
    QString icon;

    // 1. Use first word as icon (typically the program without argument)
    icon = command.split(' ').first();
    // 2. If the command is a full path, take only the program file name
    icon = icon.mid(icon.lastIndexOf('/') + 1); // strip path if given [But it doesn't care of such
    // "myprogram /my/path/argument" -> return "argument". Would
    // must first strip first word and then strip path... Useful ??
    // 3. Use characters before any '-' (e.g. use "gimp" icon if run command is "gimp-1.3")
    if (!isIconExist(icon))
        icon = icon.split('-').first();
    // 4. If the icon still not findable, use a generic icon
    if (!isIconExist(icon))
        icon = "exec";

    return icon;
}

bool NoteFactory::isIconExist(const QString &icon)
{
    return !KIconLoader::global()->loadIcon(icon, KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, true).isNull();
}

Note *NoteFactory::createEmptyNote(NoteType::Id type, BasketScene *parent)
{
    QPixmap *pixmap;
    switch (type) {
    case NoteType::Text:
        return NoteFactory::createNoteText(QString(), parent, /*reallyPlainText=*/true);
    case NoteType::Html:
        return NoteFactory::createNoteHtml(QString(), parent);
    case NoteType::Image:
        pixmap = new QPixmap(QSize(Settings::defImageX(), Settings::defImageY()));
        pixmap->fill();
        pixmap->setMask(pixmap->createHeuristicMask());
        return NoteFactory::createNoteImage(*pixmap, parent);
    case NoteType::Link:
        return NoteFactory::createNoteLink(QUrl(), parent);
    case NoteType::CrossReference:
        return NoteFactory::createNoteCrossReference(QUrl(), parent);
    case NoteType::Launcher:
        return NoteFactory::createNoteLauncher(QUrl(), parent);
    case NoteType::Color:
        return NoteFactory::createNoteColor(Qt::black, parent);
    default:
    case NoteType::Animation:
    case NoteType::Sound:
    case NoteType::File:
    case NoteType::Unknown:
        return nullptr; // Not possible!
    }
}

Note *NoteFactory::importKMenuLauncher(BasketScene *parent)
{
    QPointer<KOpenWithDialog> dialog = new KOpenWithDialog(parent->graphicsView()->viewport());
    dialog->setSaveNewApplications(true); // To create temp file, needed by createNoteLauncher()
    dialog->exec();
    if (dialog->service()) {
        // * locateLocal() return a local file even if it is a system wide one (local one doesn't exists)
        // * desktopEntryPath() returns the full path for system wide resources, but relative path if in home
        QString serviceFilePath = dialog->service()->entryPath();
        if (!serviceFilePath.startsWith('/'))
            serviceFilePath = dialog->service()->locateLocal();
        return createNoteLauncher(QUrl::fromUserInput(serviceFilePath), parent);
    }
    return nullptr;
}

Note *NoteFactory::importIcon(BasketScene *parent)
{
    QString iconName = KIconDialog::getIcon(KIconLoader::Desktop, KIconLoader::Application, false, Settings::defIconSize());
    if (!iconName.isEmpty()) {
        QPointer<IconSizeDialog> dialog = new IconSizeDialog(i18n("Import Icon as Image"), i18n("Choose the size of the icon to import as an image:"), iconName, Settings::defIconSize(), nullptr);
        dialog->exec();
        if (dialog->iconSize() > 0) {
            Settings::setDefIconSize(dialog->iconSize());
            Settings::saveConfig();
            return createNoteImage(QIcon::fromTheme(iconName).pixmap(dialog->iconSize()), parent); // TODO: wantedName = iconName !
        }
    }
    return nullptr;
}

Note *NoteFactory::importFileContent(BasketScene *parent)
{
    QUrl url = QFileDialog::getOpenFileUrl(parent->graphicsView(), i18n("Load File Content into a Note"), QUrl(), QString());
    if (!url.isEmpty())
        return copyFileAndLoad(url, parent);
    return nullptr;
}

void NoteFactory::loadNode(const QDomElement &content, const QString &lowerTypeName, Note *parent, bool lazyLoad)
{
    if (lowerTypeName == "text") {
        new TextContent(parent, content.text(), lazyLoad);
    } else if (lowerTypeName == "html") {
        new HtmlContent(parent, content.text(), lazyLoad);
    } else if (lowerTypeName == "image") {
        new ImageContent(parent, content.text(), lazyLoad);
    } else if (lowerTypeName == "animation") {
        new AnimationContent(parent, content.text(), lazyLoad);
    } else if (lowerTypeName == "sound") {
        new SoundContent(parent, content.text());
    } else if (lowerTypeName == "file")  {
        new FileContent(parent, content.text());
    } else if (lowerTypeName == "link") {
        bool autoTitle = content.attribute("title") == content.text();
        bool autoIcon = content.attribute("icon") == NoteFactory::iconForURL(QUrl::fromUserInput(content.text()));
        autoTitle = XMLWork::trueOrFalse(content.attribute("autoTitle"), autoTitle);
        autoIcon = XMLWork::trueOrFalse(content.attribute("autoIcon"), autoIcon);
        new LinkContent(parent, QUrl::fromUserInput(content.text()), content.attribute("title"), content.attribute("icon"), autoTitle, autoIcon);
    } else if (lowerTypeName == "cross_reference") {
        new CrossReferenceContent(parent, QUrl::fromUserInput(content.text()), content.attribute("title"), content.attribute("icon"));
    } else if (lowerTypeName == "launcher") {
        new LauncherContent(parent, content.text());
    } else if (lowerTypeName == "color") {
        new ColorContent(parent, QColor(content.text()));
    } else if (lowerTypeName == "unknown") {
        new UnknownContent(parent, content.text());
    }
}
