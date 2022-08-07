/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketscene.h"

#include <QApplication>
#include <QFileDialog>
#include <QFrame>
#include <QGraphicsProxyWidget>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QToolTip>
#include <QtCore/QDateTime> // seed for rand()
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QPoint>
#include <QtCore/QStringList>
#include <QtCore/QTimeLine>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QCursor>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDragLeaveEvent>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QTextDocument>
#include <QtGui/QWheelEvent>
#include <QtXml/QDomDocument>

#include <KTextEdit>
#include <KAboutData>
#include <KActionCollection>
#include <KAuthorized>
#include <KColorScheme> // for KStatefulBrush
#include <KDirWatch>
#include <KGlobalAccel>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KOpenWithDialog>
#include <KRun>
#include <KService>
#include <KDialogJobUiDelegate>

#include <KIO/ApplicationLauncherJob>
#include <KIO/CopyJob>

#include <stdlib.h> // rand() function

#include "backgroundmanager.h"
#include "basketview.h"
#include "common.h"
#include "debugwindow.h"
#include "decoratedbasket.h"
#include "focusedwidgets.h"
#include "gitwrapper.h"
#include "global.h"
#include "note.h"
#include "notedrag.h"
#include "noteedit.h"
#include "notefactory.h"
#include "noteselection.h"
#include "settings.h"
#include "tagsedit.h"
#include "tools.h"
#include "xmlwork.h"

#include "config.h"

#ifdef HAVE_LIBGPGME
#include "kgpgme.h"
#endif

void debugZone(int zone)
{
    QString s;
    switch (zone) {
    case Note::Handle:
        s = "Handle";
        break;
    case Note::Group:
        s = "Group";
        break;
    case Note::TagsArrow:
        s = "TagsArrow";
        break;
    case Note::Custom0:
        s = "Custom0";
        break;
    case Note::GroupExpander:
        s = "GroupExpander";
        break;
    case Note::Content:
        s = "Content";
        break;
    case Note::Link:
        s = "Link";
        break;
    case Note::TopInsert:
        s = "TopInsert";
        break;
    case Note::TopGroup:
        s = "TopGroup";
        break;
    case Note::BottomInsert:
        s = "BottomInsert";
        break;
    case Note::BottomGroup:
        s = "BottomGroup";
        break;
    case Note::BottomColumn:
        s = "BottomColumn";
        break;
    case Note::None:
        s = "None";
        break;
    default:
        if (zone == Note::Emblem0)
            s = "Emblem0";
        else
            s = "Emblem0+" + QString::number(zone - Note::Emblem0);
        break;
    }
    qDebug() << s;
}

#define FOR_EACH_NOTE(noteVar) for (Note *noteVar = firstNote(); noteVar; noteVar = noteVar->next())


void BasketScene::appendNoteIn(Note *note, Note *in)
{
    if (!note)
        // No note to append:
        return;

    if (in) {
        // The normal case:
        preparePlug(note);

        //      Note *last = note->lastSibling();
        Note *lastChild = in->lastChild();

        for (Note *n = note; n; n = n->next())
            n->setParentNote(in);
        note->setPrev(lastChild);
        //      last->setNext(0L);

        if (!in->firstChild())
            in->setFirstChild(note);

        if (lastChild)
            lastChild->setNext(note);

        if (m_loaded)
            signalCountsChanged();
    } else
        // Prepend it directly in the basket:
        appendNoteAfter(note, lastNote());
}

void BasketScene::appendNoteAfter(Note *note, Note *after)
{
    if (!note)
        // No note to append:
        return;

    if (!after)
        // By default, insert after the last note:
        after = lastNote();

    if (m_loaded && after && !after->isFree() && !after->isColumn())
        for (Note *n = note; n; n = n->next())
            n->inheritTagsOf(after);

    //  if (!alreadyInBasket)
    preparePlug(note);

    Note *last = note->lastSibling();
    if (after) {
        // The normal case:
        for (Note *n = note; n; n = n->next())
            n->setParentNote(after->parentNote());
        note->setPrev(after);
        last->setNext(after->next());
        after->setNext(note);
        if (last->next())
            last->next()->setPrev(last);
    } else {
        // There is no note in the basket:
        for (Note *n = note; n; n = n->next())
            n->setParentNote(nullptr);
        m_firstNote = note;
        //      note->setPrev(0);
        //      last->setNext(0);
    }

    //  if (!alreadyInBasket)
    if (m_loaded)
        signalCountsChanged();
}

void BasketScene::appendNoteBefore(Note *note, Note *before)
{
    if (!note)
        // No note to append:
        return;

    if (!before)
        // By default, insert before the first note:
        before = firstNote();

    if (m_loaded && before && !before->isFree() && !before->isColumn())
        for (Note *n = note; n; n = n->next())
            n->inheritTagsOf(before);

    preparePlug(note);

    Note *last = note->lastSibling();
    if (before) {
        // The normal case:
        for (Note *n = note; n; n = n->next())
            n->setParentNote(before->parentNote());
        note->setPrev(before->prev());
        last->setNext(before);
        before->setPrev(last);
        if (note->prev())
            note->prev()->setNext(note);
        else {
            if (note->parentNote())
                note->parentNote()->setFirstChild(note);
            else
                m_firstNote = note;
        }
    } else {
        // There is no note in the basket:
        for (Note *n = note; n; n = n->next())
            n->setParentNote(nullptr);
        m_firstNote = note;
        //      note->setPrev(0);
        //      last->setNext(0);
    }

    if (m_loaded)
        signalCountsChanged();
}

DecoratedBasket *BasketScene::decoration()
{
    return (DecoratedBasket *)parent();
}

void BasketScene::preparePlug(Note *note)
{
    // Select only the new notes, compute the new notes count and the new number of found notes:
    if (m_loaded)
        unselectAll();
    int count = 0;
    int founds = 0;
    Note *last = nullptr;
    for (Note *n = note; n; n = n->next()) {
        if (m_loaded)
            n->setSelectedRecursively(true); // Notes should have a parent basket (and they have, so that's OK).
        count += n->count();
        founds += n->newFilter(decoration()->filterData());
        last = n;
    }
    m_count += count;
    m_countFounds += founds;

    // Focus the last inserted note:
    if (m_loaded && last) {
        setFocusedNote(last);
        m_startOfShiftSelectionNote = (last->isGroup() ? last->lastRealChild() : last);
    }

    // If some notes don't match (are hidden), tell it to the user:
    if (m_loaded && founds < count) {
        if (count == 1) {
            Q_EMIT postMessage(i18n("The new note does not match the filter and is hidden."));
        } else if (founds == count - 1) {
            Q_EMIT postMessage(i18n("A new note does not match the filter and is hidden."));
        } else if (founds > 0) {
            Q_EMIT postMessage(i18n("Some new notes do not match the filter and are hidden."));
        } else {
            Q_EMIT postMessage(i18n("The new notes do not match the filter and are hidden."));
        }
    }
}

void BasketScene::unplugNote(Note *note)
{
    // If there is nothing to do...
    if (!note)
        return;

    //  if (!willBeReplugged) {
    note->setSelectedRecursively(false); // To removeSelectedNote() and decrease the selectedsCount.
    m_count -= note->count();
    m_countFounds -= note->newFilter(decoration()->filterData());
    signalCountsChanged();
    //  }

    // If it was the first note, change the first note:
    if (m_firstNote == note)
        m_firstNote = note->next();

    // Change previous and next notes:
    if (note->prev())
        note->prev()->setNext(note->next());
    if (note->next())
        note->next()->setPrev(note->prev());

    if (note->parentNote()) {
        // If it was the first note of a group, change the first note of the group:
        if (note->parentNote()->firstChild() == note)
            note->parentNote()->setFirstChild(note->next());

        if (!note->parentNote()->isColumn()) {
            // Delete parent if now 0 notes inside parent group:
            if (!note->parentNote()->firstChild()) {
                unplugNote(note->parentNote());
                // a group could call this method for one or more of its children,
                // each children could call this method for its parent's group...
                // we have to do the deletion later otherwise we may corrupt the current process
                m_notesToBeDeleted << note;
                if (m_notesToBeDeleted.count() == 1) {
                    QTimer::singleShot(0, this, SLOT(doCleanUp()));
                }
            }
            // Ungroup if still 1 note inside parent group:
            else if (!note->parentNote()->firstChild()->next()) {
                ungroupNote(note->parentNote());
            }
        }
    }

    note->setParentNote(nullptr);
    note->setPrev(nullptr);
    note->setNext(nullptr);

    // Reste focus and hover note if necessary
    if (m_focusedNote == note)
        m_focusedNote = nullptr;
    if (m_hoveredNote == note)
        m_hoveredNote = nullptr;

    //  recomputeBlankRects(); // FIXME: called too much time. It's here because when dragging and moving a note to another basket and then go back to the original basket, the note is deleted but the note rect is not painter anymore.
}

void BasketScene::ungroupNote(Note *group)
{
    Note *note = group->firstChild();
    Note *lastGroupedNote = group;
    Note *nextNote;

    // Move all notes after the group (not before, to avoid to change m_firstNote or group->m_firstChild):
    while (note) {
        nextNote = note->next();

        if (lastGroupedNote->next())
            lastGroupedNote->next()->setPrev(note);
        note->setNext(lastGroupedNote->next());
        lastGroupedNote->setNext(note);
        note->setParentNote(group->parentNote());
        note->setPrev(lastGroupedNote);

        note->setGroupWidth(group->groupWidth() - Note::GROUP_WIDTH);
        lastGroupedNote = note;
        note = nextNote;
    }

    // Unplug the group:
    group->setFirstChild(nullptr);
    unplugNote(group);
    // a group could call this method for one or more of its children,
    // each children could call this method for its parent's group...
    // we have to do the deletion later otherwise we may corrupt the current process
    m_notesToBeDeleted << group;
    if (m_notesToBeDeleted.count() == 1) {
        QTimer::singleShot(0, this, SLOT(doCleanUp()));
    }
}

void BasketScene::groupNoteBefore(Note *note, Note *with)
{
    if (!note || !with)
        // No note to group or nowhere to group it:
        return;

    //  if (m_loaded && before && !with->isFree() && !with->isColumn())
    for (Note *n = note; n; n = n->next())
        n->inheritTagsOf(with);

    preparePlug(note);

    Note *last = note->lastSibling();

    Note *group = new Note(this);
    group->setPrev(with->prev());
    group->setNext(with->next());
    group->setX(with->x());
    group->setY(with->y());
    if (with->parentNote() && with->parentNote()->firstChild() == with)
        with->parentNote()->setFirstChild(group);
    else if (m_firstNote == with)
        m_firstNote = group;
    group->setParentNote(with->parentNote());
    group->setFirstChild(note);
    group->setGroupWidth(with->groupWidth() + Note::GROUP_WIDTH);

    if (with->prev())
        with->prev()->setNext(group);
    if (with->next())
        with->next()->setPrev(group);
    with->setParentNote(group);
    with->setPrev(last);
    with->setNext(nullptr);

    for (Note *n = note; n; n = n->next())
        n->setParentNote(group);
    //  note->setPrev(0L);
    last->setNext(with);

    if (m_loaded)
        signalCountsChanged();
}

void BasketScene::groupNoteAfter(Note *note, Note *with)
{
    if (!note || !with)
        // No note to group or nowhere to group it:
        return;

    //  if (m_loaded && before && !with->isFree() && !with->isColumn())
    for (Note *n = note; n; n = n->next())
        n->inheritTagsOf(with);

    preparePlug(note);

    //  Note *last = note->lastSibling();

    Note *group = new Note(this);
    group->setPrev(with->prev());
    group->setNext(with->next());
    group->setX(with->x());
    group->setY(with->y());
    if (with->parentNote() && with->parentNote()->firstChild() == with)
        with->parentNote()->setFirstChild(group);
    else if (m_firstNote == with)
        m_firstNote = group;
    group->setParentNote(with->parentNote());
    group->setFirstChild(with);
    group->setGroupWidth(with->groupWidth() + Note::GROUP_WIDTH);

    if (with->prev())
        with->prev()->setNext(group);
    if (with->next())
        with->next()->setPrev(group);
    with->setParentNote(group);
    with->setPrev(nullptr);
    with->setNext(note);

    for (Note *n = note; n; n = n->next())
        n->setParentNote(group);
    note->setPrev(with);
    //  last->setNext(0L);

    if (m_loaded)
        signalCountsChanged();
}

void BasketScene::doCleanUp()
{
    QSet<Note *>::iterator it = m_notesToBeDeleted.begin();
    while (it != m_notesToBeDeleted.end()) {
        delete *it;
        it = m_notesToBeDeleted.erase(it);
    }
}

void BasketScene::loadNotes(const QDomElement &notes, Note *parent)
{
    Note *note;
    for (QDomNode n = notes.firstChild(); !n.isNull(); n = n.nextSibling()) {
        QDomElement e = n.toElement();
        if (e.isNull()) // Cannot handle that!
            continue;
        note = nullptr;
        // Load a Group:
        if (e.tagName() == "group") {
            note = new Note(this); // 1. Create the group...
            loadNotes(e, note);    // 3. ... And populate it with child notes.
            int noteCount = note->count();
            if (noteCount > 0 || (parent == nullptr && !isFreeLayout())) { // But don't remove columns!
                appendNoteIn(note, parent);                                // 2. ... Insert it... FIXME: Initially, the if() the insertion was the step 2. Was it on purpose?
                // The notes in the group are counted two times (it's why appendNoteIn() was called before loadNotes):
                m_count -= noteCount; // TODO: Recompute note count every time noteCount() is emitted!
                m_countFounds -= noteCount;
            }
        }
        // Load a Content-Based Note:
        if (e.tagName() == "note" || e.tagName() == "item") {                                                                          // Keep compatible with 0.6.0 Alpha 1
            note = new Note(this);                                                                                                     // Create the note...
            NoteFactory::loadNode(XMLWork::getElement(e, "content"), e.attribute("type"), note, /*lazyLoad=*/m_finishLoadOnFirstShow); // ... Populate it with content...
            if (e.attribute("type") == "text")
                m_shouldConvertPlainTextNotes = true; // Convert Pre-0.6.0 baskets: plain text notes should be converted to rich text ones once all is loaded!
            appendNoteIn(note, parent);               // ... And insert it.
            // Load dates:
            if (e.hasAttribute("added"))
                note->setAddedDate(QDateTime::fromString(e.attribute("added"), Qt::ISODate));
            if (e.hasAttribute("lastModification"))
                note->setLastModificationDate(QDateTime::fromString(e.attribute("lastModification"), Qt::ISODate));
        }
        // If we successfully loaded a note:
        if (note) {
            // Free Note Properties:
            if (note->isFree()) {
                int x = e.attribute("x").toInt();
                int y = e.attribute("y").toInt();
                note->setX(x < 0 ? 0 : x);
                note->setY(y < 0 ? 0 : y);
            }
            // Resizeable Note Properties:
            if (note->hasResizer() || note->isColumn())
                note->setGroupWidth(e.attribute("width", "200").toInt());
            // Group Properties:
            if (note->isGroup() && !note->isColumn() && XMLWork::trueOrFalse(e.attribute("folded", "false")))
                note->toggleFolded();
            // Tags:
            if (note->content()) {
                QString tagsString = XMLWork::getElementText(e, QStringLiteral("tags"), QString());
                QStringList tagsId = tagsString.split(';');
                for (QStringList::iterator it = tagsId.begin(); it != tagsId.end(); ++it) {
                    State *state = Tag::stateById(*it);
                    if (state)
                        note->addState(state, /*orReplace=*/true);
                }
            }
        }
        qApp->processEvents();
    }
}

void BasketScene::saveNotes(QXmlStreamWriter &stream, Note *parent)
{
    Note *note = (parent ? parent->firstChild() : firstNote());
    while (note) {
        // Create Element:
        stream.writeStartElement(note->isGroup() ? "group" : "note");
        // Free Note Properties:
        if (note->isFree()) {
            stream.writeAttribute("x", QString::number(note->x()));
            stream.writeAttribute("y", QString::number(note->y()));
        }
        // Resizeable Note Properties:
        if (note->hasResizer())
            stream.writeAttribute("width", QString::number(note->groupWidth()));
        // Group Properties:
        if (note->isGroup() && !note->isColumn())
            stream.writeAttribute("folded", XMLWork::trueOrFalse(note->isFolded()));
        // Save Content:
        if (note->content()) {
            // Save Dates:
            stream.writeAttribute("added", note->addedDate().toString(Qt::ISODate));
            stream.writeAttribute("lastModification", note->lastModificationDate().toString(Qt::ISODate));
            // Save Content:
            stream.writeAttribute("type", note->content()->lowerTypeName());
            note->content()->saveToNode(stream);
            // Save Tags:
            if (note->states().count() > 0) {
                QString tags;
                for (State::List::iterator it = note->states().begin(); it != note->states().end(); ++it) {
                    tags += (tags.isEmpty() ? QString() : QStringLiteral(";")) + (*it)->id();
                }
                stream.writeTextElement("tags", tags);
            }
        } else {
            // Save Child Notes:
            saveNotes(stream, note);
        }
        stream.writeEndElement();
        // Go to the Next One:
        note = note->next();
    }
}

void BasketScene::loadProperties(const QDomElement &properties)
{
    // Compute Default Values for When Loading the Properties:
    QString defaultBackgroundColor = (backgroundColorSetting().isValid() ? backgroundColorSetting().name() : QString());
    QString defaultTextColor = (textColorSetting().isValid() ? textColorSetting().name() : QString());

    // Load the Properties:
    QString icon = XMLWork::getElementText(properties, "icon", this->icon());
    QString name = XMLWork::getElementText(properties, "name", basketName());

    QDomElement appearance = XMLWork::getElement(properties, "appearance");
    // In 0.6.0-Alpha versions, there was a typo error: "backround" instead of "background"
    QString backgroundImage = appearance.attribute("backgroundImage", appearance.attribute("backroundImage", backgroundImageName()));
    QString backgroundColorString = appearance.attribute("backgroundColor", appearance.attribute("backroundColor", defaultBackgroundColor));
    QString textColorString = appearance.attribute("textColor", defaultTextColor);
    QColor backgroundColor = (backgroundColorString.isEmpty() ? QColor() : QColor(backgroundColorString));
    QColor textColor = (textColorString.isEmpty() ? QColor() : QColor(textColorString));

    QDomElement disposition = XMLWork::getElement(properties, "disposition");
    bool free = XMLWork::trueOrFalse(disposition.attribute("free", XMLWork::trueOrFalse(isFreeLayout())));
    int columnCount = disposition.attribute("columnCount", QString::number(this->columnsCount())).toInt();
    bool mindMap = XMLWork::trueOrFalse(disposition.attribute("mindMap", XMLWork::trueOrFalse(isMindMap())));

    QDomElement shortcut = XMLWork::getElement(properties, "shortcut");
    QString actionStrings[] = {"show", "globalShow", "globalSwitch"};
    QKeySequence combination = QKeySequence(shortcut.attribute("combination", m_action->shortcut().toString()));
    QString actionString = shortcut.attribute("action");
    int action = shortcutAction();
    if (actionString == actionStrings[0])
        action = 0;
    if (actionString == actionStrings[1])
        action = 1;
    if (actionString == actionStrings[2])
        action = 2;

    QDomElement protection = XMLWork::getElement(properties, "protection");
    m_encryptionType = protection.attribute("type").toInt();
    m_encryptionKey = protection.attribute("key");

    // Apply the Properties:
    setDisposition((free ? (mindMap ? 2 : 1) : 0), columnCount);
    setShortcut(combination, action);
    setAppearance(icon, name, backgroundImage, backgroundColor, textColor); // Will emit propertiesChanged(this)
}

void BasketScene::saveProperties(QXmlStreamWriter &stream)
{
    stream.writeStartElement("properties");

    stream.writeTextElement("name", basketName());
    stream.writeTextElement("icon", icon());

    stream.writeStartElement("appearance");
    stream.writeAttribute("backgroundColor", backgroundColorSetting().isValid() ? backgroundColorSetting().name() : QString());
    stream.writeAttribute("backgroundImage", backgroundImageName());
    stream.writeAttribute("textColor", textColorSetting().isValid() ? textColorSetting().name() : QString());
    stream.writeEndElement();

    stream.writeStartElement("disposition");
    stream.writeAttribute("columnCount", QString::number(columnsCount()));
    stream.writeAttribute("free", XMLWork::trueOrFalse(isFreeLayout()));
    stream.writeAttribute("mindMap", XMLWork::trueOrFalse(isMindMap()));
    stream.writeEndElement();

    stream.writeStartElement("shortcut");
    QString actionStrings[] = {"show", "globalShow", "globalSwitch"};
    stream.writeAttribute("action", actionStrings[shortcutAction()]);
    stream.writeAttribute("combination", m_action->shortcut().toString());
    stream.writeEndElement();

    stream.writeStartElement("protection");
    stream.writeAttribute("key", m_encryptionKey);
    stream.writeAttribute("type", QString::number(m_encryptionType));
    stream.writeEndElement();

    stream.writeEndElement();
}

void BasketScene::subscribeBackgroundImages()
{
    if (!m_backgroundImageName.isEmpty()) {
        Global::backgroundManager->subscribe(m_backgroundImageName);
        Global::backgroundManager->subscribe(m_backgroundImageName, this->backgroundColor());
        Global::backgroundManager->subscribe(m_backgroundImageName, selectionRectInsideColor());
        m_backgroundPixmap = Global::backgroundManager->pixmap(m_backgroundImageName);
        m_opaqueBackgroundPixmap = Global::backgroundManager->opaquePixmap(m_backgroundImageName, this->backgroundColor());
        m_selectedBackgroundPixmap = Global::backgroundManager->opaquePixmap(m_backgroundImageName, selectionRectInsideColor());
        m_backgroundTiled = Global::backgroundManager->tiled(m_backgroundImageName);
    }
}

void BasketScene::unsubscribeBackgroundImages()
{
    if (hasBackgroundImage()) {
        Global::backgroundManager->unsubscribe(m_backgroundImageName);
        Global::backgroundManager->unsubscribe(m_backgroundImageName, this->backgroundColor());
        Global::backgroundManager->unsubscribe(m_backgroundImageName, selectionRectInsideColor());
        m_backgroundPixmap = nullptr;
        m_opaqueBackgroundPixmap = nullptr;
        m_selectedBackgroundPixmap = nullptr;
    }
}

void BasketScene::setAppearance(const QString &icon, const QString &name, const QString &backgroundImage, const QColor &backgroundColor, const QColor &textColor)
{
    unsubscribeBackgroundImages();

    m_basketName = name;
    m_backgroundImageName = backgroundImage;
    m_backgroundColorSetting = backgroundColor;
    m_textColorSetting = textColor;

    // Where is this shown?
    m_action->setText("BASKET SHORTCUT: " + name);

    // Basket should ALWAYS have an icon (the "basket" icon by default):
    QPixmap iconTest = KIconLoader::global()->loadIcon(icon, KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, /*canReturnNull=*/true);
    if (!iconTest.isNull())
        m_icon = icon;

    // We don't request the background images if it's not loaded yet (to make the application startup fast).
    // When the basket is loading (because requested by the user: he/she want to access it)
    // it load the properties, subscribe to (and then load) the images, update the "Loading..." message with the image,
    // load all the notes and it's done!
    if (m_loadingLaunched)
        subscribeBackgroundImages();

    recomputeAllStyles();  // If a note have a tag with the same background color as the basket one, then display a "..."
    recomputeBlankRects(); // See the drawing of blank areas in BasketScene::drawContents()
    unbufferizeAll();

    if (isDuringEdit() && m_editor->graphicsWidget()) {
        QPalette palette;
        palette.setColor(m_editor->graphicsWidget()->widget()->backgroundRole(), m_editor->note()->backgroundColor());
        palette.setColor(m_editor->graphicsWidget()->widget()->foregroundRole(), m_editor->note()->textColor());
        m_editor->graphicsWidget()->setPalette(palette);
    }

    Q_EMIT propertiesChanged(this);
}

void BasketScene::setDisposition(int disposition, int columnCount)
{
    static const int COLUMNS_LAYOUT = 0;
    static const int FREE_LAYOUT = 1;
    static const int MINDMAPS_LAYOUT = 2;

    int currentDisposition = (isFreeLayout() ? (isMindMap() ? MINDMAPS_LAYOUT : FREE_LAYOUT) : COLUMNS_LAYOUT);

    if (currentDisposition == COLUMNS_LAYOUT && disposition == COLUMNS_LAYOUT) {
        if (firstNote() && columnCount > m_columnsCount) {
            // Insert each new columns:
            for (int i = m_columnsCount; i < columnCount; ++i) {
                Note *newColumn = new Note(this);
                insertNote(newColumn, /*clicked=*/lastNote(), /*zone=*/Note::BottomInsert);
            }
        } else if (firstNote() && columnCount < m_columnsCount) {
            Note *column = firstNote();
            Note *cuttedNotes = nullptr;
            for (int i = 1; i <= m_columnsCount; ++i) {
                Note *columnToRemove = column;
                column = column->next();
                if (i > columnCount) {
                    // Remove the columns that are too much:
                    unplugNote(columnToRemove);
                    // "Cut" the content in the columns to be deleted:
                    if (columnToRemove->firstChild()) {
                        for (Note *it = columnToRemove->firstChild(); it; it = it->next())
                            it->setParentNote(nullptr);
                        if (!cuttedNotes)
                            cuttedNotes = columnToRemove->firstChild();
                        else {
                            Note *lastCuttedNote = cuttedNotes;
                            while (lastCuttedNote->next())
                                lastCuttedNote = lastCuttedNote->next();
                            lastCuttedNote->setNext(columnToRemove->firstChild());
                            columnToRemove->firstChild()->setPrev(lastCuttedNote);
                        }
                        columnToRemove->setFirstChild(nullptr);
                    }
                    delete columnToRemove;
                }
            }
            // Paste the content in the last column:
            if (cuttedNotes)
                insertNote(cuttedNotes, /*clicked=*/lastNote(), /*zone=*/Note::BottomColumn);
            unselectAll();
        }
        if (columnCount != m_columnsCount) {
            m_columnsCount = (columnCount <= 0 ? 1 : columnCount);
            equalizeColumnSizes(); // Will relayoutNotes()
        }
    } else if (currentDisposition == COLUMNS_LAYOUT && (disposition == FREE_LAYOUT || disposition == MINDMAPS_LAYOUT)) {
        Note *column = firstNote();
        m_columnsCount = 0; // Now, so relayoutNotes() will not relayout the free notes as if they were columns!
        while (column) {
            // Move all childs on the first level:
            Note *nextColumn = column->next();
            ungroupNote(column);
            column = nextColumn;
        }
        unselectAll();
        m_mindMap = (disposition == MINDMAPS_LAYOUT);
        relayoutNotes();
    } else if ((currentDisposition == FREE_LAYOUT || currentDisposition == MINDMAPS_LAYOUT) && disposition == COLUMNS_LAYOUT) {
        if (firstNote()) {
            // TODO: Reorder notes!
            // Remove all notes (but keep a reference to them, we're not crazy ;-) ):
            Note *notes = m_firstNote;
            m_firstNote = nullptr;
            m_count = 0;
            m_countFounds = 0;
            // Insert the number of columns that is needed:
            Note *lastInsertedColumn = nullptr;
            for (int i = 0; i < columnCount; ++i) {
                Note *column = new Note(this);
                if (lastInsertedColumn)
                    insertNote(column, /*clicked=*/lastInsertedColumn, /*zone=*/Note::BottomInsert);
                else
                    m_firstNote = column;
                lastInsertedColumn = column;
            }
            // Reinsert the old notes in the first column:
            insertNote(notes, /*clicked=*/firstNote(), /*zone=*/Note::BottomColumn);
            unselectAll();
        } else {
            // Insert the number of columns that is needed:
            Note *lastInsertedColumn = nullptr;
            for (int i = 0; i < columnCount; ++i) {
                Note *column = new Note(this);
                if (lastInsertedColumn)
                    insertNote(column, /*clicked=*/lastInsertedColumn, /*zone=*/Note::BottomInsert);
                else
                    m_firstNote = column;
                lastInsertedColumn = column;
            }
        }
        m_columnsCount = (columnCount <= 0 ? 1 : columnCount);
        equalizeColumnSizes(); // Will relayoutNotes()
    }
}

void BasketScene::equalizeColumnSizes()
{
    if (!firstNote())
        return;

    // Necessary to know the available space;
    relayoutNotes();

    int availableSpace = m_view->viewport()->width();
    int columnWidth = (availableSpace - (columnsCount() - 1) * Note::GROUP_WIDTH) / columnsCount();
    int columnCount = columnsCount();
    Note *column = firstNote();
    while (column) {
        int minGroupWidth = column->minRight() - column->x();
        if (minGroupWidth > columnWidth) {
            availableSpace -= minGroupWidth;
            --columnCount;
        }
        column = column->next();
    }
    columnWidth = (availableSpace - (columnsCount() - 1) * Note::GROUP_WIDTH) / columnCount;

    column = firstNote();
    while (column) {
        int minGroupWidth = column->minRight() - column->x();
        if (minGroupWidth > columnWidth)
            column->setGroupWidth(minGroupWidth);
        else
            column->setGroupWidth(columnWidth);
        column = column->next();
    }

    relayoutNotes();
}

void BasketScene::enableActions()
{
    Global::bnpView->enableActions();
    m_view->setFocusPolicy(isLocked() ? Qt::NoFocus : Qt::StrongFocus);
    if (isLocked())
        m_view->viewport()->setCursor(Qt::ArrowCursor); // When locking, the cursor stays the last form it was
}

bool BasketScene::save()
{
    if (!m_loaded)
        return false;

    DEBUG_WIN << "Basket[" + folderName() + "]: Saving...";

    QString data;
    QXmlStreamWriter stream(&data);
    XMLWork::setupXmlStream(stream, "basket");

    // Create Properties Element and Populate It:
    saveProperties(stream);

    // Create Notes Element and Populate It:
    stream.writeStartElement("notes");
    saveNotes(stream, nullptr);
    stream.writeEndElement();

    stream.writeEndElement();
    stream.writeEndDocument();

    // Write to Disk:
    if (!FileStorage::saveToFile(fullPath() + ".basket", data, isEncrypted())) {
        DEBUG_WIN << "Basket[" + folderName() + "]: <font color=red>FAILED to save</font>!";
        return false;
    }

    Global::bnpView->setUnsavedStatus(false);

    m_commitdelay.start(10000); // delay is 10 seconds

    return true;
}

void BasketScene::commitEdit()
{
    GitWrapper::commitBasket(this);
}

void BasketScene::aboutToBeActivated()
{
    if (m_finishLoadOnFirstShow) {
        FOR_EACH_NOTE(note)
        note->finishLazyLoad();

        // relayoutNotes(/*animate=*/false);
        setFocusedNote(nullptr); // So that during the focusInEvent that will come shortly, the FIRST note is focused.

        m_finishLoadOnFirstShow = false;
        m_loaded = true;
    }
}

void BasketScene::reload()
{
    closeEditor();
    unbufferizeAll(); // Keep the memory footprint low

    m_firstNote = nullptr;

    m_loaded = false;
    m_loadingLaunched = false;

    invalidate();
}

void BasketScene::load()
{
    // Load only once:
    if (m_loadingLaunched)
        return;
    m_loadingLaunched = true;

    DEBUG_WIN << "Basket[" + folderName() + "]: Loading...";
    QDomDocument *doc = nullptr;
    QString content;

    // Load properties
    if (FileStorage::loadFromFile(fullPath() + ".basket", &content)) {
        doc = new QDomDocument("basket");
        if (!doc->setContent(content)) {
            DEBUG_WIN << "Basket[" + folderName() + "]: <font color=red>FAILED to parse XML</font>!";
            delete doc;
            doc = nullptr;
        }
    }
    if (isEncrypted())
        DEBUG_WIN << "Basket is encrypted.";
    if (!doc) {
        DEBUG_WIN << "Basket[" + folderName() + "]: <font color=red>FAILED to load</font>!";
        m_loadingLaunched = false;
        if (isEncrypted())
            m_locked = true;
        Global::bnpView->notesStateChanged(); // Show "Locked" instead of "Loading..." in the statusbar
        return;
    }
    m_locked = false;

    QDomElement docElem = doc->documentElement();
    QDomElement properties = XMLWork::getElement(docElem, "properties");

    loadProperties(properties); // Since we are loading, this time the background image will also be loaded!
    // Now that the background image is loaded and subscribed, we display it during the load process:
    delete doc;

    // BEGIN Compatibility with 0.6.0 Pre-Alpha versions:
    QDomElement notes = XMLWork::getElement(docElem, "notes");
    if (notes.isNull())
        notes = XMLWork::getElement(docElem, "items");
    m_watcher->stopScan();
    m_shouldConvertPlainTextNotes = false; // Convert Pre-0.6.0 baskets: plain text notes should be converted to rich text ones once all is loaded!

    // Load notes
    m_finishLoadOnFirstShow = (Global::bnpView->currentBasket() != this);
    loadNotes(notes, nullptr);
    if (m_shouldConvertPlainTextNotes)
        convertTexts();
    m_watcher->startScan();

    signalCountsChanged();
    if (isColumnsLayout()) {
        // Count the number of columns:
        int columnsCount = 0;
        Note *column = firstNote();
        while (column) {
            ++columnsCount;
            column = column->next();
        }
        m_columnsCount = columnsCount;
    }

    relayoutNotes();

    // On application start, the current basket is not focused yet, so the focus rectangle is not shown when calling focusANote():
    if (Global::bnpView->currentBasket() == this)
        setFocus();
    focusANote();

    m_loaded = true;
    enableActions();
}

void BasketScene::filterAgain(bool andEnsureVisible /* = true*/)
{
    newFilter(decoration()->filterData(), andEnsureVisible);
}

void BasketScene::filterAgainDelayed()
{
    QTimer::singleShot(0, this, SLOT(filterAgain()));
}

void BasketScene::newFilter(const FilterData &data, bool andEnsureVisible /* = true*/)
{
    if (!isLoaded())
        return;

    // StopWatch::start(20);

    m_countFounds = 0;
    //Search within basket titles as well
    if (data.tagFilterType == FilterData::DontCareTagsFilter)
        if (!data.string.isEmpty())
            if (basketName().contains(data.string, Qt::CaseInsensitive)){
                ++m_countFounds;
            }

    for (Note *note = firstNote(); note; note = note->next())
        m_countFounds += note->newFilter(data);

    relayoutNotes();
    signalCountsChanged();

    if (hasFocus())   // if (!hasFocus()), focusANote() will be called at focusInEvent()
        focusANote(); //  so, we avoid de-focus a note if it will be re-shown soon
    if (andEnsureVisible && m_focusedNote != nullptr)
        ensureNoteVisible(m_focusedNote);

    Global::bnpView->setFiltering(data.isFiltering);

    // StopWatch::check(20);
}

bool BasketScene::isFiltering()
{
    return decoration()->filterBar()->filterData().isFiltering;
}

QString BasketScene::fullPath()
{
    return Global::basketsFolder() + folderName();
}

QString BasketScene::fullPathForFileName(const QString &fileName)
{
    return fullPath() + fileName;
}

/*static*/ QString BasketScene::fullPathForFolderName(const QString &folderName)
{
    return Global::basketsFolder() + folderName;
}

void BasketScene::setShortcut(QKeySequence shortcut, int action)
{
    QList<QKeySequence> shortcuts {shortcut};
    m_action->setShortcuts(shortcuts);
    m_shortcutAction = action;

    if (!shortcut.isEmpty()) {
        if (action > 0) {
            KGlobalAccel::self()->setShortcut(m_action, shortcuts, KGlobalAccel::Autoloading);
            KGlobalAccel::self()->setDefaultShortcut(m_action, shortcuts, KGlobalAccel::Autoloading);
        }
    }
}

void BasketScene::activatedShortcut()
{
    Global::bnpView->setCurrentBasket(this);

    if (m_shortcutAction == 1)
        Global::bnpView->setActive(true);
}

void BasketScene::signalCountsChanged()
{
    if (!m_timerCountsChanged.isActive()) {
        m_timerCountsChanged.setSingleShot(true);
        m_timerCountsChanged.start(0);
    }
}

void BasketScene::countsChangedTimeOut()
{
    Q_EMIT countsChanged(this);
}

BasketScene::BasketScene(QWidget *parent, const QString &folderName)
    //: Q3ScrollView(parent)
    : QGraphicsScene(parent)
    , m_noActionOnMouseRelease(false)
    , m_ignoreCloseEditorOnNextMouseRelease(false)
    , m_pressPos(-100, -100)
    , m_canDrag(false)
    , m_firstNote(nullptr)
    , m_columnsCount(1)
    , m_mindMap(false)
    , m_resizingNote(nullptr)
    , m_pickedResizer(0)
    , m_movingNote(nullptr)
    , m_pickedHandle(0, 0)
    , m_notesToBeDeleted()
    , m_clickedToInsert(nullptr)
    , m_zoneToInsert(0)
    , m_posToInsert(-1, -1)
    , m_isInsertPopupMenu(false)
    , m_insertMenuTitle(nullptr)
    , m_loaded(false)
    , m_loadingLaunched(false)
    , m_locked(false)
    , m_decryptBox(nullptr)
    , m_button(nullptr)
    , m_encryptionType(NoEncryption)
#ifdef HAVE_LIBGPGME
    , m_gpg(0)
#endif
    , m_backgroundPixmap(nullptr)
    , m_opaqueBackgroundPixmap(nullptr)
    , m_selectedBackgroundPixmap(nullptr)
    , m_action(nullptr)
    , m_shortcutAction(0)
    , m_hoveredNote(nullptr)
    , m_hoveredZone(Note::None)
    , m_lockedHovering(false)
    , m_underMouse(false)
    , m_inserterRect()
    , m_inserterShown(false)
    , m_inserterSplit(true)
    , m_inserterTop(false)
    , m_inserterGroup(false)
    , m_lastDisableClick(QTime::currentTime())
    , m_isSelecting(false)
    , m_selectionStarted(false)
    , m_count(0)
    , m_countFounds(0)
    , m_countSelecteds(0)
    , m_icon("org.kde.basket")
    , m_folderName(folderName)
    , m_editor(nullptr)
    , m_redirectEditActions(false)
    , m_editorTrackMouseEvent(false)
    , m_editorWidth(-1)
    , m_editorHeight(-1)
    , m_doNotCloseEditor(false)
    , m_isDuringDrag(false)
    , m_draggedNotes()
    , m_focusedNote(nullptr)
    , m_startOfShiftSelectionNote(nullptr)
    , m_finishLoadOnFirstShow(false)
    , m_relayoutOnNextShow(false)
{
    m_view = new BasketView(this);
    m_view->setFocusPolicy(Qt::StrongFocus);
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // We do this in the basket properties dialog (and keep it in sync with the
    // global one)
    if (!m_folderName.endsWith('/'))
        m_folderName += '/';

    KActionCollection *ac = Global::bnpView->actionCollection();
    m_action = ac->addAction(m_folderName, this, SLOT(activatedShortcut()));
    ac->setShortcutsConfigurable(m_action, false);
    KGlobalAccel::setGlobalShortcut(m_action, (QKeySequence()));

    //    setDragAutoScroll(true);

    // By default, there is no corner widget: we set one for the corner area to be painted!
    // If we don't set one and there are two scrollbars present, slowly resizing up the window show graphical glitches in that area!
    m_cornerWidget = new QWidget(m_view);
    m_view->setCornerWidget(m_cornerWidget);

    m_view->viewport()->setAcceptDrops(true);
    m_view->viewport()->setMouseTracking(true);
    m_view->viewport()->setAutoFillBackground(false); // Do not clear the widget before paintEvent() because we always draw every pixels (faster and flicker-free)

    // File Watcher:
    m_watcher = new KDirWatch(this);

    connect(m_watcher, &KDirWatch::dirty, this, &BasketScene::watchedFileModified);
    //connect(m_watcher, &KDirWatch::deleted, this, &BasketScene::watchedFileDeleted);
    connect(&m_watcherTimer, &QTimer::timeout, this, &BasketScene::updateModifiedNotes);

    // Various Connections:
    connect(&m_autoScrollSelectionTimer, &QTimer::timeout, this, &BasketScene::doAutoScrollSelection);
    connect(&m_timerCountsChanged, &QTimer::timeout, this, &BasketScene::countsChangedTimeOut);
    connect(&m_inactivityAutoSaveTimer, &QTimer::timeout, this, &BasketScene::inactivityAutoSaveTimeout);
    connect(&m_inactivityAutoLockTimer, &QTimer::timeout, this, &BasketScene::inactivityAutoLockTimeout);

#ifdef HAVE_LIBGPGME
    m_gpg = new KGpgMe();
#endif
    m_locked = isFileEncrypted();

    // setup the delayed commit timer
    m_commitdelay.setSingleShot(true);
    connect(&m_commitdelay, &QTimer::timeout, this, &BasketScene::commitEdit);
}

void BasketScene::enterEvent(QEvent *)
{
    m_underMouse = true;
    doHoverEffects();
}

void BasketScene::leaveEvent(QEvent *)
{
    m_underMouse = false;
    doHoverEffects();

    if (m_lockedHovering)
        return;

    removeInserter();
    if (m_hoveredNote) {
        m_hoveredNote->setHovered(false);
        m_hoveredNote->setHoveredZone(Note::None);
        m_hoveredNote->update();
    }
    m_hoveredNote = nullptr;
}

void BasketScene::setFocusIfNotInPopupMenu()
{
    if (!qApp->activePopupWidget()) {
        if (isDuringEdit())
            m_editor->graphicsWidget()->setFocus();
        else
            setFocus();
    }
}

void BasketScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // If user click the basket, focus it!
    // The focus is delayed because if the click results in showing a popup menu,
    // the interface flicker by showing the focused rectangle (as the basket gets focus)
    // and immediately removing it (because the popup menu now have focus).
    if (!isDuringEdit())
        QTimer::singleShot(0, this, SLOT(setFocusIfNotInPopupMenu()));

    // Convenient variables:
    bool controlPressed = event->modifiers() & Qt::ControlModifier;
    bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

    // Do nothing if we disabled the click some milliseconds sooner.
    // For instance when a popup menu has been closed with click, we should not do action:
    if (event->button() == Qt::LeftButton && (qApp->activePopupWidget() || m_lastDisableClick.msecsTo(QTime::currentTime()) <= 80)) {
        doHoverEffects();
        m_noActionOnMouseRelease = true;
        // But we allow to select:
        // The code is the same as at the bottom of this method:
        if (event->button() == Qt::LeftButton) {
            m_selectionStarted = true;
            m_selectionBeginPoint = event->scenePos();
            m_selectionInvert = controlPressed || shiftPressed;
        }

        return;
    }

    // if we are editing and no control key are pressed
    if (m_editor && !shiftPressed && !controlPressed) {
        // if the mouse is over the editor
        QPoint view_shift(m_view->horizontalScrollBar()->value(), m_view->verticalScrollBar()->value());
        QGraphicsWidget *widget = dynamic_cast<QGraphicsWidget *>(m_view->itemAt((event->scenePos() - view_shift).toPoint()));
        if (widget && m_editor->graphicsWidget() == widget) {
            if (event->button() == Qt::LeftButton) {
                m_editorTrackMouseEvent = true;
                m_editor->startSelection(event->scenePos());
                return;
            } else if (event->button() == Qt::MiddleButton) {
                m_editor->paste(event->scenePos(), QClipboard::Selection);
                return;
            }
        }
    }

    // Figure out what is the clicked note and zone:
    Note *clicked = noteAt(event->scenePos());
    if (m_editor && (!clicked || (clicked && !(editedNote() == clicked)))) {
        closeEditor();
    }
    Note::Zone zone = (clicked ? clicked->zoneAt(event->scenePos() - QPointF(clicked->x(), clicked->y())) : Note::None);

    // Popup Tags menu:
    if (zone == Note::TagsArrow && !controlPressed && !shiftPressed && event->button() != Qt::MidButton) {
        if (!clicked->allSelected())
            unselectAllBut(clicked);
        setFocusedNote(clicked); /// /// ///
        m_startOfShiftSelectionNote = clicked;
        m_noActionOnMouseRelease = true;
        popupTagsMenu(clicked);

        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Prepare to allow drag and drop when moving mouse further:
        if ((zone == Note::Handle || zone == Note::Group) || (clicked && clicked->allSelected() && (zone == Note::TagsArrow || zone == Note::Custom0 || zone == Note::Content || zone == Note::Link /**/ || zone >= Note::Emblem0 /**/))) {
            if (!shiftPressed && !controlPressed) {
                m_pressPos = event->scenePos(); // TODO: Allow to drag emblems to assign them to other notes. Then don't allow drag at Emblem0!!

                m_canDrag = true;

                // Saving where we were editing, because during a drag, the mouse can fly over the text edit and move the cursor position:
                if (m_editor && m_editor->textEdit()) {
                    KTextEdit *editor = m_editor->textEdit();
                    m_textCursor = editor->textCursor();
                }
            }
        }

        // Initializing Resizer move:
        if (zone == Note::Resizer) {
            m_resizingNote = clicked;
            m_pickedResizer = event->scenePos().x() - clicked->rightLimit();
            m_noActionOnMouseRelease = true;
            m_lockedHovering = true;

            return;
        }

        // Select note(s):
        if (zone == Note::Handle || zone == Note::Group || (zone == Note::GroupExpander && (controlPressed || shiftPressed))) {
            // closeEditor();
            Note *end = clicked;
            if (clicked->isGroup() && shiftPressed) {
                if (clicked->containsNote(m_startOfShiftSelectionNote)) {
                    m_startOfShiftSelectionNote = clicked->firstRealChild();
                    end = clicked->lastRealChild();
                } else if (clicked->firstRealChild()->isAfter(m_startOfShiftSelectionNote)) {
                    end = clicked->lastRealChild();
                } else {
                    end = clicked->firstRealChild();
                }
            }
            if (controlPressed && shiftPressed)
                selectRange(m_startOfShiftSelectionNote, end, /*unselectOthers=*/false);
            else if (shiftPressed)
                selectRange(m_startOfShiftSelectionNote, end);
            else if (controlPressed)
                clicked->setSelectedRecursively(!clicked->allSelected());
            else if (!clicked->allSelected())
                unselectAllBut(clicked);
            setFocusedNote(end); /// /// ///
            m_startOfShiftSelectionNote = (end->isGroup() ? end->firstRealChild() : end);
            // m_noActionOnMouseRelease = false;
            m_noActionOnMouseRelease = true;

            return;
        }

        // Folding/Unfolding group:
        if (zone == Note::GroupExpander) {
            clicked->toggleFolded();

            relayoutNotes();
            m_noActionOnMouseRelease = true;

            return;
        }
    }

    // Popup menu for tag emblems:
    if (event->button() == Qt::RightButton && zone >= Note::Emblem0) {
        if (!clicked->allSelected())
            unselectAllBut(clicked);
        setFocusedNote(clicked); /// /// ///
        m_startOfShiftSelectionNote = clicked;
        popupEmblemMenu(clicked, zone - Note::Emblem0);
        m_noActionOnMouseRelease = true;

        return;
    }

    // Insertion Popup Menu:
    if ((event->button() == Qt::RightButton) && ((!clicked && isFreeLayout()) || (clicked && (zone == Note::TopInsert || zone == Note::TopGroup || zone == Note::BottomInsert || zone == Note::BottomGroup || zone == Note::BottomColumn)))) {
        unselectAll();
        m_clickedToInsert = clicked;
        m_zoneToInsert = zone;
        m_posToInsert = event->scenePos();

        QMenu menu(m_view);
        menu.addActions(Global::bnpView->popupMenu("insert_popup")->actions());

        // If we already added a title, remove it because it would be kept and
        // then added several times.
        if (m_insertMenuTitle && menu.actions().contains(m_insertMenuTitle))
            menu.removeAction(m_insertMenuTitle);

        QAction *first = menu.actions().value(0);

        // i18n: Verbs (for the "insert" menu)
        if (zone == Note::TopGroup || zone == Note::BottomGroup)
            m_insertMenuTitle = menu.insertSection(first, i18n("Group"));
        else
            m_insertMenuTitle = menu.insertSection(first, i18n("Insert"));

        setInsertPopupMenu();
        connect(&menu, &QMenu::aboutToHide, this, &BasketScene::delayedCancelInsertPopupMenu);
        connect(&menu, &QMenu::aboutToHide, this, &BasketScene::unlockHovering);
        connect(&menu, &QMenu::aboutToHide, this, &BasketScene::disableNextClick);
        connect(&menu, &QMenu::aboutToHide, this, &BasketScene::hideInsertPopupMenu);
        doHoverEffects(clicked, zone); // In the case where another popup menu was open, we should do that manually!
        m_lockedHovering = true;
        menu.exec(QCursor::pos());
        m_noActionOnMouseRelease = true;
        return;
    }

    // Note Context Menu:
    if (event->button() == Qt::RightButton && clicked && !clicked->isColumn() && zone != Note::Resizer) {
        if (!clicked->allSelected())
            unselectAllBut(clicked);
        setFocusedNote(clicked); /// /// ///
        if (editedNote() == clicked) {
            closeEditor(false);
            clicked->setSelected(true);
        }
        m_startOfShiftSelectionNote = (clicked->isGroup() ? clicked->firstRealChild() : clicked);
        QMenu *menu = Global::bnpView->popupMenu("note_popup");
        connect(menu, &QMenu::aboutToHide, this, &BasketScene::unlockHovering);
        connect(menu, &QMenu::aboutToHide, this, &BasketScene::disableNextClick);
        doHoverEffects(clicked, zone); // In the case where another popup menu was open, we should do that manually!
        m_lockedHovering = true;
        menu->exec(QCursor::pos());
        m_noActionOnMouseRelease = true;
        return;
    }

    // Paste selection under cursor (but not "create new primary note under cursor" because this is on moveRelease):
    if (event->button() == Qt::MidButton && zone != Note::Resizer && (!isDuringEdit() || clicked != editedNote())) {
        if ((Settings::middleAction() != 0) && (event->modifiers() == Qt::ShiftModifier)) {
            m_clickedToInsert = clicked;
            m_zoneToInsert = zone;
            m_posToInsert = event->scenePos();
            // closeEditor();
            removeInserter();                    // If clicked at an insertion line and the new note shows a dialog for editing,
            NoteType::Id type = (NoteType::Id)0; //  hide that inserter before the note edition instead of after the dialog is closed
            switch (Settings::middleAction()) {
            case 1:
                m_isInsertPopupMenu = true;
                pasteNote();
                break;
            case 2:
                type = NoteType::Image;
                break;
            case 3:
                type = NoteType::Link;
                break;
            case 4:
                type = NoteType::Launcher;
                break;
            default:
                m_noActionOnMouseRelease = false;
                return;
            }
            if (type != 0) {
                m_ignoreCloseEditorOnNextMouseRelease = true;
                Global::bnpView->insertEmpty(type);
            }
        } else {
            if (clicked)
                zone = clicked->zoneAt(event->scenePos() - QPoint(clicked->x(), clicked->y()), true);
            // closeEditor();
            clickedToInsert(event, clicked, zone);
            save();
        }
        m_noActionOnMouseRelease = true;
        return;
    }

    // Finally, no action has been done during pressEvent, so an action can be done on releaseEvent:
    m_noActionOnMouseRelease = false;

    /* Selection scenario:
     * On contentsMousePressEvent, put m_selectionStarted to true and init Begin and End selection point.
     * On contentsMouseMoveEvent, if m_selectionStarted, update End selection point, update selection rect,
     * and if it's larger, switching to m_isSelecting mode: we can draw the selection rectangle.
     */
    // Prepare selection:
    if (event->button() == Qt::LeftButton) {
        m_selectionStarted = true;
        m_selectionBeginPoint = event->scenePos();
        // We usually invert the selection with the Ctrl key, but some environments (like GNOME or The Gimp) do it with the Shift key.
        // Since the Shift key has no specific usage, we allow to invert selection ALSO with Shift for Gimp people
        m_selectionInvert = controlPressed || shiftPressed;
    }
}

void BasketScene::delayedCancelInsertPopupMenu()
{
    QTimer::singleShot(0, this, SLOT(cancelInsertPopupMenu()));
}

void BasketScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (event->reason() == QGraphicsSceneContextMenuEvent::Keyboard) {
        if (countFounds /*countShown*/ () == 0) { // TODO: Count shown!!
            QMenu *menu = Global::bnpView->popupMenu("insert_popup");
            setInsertPopupMenu();
            connect(menu, &QMenu::aboutToHide, this, &BasketScene::delayedCancelInsertPopupMenu);
            connect(menu, &QMenu::aboutToHide, this, &BasketScene::unlockHovering);
            connect(menu, &QMenu::aboutToHide, this, &BasketScene::disableNextClick);
            removeInserter();
            m_lockedHovering = true;
            menu->exec(m_view->mapToGlobal(QPoint(0, 0)));
        } else {
            if (!m_focusedNote->isSelected())
                unselectAllBut(m_focusedNote);
            setFocusedNote(m_focusedNote); /// /// ///
            m_startOfShiftSelectionNote = (m_focusedNote->isGroup() ? m_focusedNote->firstRealChild() : m_focusedNote);
            // Popup at bottom (or top) of the focused note, if visible :
            QMenu *menu = Global::bnpView->popupMenu("note_popup");
            connect(menu, &QMenu::aboutToHide, this, &BasketScene::unlockHovering);
            connect(menu, &QMenu::aboutToHide, this, &BasketScene::disableNextClick);
            doHoverEffects(m_focusedNote, Note::Content); // In the case where another popup menu was open, we should do that manually!
            m_lockedHovering = true;
            menu->exec(noteVisibleRect(m_focusedNote).bottomLeft().toPoint());
        }
    }
}

QRectF BasketScene::noteVisibleRect(Note *note)
{
    QRectF rect(QPointF(note->x(), note->y()), QSizeF(note->width(), note->height()));
    QPoint basketPoint = m_view->mapToGlobal(QPoint(0, 0));
    rect.moveTopLeft(rect.topLeft() + basketPoint + QPoint(m_view->frameWidth(), m_view->frameWidth()));

    // Now, rect contain the global note rectangle on the screen.
    // We have to clip it by the basket widget :
    //    if (rect.bottom() > basketPoint.y() + visibleHeight() + 1) { // Bottom too... bottom
    //        rect.setBottom(basketPoint.y() + visibleHeight() + 1);
    if (rect.bottom() > basketPoint.y() + m_view->viewport()->height() + 1) { // Bottom too... bottom
        rect.setBottom(basketPoint.y() + m_view->viewport()->height() + 1);
        if (rect.height() <= 0) // Have at least one visible pixel of height
            rect.setTop(rect.bottom());
    }
    if (rect.top() < basketPoint.y() + m_view->frameWidth()) { // Top too... top
        rect.setTop(basketPoint.y() + m_view->frameWidth());
        if (rect.height() <= 0)
            rect.setBottom(rect.top());
    }
    //    if (rect.right() > basketPoint.x() + visibleWidth() + 1) { // Right too... right
    //        rect.setRight(basketPoint.x() + visibleWidth() + 1);
    if (rect.right() > basketPoint.x() + m_view->viewport()->width() + 1) { // Right too... right
        rect.setRight(basketPoint.x() + m_view->viewport()->width() + 1);
        if (rect.width() <= 0) // Have at least one visible pixel of width
            rect.setLeft(rect.right());
    }
    if (rect.left() < basketPoint.x() + m_view->frameWidth()) { // Left too... left
        rect.setLeft(basketPoint.x() + m_view->frameWidth());
        if (rect.width() <= 0)
            rect.setRight(rect.left());
    }
    return rect;
}

void BasketScene::disableNextClick()
{
    m_lastDisableClick = QTime::currentTime();
}

void BasketScene::recomputeAllStyles()
{
    FOR_EACH_NOTE(note)
    note->recomputeAllStyles();
}

void BasketScene::removedStates(const QList<State *> &deletedStates)
{
    bool modifiedBasket = false;

    FOR_EACH_NOTE(note)
    if (note->removedStates(deletedStates))
        modifiedBasket = true;

    if (modifiedBasket)
        save();
}

void BasketScene::insertNote(Note *note, Note *clicked, int zone, const QPointF &pos)
{
    if (!note) {
        qDebug() << "Wanted to insert NO note";
        return;
    }

    if (clicked && zone == Note::BottomColumn) {
        // When inserting at the bottom of a column, it's obvious the new note SHOULD inherit tags.
        // We ensure that by changing the insertion point after the last note of the column:
        Note *last = clicked->lastChild();
        if (last) {
            clicked = last;
            zone = Note::BottomInsert;
        }
    }

    /// Insertion at the bottom of a column:
    if (clicked && zone == Note::BottomColumn) {
        note->setWidth(clicked->rightLimit() - clicked->x());
        Note *lastChild = clicked->lastChild();
        for (Note *n = note; n; n = n->next()) {
            n->setXRecursively(clicked->x());
            n->setYRecursively((lastChild ? lastChild : clicked)->bottom() + 1);
        }
        appendNoteIn(note, clicked);

        /// Insertion relative to a note (top/bottom, insert/group):
    } else if (clicked) {
        note->setWidth(clicked->width());
        for (Note *n = note; n; n = n->next()) {
            if (zone == Note::TopGroup || zone == Note::BottomGroup) {
                n->setXRecursively(clicked->x() + Note::GROUP_WIDTH);
            } else {
                n->setXRecursively(clicked->x());
            }
            if (zone == Note::TopInsert || zone == Note::TopGroup) {
                n->setYRecursively(clicked->y());
            } else {
                n->setYRecursively(clicked->bottom() + 1);
            }
        }

        if (zone == Note::TopInsert) {
            appendNoteBefore(note, clicked);
        } else if (zone == Note::BottomInsert) {
            appendNoteAfter(note, clicked);
        } else if (zone == Note::TopGroup) {
            groupNoteBefore(note, clicked);
        } else if (zone == Note::BottomGroup) {
            groupNoteAfter(note, clicked);
        }

        /// Free insertion:
    } else if (isFreeLayout()) {
        // Group if note have siblings:
        if (note->next()) {
            Note *group = new Note(this);
            for (Note *n = note; n; n = n->next())
                n->setParentNote(group);
            group->setFirstChild(note);
            note = group;
        }
        // Insert at cursor position:
        const int initialWidth = 250;
        note->setWidth(note->isGroup() ? Note::GROUP_WIDTH : initialWidth);
        if (note->isGroup() && note->firstChild())
            note->setInitialHeight(note->firstChild()->height());
        // note->setGroupWidth(initialWidth);
        note->setXRecursively(pos.x());
        note->setYRecursively(pos.y());
        appendNoteAfter(note, lastNote());
    }

    relayoutNotes();
}

void BasketScene::clickedToInsert(QGraphicsSceneMouseEvent *event, Note *clicked, /*Note::Zone*/ int zone)
{
    Note *note;
    if (event->button() == Qt::MidButton)
        note = NoteFactory::dropNote(QApplication::clipboard()->mimeData(QClipboard::Selection), this);
    else
        note = NoteFactory::createNoteText(QString(), this);

    if (!note)
        return;

    insertNote(note, clicked, zone, QPointF(event->scenePos()));

    //  ensureNoteVisible(lastInsertedNote()); // TODO: in insertNote()

    if (event->button() != Qt::MidButton) {
        removeInserter(); // Case: user clicked below a column to insert, the note is inserted and doHoverEffects() put a new inserter below. We don't want it.
        closeEditor();
        noteEdit(note, /*justAdded=*/true);
    }
}

void BasketScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    m_isDuringDrag = true;
    Global::bnpView->updateStatusBarHint();
    if (NoteDrag::basketOf(event->mimeData()) == this) {
        m_draggedNotes = NoteDrag::notesOf(event);
        NoteDrag::saveNoteSelectionToList(selectedNotes());
    }
    event->accept();
}

void BasketScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    //  m_isDuringDrag = true;

    //  if (isLocked())
    //      return;

    //  FIXME: viewportToContents does NOT work !!!
    //  QPoint pos = viewportToContents(event->pos());
    //  QPoint pos( event->pos().x() + contentsX(), event->pos().y() + contentsY() );

    //  if (insertAtCursorPos())
    //      computeInsertPlace(pos);
    doHoverEffects(event->scenePos());

    //  showFrameInsertTo();
    if (isFreeLayout() || noteAt(event->scenePos())) // Cursor before rightLimit() or hovering the dragged source notes
        acceptDropEvent(event);
    else {
        event->setAccepted(false);
    }

    /*  Note *hoveredNote = noteAt(event->pos().x(), event->pos().y());
        if ( (isColumnsLayout() && !hoveredNote) || (draggedNotes().contains(hoveredNote)) ) {
            event->acceptAction(false);
            event->accept(false);
        } else
            acceptDropEvent(event);*/

    // A workaround since QScrollView::dragAutoScroll seem to have no effect :
    //  ensureVisible(event->pos().x() + contentsX(), event->pos().y() + contentsY(), 30, 30);
    //  QScrollView::dragMoveEvent(event);
}

void BasketScene::dragLeaveEvent(QGraphicsSceneDragDropEvent *)
{
    //  resetInsertTo();
    m_isDuringDrag = false;
    m_draggedNotes.clear();
    NoteDrag::selectedNotes.clear();
    m_noActionOnMouseRelease = true;
    Q_EMIT resetStatusBarText();
    doHoverEffects();
}

void BasketScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    QPointF pos = event->scenePos();
    qDebug() << "Drop Event at position " << pos.x() << ":" << pos.y();

    m_isDuringDrag = false;
    Q_EMIT resetStatusBarText();

    //  if (isLocked())
    //      return;

    // Do NOT check the bottom&right borders.
    // Because imagine someone drag&drop a big note from the top to the bottom of a big basket (with big vertical scrollbars),
    // the note is first removed, and relayoutNotes() compute the new height that is smaller
    // Then noteAt() is called for the mouse pointer position, because the basket is now smaller, the cursor is out of boundaries!!!
    // Should, of course, not return 0:
    Note *clicked = noteAt(pos);

    if (NoteFactory::movingNotesInTheSameBasket(event->mimeData(), this, event->dropAction()) && event->dropAction() == Qt::MoveAction) {
        m_doNotCloseEditor = true;
    }

    Note *note = NoteFactory::dropNote(event->mimeData(), this, true, event->dropAction(), dynamic_cast<Note *>(event->source()));

    if (note) {
        Note::Zone zone = (clicked ? clicked->zoneAt(pos - QPointF(clicked->x(), clicked->y()), /*toAdd=*/true) : Note::None);
        bool animateNewPosition = NoteFactory::movingNotesInTheSameBasket(event->mimeData(), this, event->dropAction());
        if (animateNewPosition) {
            FOR_EACH_NOTE(n)
            n->setOnTop(false);
            // FOR_EACH_NOTE_IN_CHUNK(note)
            for (Note *n = note; n; n = n->next())
                n->setOnTop(true);
        }

        insertNote(note, clicked, zone, pos);

        // If moved a note on bottom, contentsHeight has been diminished, then view scrolled up, and we should re-scroll the view down:
        ensureNoteVisible(note);

        //      if (event->button() != Qt::MidButton) {
        //          removeInserter(); // Case: user clicked below a column to insert, the note is inserted and doHoverEffects() put a new inserter below. We don't want it.
        //      }

        //      resetInsertTo();
        //      doHoverEffects(); called by insertNote()
        save();
    }

    m_draggedNotes.clear();
    NoteDrag::selectedNotes.clear();

    m_doNotCloseEditor = false;
    // When starting the drag, we saved where we were editing.
    // This is because during a drag, the mouse can fly over the text edit and move the cursor position, and even HIDE the cursor.
    // So we re-show the cursor, and re-position it at the right place:
    if (m_editor && m_editor->textEdit()) {
        KTextEdit *editor = m_editor->textEdit();
        editor->setTextCursor(m_textCursor);
    }
}

// handles dropping of a note to basket that is not shown
// (usually through its entry in the basket list)
void BasketScene::blindDrop(QGraphicsSceneDragDropEvent *event)
{
    if (!m_isInsertPopupMenu && redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->paste();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->paste();
    } else {
        if (!isLoaded()) {
            load();
        }
        closeEditor();
        unselectAll();
        Note *note = NoteFactory::dropNote(event->mimeData(), this, true, event->dropAction(), dynamic_cast<Note *>(event->source()));
        if (note) {
            insertCreatedNote(note);
            // unselectAllBut(note);
        }
    }
    save();
}

void BasketScene::blindDrop(const QMimeData *mimeData, Qt::DropAction dropAction, QObject *source)
{
    if (!m_isInsertPopupMenu && redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->paste();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->paste();
    } else {
        if (!isLoaded()) {
            load();
        }
        closeEditor();
        unselectAll();
        Note *note = NoteFactory::dropNote(mimeData, this, true, dropAction, dynamic_cast<Note *>(source));
        if (note) {
            insertCreatedNote(note);
            // unselectAllBut(note);
        }
    }
    save();
}

void BasketScene::insertEmptyNote(int type)
{
    if (!isLoaded())
        load();
    if (isDuringEdit())
        closeEditor();
    Note *note = NoteFactory::createEmptyNote((NoteType::Id)type, this);
    insertCreatedNote(note /*, / *edit=* /true*/);
    noteEdit(note, /*justAdded=*/true);
}

void BasketScene::insertWizard(int type)
{
    saveInsertionData();
    Note *note = nullptr;
    switch (type) {
    default:
    case 1:
        note = NoteFactory::importKMenuLauncher(this);
        break;
    case 2:
        note = NoteFactory::importIcon(this);
        break;
    case 3:
        note = NoteFactory::importFileContent(this);
        break;
    }
    if (!note)
        return;
    restoreInsertionData();
    insertCreatedNote(note);
    unselectAllBut(note);
    resetInsertionData();
}

void BasketScene::insertColor(const QColor &color)
{
    Note *note = NoteFactory::createNoteColor(color, this);
    restoreInsertionData();
    insertCreatedNote(note);
    unselectAllBut(note);
    resetInsertionData();
}

void BasketScene::insertImage(const QPixmap &image)
{
    Note *note = NoteFactory::createNoteImage(image, this);
    restoreInsertionData();
    insertCreatedNote(note);
    unselectAllBut(note);
    resetInsertionData();
}

void BasketScene::pasteNote(QClipboard::Mode mode)
{
    if (!m_isInsertPopupMenu && redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->paste();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->paste();
    } else {
        if (!isLoaded()) {
            load();
        }
        closeEditor();
        unselectAll();
        Note *note = NoteFactory::dropNote(QApplication::clipboard()->mimeData(mode), this);
        if (note) {
            insertCreatedNote(note);
            // unselectAllBut(note);
        }
    }
}

void BasketScene::insertCreatedNote(Note *note)
{
    // Get the insertion data if the user clicked inside the basket:
    Note *clicked = m_clickedToInsert;
    int zone = m_zoneToInsert;
    QPointF pos = m_posToInsert;

    // If it isn't the case, use the default position:
    if (!clicked && (pos.x() < 0 || pos.y() < 0)) {
        // Insert right after the focused note:
        focusANote();
        if (m_focusedNote) {
            clicked = m_focusedNote;
            zone = (m_focusedNote->isFree() ? Note::BottomGroup : Note::BottomInsert);
            pos = QPointF(m_focusedNote->x(), m_focusedNote->bottom());
            // Insert at the end of the last column:
        } else if (isColumnsLayout()) {
            Note *column = /*(Settings::newNotesPlace == 0 ?*/ firstNote() /*: lastNote())*/;
            /*if (Settings::newNotesPlace == 0 && column->firstChild()) { // On Top, if at least one child in the column
                clicked = column->firstChild();
                zone    = Note::TopInsert;
            } else { // On Bottom*/
            clicked = column;
            zone = Note::BottomColumn;
            /*}*/
            // Insert at free position:
        } else {
            pos = QPointF(0, 0);
        }
    }

    insertNote(note, clicked, zone, pos);
    //  ensureNoteVisible(lastInsertedNote());
    removeInserter(); // Case: user clicked below a column to insert, the note is inserted and doHoverEffects() put a new inserter below. We don't want it.
                      //  resetInsertTo();
    save();
}

void BasketScene::saveInsertionData()
{
    m_savedClickedToInsert = m_clickedToInsert;
    m_savedZoneToInsert = m_zoneToInsert;
    m_savedPosToInsert = m_posToInsert;
}

void BasketScene::restoreInsertionData()
{
    m_clickedToInsert = m_savedClickedToInsert;
    m_zoneToInsert = m_savedZoneToInsert;
    m_posToInsert = m_savedPosToInsert;
}

void BasketScene::resetInsertionData()
{
    m_clickedToInsert = nullptr;
    m_zoneToInsert = 0;
    m_posToInsert = QPoint(-1, -1);
}

void BasketScene::hideInsertPopupMenu()
{
    QTimer::singleShot(50 /*ms*/, this, SLOT(timeoutHideInsertPopupMenu()));
}

void BasketScene::timeoutHideInsertPopupMenu()
{
    resetInsertionData();
}

void BasketScene::acceptDropEvent(QGraphicsSceneDragDropEvent *event, bool preCond)
{
    // FIXME: Should not accept all actions! Or not all actions (link not supported?!)
    // event->acceptAction(preCond && 1);
    // event->accept(preCond);
    event->setAccepted(preCond);
}

void BasketScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // Now disallow drag and mouse redirection
    m_canDrag = false;

    if (m_editorTrackMouseEvent) {
        m_editorTrackMouseEvent = false;
        m_editor->endSelection(m_pressPos);
        return;
    }

    // Cancel Resizer move:
    if (m_resizingNote) {
        m_resizingNote = nullptr;
        m_pickedResizer = 0;
        m_lockedHovering = false;
        doHoverEffects();
        save();
    }

    // Cancel Note move:
    /*  if (m_movingNote) {
            m_movingNote   = 0;
            m_pickedHandle = QPoint(0, 0);
            m_lockedHovering = false;
            //doHoverEffects();
            save();
    }*/

    // Cancel Selection rectangle:
    if (m_isSelecting) {
        m_isSelecting = false;
        stopAutoScrollSelection();
        resetWasInLastSelectionRect();
        doHoverEffects();
        invalidate(m_selectionRect);
    }
    m_selectionStarted = false;

    Note *clicked = noteAt(event->scenePos());
    Note::Zone zone = (clicked ? clicked->zoneAt(event->scenePos() - QPointF(clicked->x(), clicked->y())) : Note::None);
    if ((zone == Note::Handle || zone == Note::Group) && editedNote() && editedNote() == clicked) {
        if (m_ignoreCloseEditorOnNextMouseRelease)
            m_ignoreCloseEditorOnNextMouseRelease = false;
        else {
            bool editedNoteStillThere = closeEditor();
            if (editedNoteStillThere)
                // clicked->setSelected(true);
                unselectAllBut(clicked);
        }
    }

    /*
        if (event->buttons() == 0 && (zone == Note::Group || zone == Note::Handle)) {
            closeEditor();
            unselectAllBut(clicked);
        }
    */

    // Do nothing if an action has already been made during mousePressEvent,
    // or if user made a selection and canceled it by regressing to a very small rectangle.
    if (m_noActionOnMouseRelease)
        return;

    // We immediately set it to true, to avoid actions set on mouseRelease if NO mousePress event has been triggered.
    // This is the case when a popup menu is shown, and user click to the basket area to close it:
    // the menu then receive the mousePress event and the basket area ONLY receive the mouseRelease event.
    // Obviously, nothing should be done in this case:
    m_noActionOnMouseRelease = true;

    if (event->button() == Qt::MidButton && zone != Note::Resizer && (!isDuringEdit() || clicked != editedNote())) {
        if ((Settings::middleAction() != 0) && (event->modifiers() == Qt::ShiftModifier)) {
            m_clickedToInsert = clicked;
            m_zoneToInsert = zone;
            m_posToInsert = event->scenePos();
            closeEditor();
            removeInserter();                    // If clicked at an insertion line and the new note shows a dialog for editing,
            NoteType::Id type = (NoteType::Id)0; //  hide that inserter before the note edition instead of after the dialog is closed
            switch (Settings::middleAction()) {
            case 5:
                type = NoteType::Color;
                break;
            case 6:
                Global::bnpView->grabScreenshot();
                return;
            case 7:
                Global::bnpView->slotColorFromScreen();
                return;
            case 8:
                Global::bnpView->insertWizard(3); // loadFromFile
                return;
            case 9:
                Global::bnpView->insertWizard(1); // importKMenuLauncher
                return;
            case 10:
                Global::bnpView->insertWizard(2); // importIcon
                return;
            }
            if (type != 0) {
                m_ignoreCloseEditorOnNextMouseRelease = true;
                Global::bnpView->insertEmpty(type);
                return;
            }
        }
    }

    //  Note *clicked = noteAt(event->pos().x(), event->pos().y());
    if (!clicked) {
        if (isFreeLayout() && event->button() == Qt::LeftButton) {
            clickedToInsert(event);
            save();
        }

        return;
    }
    //  Note::Zone zone = clicked->zoneAt( event->pos() - QPoint(clicked->x(), clicked->y()) );

    // Convenient variables:
    bool controlPressed = event->modifiers() & Qt::ControlModifier;
    bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

    if (clicked && zone != Note::None && zone != Note::BottomColumn && zone != Note::Resizer && (controlPressed || shiftPressed)) {
        if (controlPressed && shiftPressed)
            selectRange(m_startOfShiftSelectionNote, clicked, /*unselectOthers=*/false);
        else if (shiftPressed)
            selectRange(m_startOfShiftSelectionNote, clicked);
        else if (controlPressed)
            clicked->setSelectedRecursively(!clicked->allSelected());
        setFocusedNote(clicked); /// /// ///
        m_startOfShiftSelectionNote = (clicked->isGroup() ? clicked->firstRealChild() : clicked);
        m_noActionOnMouseRelease = true;

        return;
    }

    // Switch tag states:
    if (zone >= Note::Emblem0) {
        if (event->button() == Qt::LeftButton) {
            int icons = -1;
            for (State::List::iterator it = clicked->states().begin(); it != clicked->states().end(); ++it) {
                if (!(*it)->emblem().isEmpty())
                    icons++;
                if (icons == zone - Note::Emblem0) {
                    State *state = (*it)->nextState();
                    if (!state)
                        return;
                    it = clicked->states().insert(it, state);
                    ++it;
                    clicked->states().erase(it);
                    clicked->recomputeStyle();
                    clicked->unbufferize();
                    clicked->update();
                    updateEditorAppearance();
                    filterAgain();
                    save();
                    break;
                }
            }

            return;
        } /* else if (event->button() == Qt::RightButton) {
         popupEmblemMenu(clicked, zone - Note::Emblem0);
         return;
     }*/
    }

    // Insert note or past clipboard:
    QString link;
    if (event->button() == Qt::MidButton && zone == Note::Resizer) {
        return; // zone = clicked->zoneAt( event->pos() - QPoint(clicked->x(), clicked->y()), true );
    }
    if (event->button() == Qt::RightButton && (clicked->isColumn() || zone == Note::Resizer)) {
        return;
    }
    if (clicked->isGroup() && zone == Note::None) {
        return;
    }
    switch (zone) {
    case Note::Handle:
    case Note::Group:
        // We select note on mousePress if it was unselected or Ctrl is pressed.
        // But the user can want to drag select_s_ notes, so it the note is selected, we only select it alone on mouseRelease:
        if (event->buttons() == 0) {
            qDebug() << "EXEC";
            if (!(event->modifiers() & Qt::ControlModifier) && clicked->allSelected())
                unselectAllBut(clicked);
            if (zone == Note::Handle && isDuringEdit() && editedNote() == clicked) {
                closeEditor();
                clicked->setSelected(true);
            }
        }
        break;

    case Note::Custom0:
        // unselectAllBut(clicked);
        setFocusedNote(clicked);
        noteOpen(clicked);
        break;

    case Note::GroupExpander:
    case Note::TagsArrow:
        break;

    case Note::Link:
        link = clicked->linkAt(event->scenePos() - QPoint(clicked->x(), clicked->y()));
        if (!link.isEmpty()) {
            if (link == "basket-internal-remove-basket") {
                // TODO: ask confirmation: "Do you really want to delete the welcome baskets?\n You can re-add them at any time in the Help menu."
                Global::bnpView->doBasketDeletion(this);
            } else if (link == "basket-internal-import") {
                QMenu *menu = Global::bnpView->popupMenu("fileimport");
                menu->exec(event->screenPos());
            } else if (link.startsWith("basket://")) {
                Q_EMIT crossReference(link);
            } else {
                KRun *run = new KRun(QUrl::fromUserInput(link), m_view->window()); //  open the URL.
                run->setAutoDelete(true);
            }
            break;
        } // If there is no link, edit note content
        Q_FALLTHROUGH();
    case Note::Content: {
        if (m_editor && m_editor->note() == clicked && m_editor->graphicsWidget()) {
            m_editor->setCursorTo(event->scenePos());
        } else {
            closeEditor();
            unselectAllBut(clicked);
            noteEdit(clicked, /*justAdded=*/false, event->scenePos());
            QGraphicsScene::mouseReleaseEvent(event);
        }
        break;
    }
    case Note::TopInsert:
    case Note::TopGroup:
    case Note::BottomInsert:
    case Note::BottomGroup:
    case Note::BottomColumn:
        clickedToInsert(event, clicked, zone);
        save();
        break;

    case Note::None:
    default:
        KMessageBox::information(m_view->viewport(),
                                 i18n("This message should never appear. If it does, this program is buggy! "
                                      "Please report the bug to the developer."));
        break;
    }
}

void BasketScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Note *clicked = noteAt(event->scenePos());
    Note::Zone zone = (clicked ? clicked->zoneAt(event->scenePos() - QPointF(clicked->x(), clicked->y())) : Note::None);

    if (event->button() == Qt::LeftButton && (zone == Note::Group || zone == Note::Handle)) {
        doCopy(CopyToSelection);
        m_noActionOnMouseRelease = true;
    } else
        mousePressEvent(event);
}

void BasketScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // redirect this event to the editor if track mouse event is active
    if (m_editorTrackMouseEvent && (m_pressPos - event->scenePos()).manhattanLength() > QApplication::startDragDistance()) {
        m_editor->updateSelection(event->scenePos());
        return;
    }

    // Drag the notes:
    if (m_canDrag && (m_pressPos - event->scenePos()).manhattanLength() > QApplication::startDragDistance()) {
        m_canDrag = false;
        m_isSelecting = false; // Don't draw selection rectangle after drag!
        m_selectionStarted = false;

        NoteSelection *selection = selectedNotes();
        if (selection->firstStacked()) {
            QDrag *d = NoteDrag::dragObject(selection, /*cutting=*/false, /*source=*/m_view); // d will be deleted by QT
            /*bool shouldRemove = */ d->exec();
            //      delete selection;

            // Never delete because URL is dragged and the file must be available for the extern application
            //      if (shouldRemove && d->target() == 0) // If target is another application that request to remove the note
            //          Q_EMIT wantDelete(this);
        }
        return;
    }

    // Moving a Resizer:
    if (m_resizingNote) {
        qreal groupWidth = event->scenePos().x() - m_resizingNote->x() - m_pickedResizer;
        qreal minRight = m_resizingNote->minRight();
        //        int maxRight   = 100 * contentsWidth(); // A big enough value (+infinity) for free layouts.
        qreal maxRight = 100 * sceneRect().width(); // A big enough value (+infinity) for free layouts.
        Note *nextColumn = m_resizingNote->next();
        if (m_resizingNote->isColumn()) {
            if (nextColumn)
                maxRight = nextColumn->x() + nextColumn->rightLimit() - nextColumn->minRight() - Note::RESIZER_WIDTH;
            else
                //                maxRight = contentsWidth();
                maxRight = sceneRect().width();
        }
        if (groupWidth > maxRight - m_resizingNote->x())
            groupWidth = maxRight - m_resizingNote->x();
        if (groupWidth < minRight - m_resizingNote->x())
            groupWidth = minRight - m_resizingNote->x();
        qreal delta = groupWidth - m_resizingNote->groupWidth();
        m_resizingNote->setGroupWidth(groupWidth);
        // If resizing columns:
        if (m_resizingNote->isColumn()) {
            Note *column = m_resizingNote;
            if ((column = column->next())) {
                // Next columns should not have them X coordinate animated, because it would flicker:
                column->setXRecursively(column->x() + delta);
                // And the resizer should resize the TWO sibling columns, and not push the other columns on th right:
                column->setGroupWidth(column->groupWidth() - delta);
            }
        }
        relayoutNotes();
    }

    // Moving a Note:
    /*  if (m_movingNote) {
            int x = event->pos().x() - m_pickedHandle.x();
            int y = event->pos().y() - m_pickedHandle.y();
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            m_movingNote->setX(x);
            m_movingNote->setY(y);
            m_movingNote->relayoutAt(x, y, / *animate=* /false);
            relayoutNotes(true);
        }
    */

    // Dragging the selection rectangle:
    if (m_selectionStarted)
        doAutoScrollSelection();

    doHoverEffects(event->scenePos());
}

void BasketScene::doAutoScrollSelection()
{
    static const int AUTO_SCROLL_MARGIN = 50; // pixels
    static const int AUTO_SCROLL_DELAY = 100; // milliseconds

    QPoint pos = m_view->mapFromGlobal(QCursor::pos());
    // Do the selection:
    if (m_isSelecting)
        invalidate(m_selectionRect);

    m_selectionEndPoint = m_view->mapToScene(pos);
    m_selectionRect = QRectF(m_selectionBeginPoint, m_selectionEndPoint).normalized();
    if (m_selectionRect.left() < 0)
        m_selectionRect.setLeft(0);
    if (m_selectionRect.top() < 0)
        m_selectionRect.setTop(0);
    //    if (m_selectionRect.right() >= contentsWidth())    m_selectionRect.setRight(contentsWidth() - 1);
    //    if (m_selectionRect.bottom() >= contentsHeight())  m_selectionRect.setBottom(contentsHeight() - 1);
    if (m_selectionRect.right() >= sceneRect().width())
        m_selectionRect.setRight(sceneRect().width() - 1);
    if (m_selectionRect.bottom() >= sceneRect().height())
        m_selectionRect.setBottom(sceneRect().height() - 1);

    if ((m_selectionBeginPoint - m_selectionEndPoint).manhattanLength() > QApplication::startDragDistance()) {
        m_isSelecting = true;
        selectNotesIn(m_selectionRect, m_selectionInvert);
        invalidate(m_selectionRect);
        m_noActionOnMouseRelease = true;
    } else {
        // If the user was selecting but cancel by making the rectangle too small, cancel it really!!!
        if (m_isSelecting) {
            if (m_selectionInvert)
                selectNotesIn(QRectF(), m_selectionInvert);
            else
                unselectAllBut(nullptr); // TODO: unselectAll();
        }
        if (m_isSelecting)
            resetWasInLastSelectionRect();
        m_isSelecting = false;
        stopAutoScrollSelection();

        return;
    }

    // Do the auto-scrolling:
    // FIXME: It's still flickering

    //    QRectF insideRect(AUTO_SCROLL_MARGIN, AUTO_SCROLL_MARGIN, visibleWidth() - 2*AUTO_SCROLL_MARGIN, visibleHeight() - 2*AUTO_SCROLL_MARGIN);
    // QRectF insideRect(AUTO_SCROLL_MARGIN, AUTO_SCROLL_MARGIN, m_view->viewport()->width() - 2 * AUTO_SCROLL_MARGIN, m_view->viewport()->height() - 2 * AUTO_SCROLL_MARGIN);

    int dx = 0;
    int dy = 0;

    if (pos.y() < AUTO_SCROLL_MARGIN)
        dy = pos.y() - AUTO_SCROLL_MARGIN;
    else if (pos.y() > m_view->viewport()->height() - AUTO_SCROLL_MARGIN)
        dy = pos.y() - m_view->viewport()->height() + AUTO_SCROLL_MARGIN;
    //    else if (pos.y() > visibleHeight() - AUTO_SCROLL_MARGIN)
    //        dy = pos.y() - visibleHeight() + AUTO_SCROLL_MARGIN;

    if (pos.x() < AUTO_SCROLL_MARGIN)
        dx = pos.x() - AUTO_SCROLL_MARGIN;
    else if (pos.x() > m_view->viewport()->width() - AUTO_SCROLL_MARGIN)
        dx = pos.x() - m_view->viewport()->width() + AUTO_SCROLL_MARGIN;
    //    else if (pos.x() > visibleWidth() - AUTO_SCROLL_MARGIN)
    //        dx = pos.x() - visibleWidth() + AUTO_SCROLL_MARGIN;

    if (dx || dy) {
        qApp->sendPostedEvents(); // Do the repaints, because the scrolling will make the area to repaint to be wrong
                                  //        scrollBy(dx, dy);
        if (!m_autoScrollSelectionTimer.isActive())
            m_autoScrollSelectionTimer.start(AUTO_SCROLL_DELAY);
    } else
        stopAutoScrollSelection();
}

void BasketScene::stopAutoScrollSelection()
{
    m_autoScrollSelectionTimer.stop();
}

void BasketScene::resetWasInLastSelectionRect()
{
    Note *note = m_firstNote;
    while (note) {
        note->resetWasInLastSelectionRect();
        note = note->next();
    }
}

void BasketScene::selectAll()
{
    if (redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->selectAll();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->selectAll();
    } else {
        // First select all in the group, then in the parent group...
        Note *parent = (m_focusedNote ? m_focusedNote->parentNote() : nullptr);
        while (parent) {
            if (!parent->allSelected()) {
                parent->setSelectedRecursively(true);
                return;
            }
            parent = parent->parentNote();
        }
        // Then, select all:
        FOR_EACH_NOTE(note)
        note->setSelectedRecursively(true);
    }
}

void BasketScene::unselectAll()
{
    if (redirectEditActions()) {
        if (m_editor->textEdit()) {
            QTextCursor cursor = m_editor->textEdit()->textCursor();
            cursor.clearSelection();
            m_editor->textEdit()->setTextCursor(cursor);
            selectionChangedInEditor(); // THIS IS NOT EMITTED BY Qt!!!
        } else if (m_editor->lineEdit())
            m_editor->lineEdit()->deselect();
    } else {
        if (countSelecteds() > 0) // Optimization
            FOR_EACH_NOTE(note)
        note->setSelectedRecursively(false);
    }
}

void BasketScene::invertSelection()
{
    FOR_EACH_NOTE(note)
    note->invertSelectionRecursively();
}

void BasketScene::unselectAllBut(Note *toSelect)
{
    FOR_EACH_NOTE(note)
    note->unselectAllBut(toSelect);
}

void BasketScene::invertSelectionOf(Note *toSelect)
{
    FOR_EACH_NOTE(note)
    note->invertSelectionOf(toSelect);
}

void BasketScene::selectNotesIn(const QRectF &rect, bool invertSelection, bool unselectOthers /*= true*/)
{
    FOR_EACH_NOTE(note)
    note->selectIn(rect, invertSelection, unselectOthers);
}

void BasketScene::doHoverEffects()
{
    doHoverEffects(m_view->mapToScene(m_view->viewport()->mapFromGlobal(QCursor::pos())));
}

void BasketScene::doHoverEffects(Note *note, Note::Zone zone, const QPointF &pos)
{
    // Inform the old and new hovered note (if any):
    Note *oldHoveredNote = m_hoveredNote;
    if (note != m_hoveredNote) {
        if (m_hoveredNote) {
            m_hoveredNote->setHovered(false);
            m_hoveredNote->setHoveredZone(Note::None);
            m_hoveredNote->update();
        }
        m_hoveredNote = note;
        if (m_hoveredNote) {
            m_hoveredNote->setHovered(true);
        }
    }

    // If we are hovering a note, compute which zone is hovered and inform the note:
    if (m_hoveredNote) {
        if (zone != m_hoveredZone || oldHoveredNote != m_hoveredNote) {
            m_hoveredZone = zone;
            m_hoveredNote->setHoveredZone(zone);
            m_view->viewport()->setCursor(m_hoveredNote->cursorFromZone(zone));

            m_hoveredNote->update();
        }

        // If we are hovering an insert line zone, update this thing:
        if (zone == Note::TopInsert || zone == Note::TopGroup || zone == Note::BottomInsert || zone == Note::BottomGroup || zone == Note::BottomColumn) {
            placeInserter(m_hoveredNote, zone);
        } else {
            removeInserter();
        }
        // If we are hovering an embedded link in a rich text element, show the destination in the statusbar:
        if (zone == Note::Link)
            Q_EMIT setStatusBarText(m_hoveredNote->linkAt(pos - QPoint(m_hoveredNote->x(), m_hoveredNote->y())));
        else if (m_hoveredNote->content())
            Q_EMIT setStatusBarText(m_hoveredNote->content()->statusBarMessage(m_hoveredZone)); // resetStatusBarText();
        // If we aren't hovering a note, reset all:
    } else {
        if (isFreeLayout() && !isSelecting())
            m_view->viewport()->setCursor(Qt::CrossCursor);
        else
            m_view->viewport()->unsetCursor();
        m_hoveredZone = Note::None;
        removeInserter();
        Q_EMIT resetStatusBarText();
    }
}

void BasketScene::doHoverEffects(const QPointF &pos)
{
    //  if (isDuringEdit())
    //      viewport()->unsetCursor();

    // Do we have the right to do hover effects?
    if (!m_loaded || m_lockedHovering) {
        return;
    }

    // enterEvent() (mouse enter in the widget) set m_underMouse to true, and leaveEvent() make it false.
    // But some times the enterEvent() is not trigerred: eg. when dragging the scrollbar:
    // Ending the drag INSIDE the basket area will make NO hoverEffects() because m_underMouse is false.
    // User need to leave the area and re-enter it to get effects.
    // This hack solve that by dismissing the m_underMouse variable:

    // Don't do hover effects when a popup menu is opened.
    // Primarily because the basket area will only receive mouseEnterEvent and mouveLeaveEvent.
    // It willn't be noticed of mouseMoveEvent, which would result in a apparently broken application state:
    bool underMouse = !qApp->activePopupWidget();
    // if (qApp->activePopupWidget())
    //    underMouse = false;

    // Compute which note is hovered:
    Note *note = (m_isSelecting || !underMouse ? nullptr : noteAt(pos));
    Note::Zone zone = (note ? note->zoneAt(pos - QPointF(note->x(), note->y()), isDuringDrag()) : Note::None);

    // Inform the old and new hovered note (if any) and update the areas:
    doHoverEffects(note, zone, pos);
}

void BasketScene::mouseEnteredEditorWidget()
{
    if (!m_lockedHovering && !qApp->activePopupWidget())
        doHoverEffects(editedNote(), Note::Content, QPoint());
}

void BasketScene::removeInserter()
{
    if (m_inserterShown) { // Do not hide (and then update/repaint the view) if it is already hidden!
        m_inserterShown = false;
        invalidate(m_inserterRect);
    }
}

void BasketScene::placeInserter(Note *note, int zone)
{
    // Remove the inserter:
    if (!note) {
        removeInserter();
        return;
    }

    // Update the old position:
    if (inserterShown()) {
        invalidate(m_inserterRect);
    }
    // Some commodities:
    m_inserterShown = true;
    m_inserterTop = (zone == Note::TopGroup || zone == Note::TopInsert);
    m_inserterGroup = (zone == Note::TopGroup || zone == Note::BottomGroup);
    // X and width:
    qreal groupIndent = (note->isGroup() ? note->width() : Note::HANDLE_WIDTH);
    qreal x = note->x();
    qreal width = (note->isGroup() ? note->rightLimit() - note->x() : note->width());
    if (m_inserterGroup) {
        x += groupIndent;
        width -= groupIndent;
    }
    m_inserterSplit = (Settings::groupOnInsertionLine() && note && !note->isGroup() && !note->isFree() && !note->isColumn());
    //  if (note->isGroup())
    //      width = note->rightLimit() - note->x() - (m_inserterGroup ? groupIndent : 0);
    // Y:
    qreal y = note->y() - (m_inserterGroup && m_inserterTop ? 1 : 3);
    if (!m_inserterTop)
        y += (note->isColumn() ? note->height() : note->height());
    // Assigning result:
    m_inserterRect = QRectF(x, y, width, 6 - (m_inserterGroup ? 2 : 0));
    // Update the new position:
    invalidate(m_inserterRect);
}

inline void drawLineByRect(QPainter &painter, qreal x, qreal y, qreal width, qreal height)
{
    painter.drawLine(x, y, x + width - 1, y + height - 1);
}

void BasketScene::drawInserter(QPainter &painter, qreal xPainter, qreal yPainter)
{
    if (!m_inserterShown)
        return;

    QRectF rect = m_inserterRect; // For shorter code-lines when drawing!
    rect.translate(-xPainter, -yPainter);
    int lineY = (m_inserterGroup && m_inserterTop ? 0 : 2);
    int roundY = (m_inserterGroup && m_inserterTop ? 0 : 1);

    KStatefulBrush statefulBrush(KColorScheme::View, KColorScheme::HoverColor);
    const QColor highlightColor = palette().color(QPalette::Highlight).lighter();
    painter.setPen(highlightColor);
    // The horizontal line:
    // painter.drawRect(       rect.x(),                    rect.y() + lineY,  rect.width(), 2);
    int width = rect.width() - 4;
    painter.fillRect(rect.x() + 2, rect.y() + lineY, width, 2, highlightColor);
    // The left-most and right-most edges (biggest vertical lines):
    drawLineByRect(painter, rect.x(), rect.y(), 1, (m_inserterGroup ? 4 : 6));
    drawLineByRect(painter, rect.x() + rect.width() - 1, rect.y(), 1, (m_inserterGroup ? 4 : 6));
    // The left and right mid vertical lines:
    drawLineByRect(painter, rect.x() + 1, rect.y() + roundY, 1, (m_inserterGroup ? 3 : 4));
    drawLineByRect(painter, rect.x() + rect.width() - 2, rect.y() + roundY, 1, (m_inserterGroup ? 3 : 4));
    // Draw the split as a feedback to know where is the limit between insert and group:
    if (m_inserterSplit) {
        int noteWidth = rect.width() + (m_inserterGroup ? Note::HANDLE_WIDTH : 0);
        int xSplit = rect.x() - (m_inserterGroup ? Note::HANDLE_WIDTH : 0) + noteWidth / 2;
        painter.drawRect(xSplit - 2, rect.y() + lineY, 4, 2);
        painter.drawRect(xSplit - 1, rect.y() + lineY, 2, 2);
    }
}

void BasketScene::helpEvent(QGraphicsSceneHelpEvent *event)
{
    if (!m_loaded || !Settings::showNotesToolTip())
        return;

    QString message;
    QRectF rect;

    QPointF contentPos = event->scenePos();
    Note *note = noteAt(contentPos);

    if (!note && isFreeLayout()) {
        message = i18n("Insert note here\nRight click for more options");
        QRectF itRect;
        for (QList<QRectF>::iterator it = m_blankAreas.begin(); it != m_blankAreas.end(); ++it) {
            itRect = QRectF(0, 0, m_view->viewport()->width(), m_view->viewport()->height()).intersected(*it);
            if (itRect.contains(contentPos)) {
                rect = itRect;
                rect.moveLeft(rect.left() - sceneRect().x());
                rect.moveTop(rect.top() - sceneRect().y());
                break;
            }
        }
    } else {
        if (!note)
            return;

        Note::Zone zone = note->zoneAt(contentPos - QPointF(note->x(), note->y()));

        switch (zone) {
        case Note::Resizer:
            message = (note->isColumn() ? i18n("Resize those columns") : (note->isGroup() ? i18n("Resize this group") : i18n("Resize this note")));
            break;
        case Note::Handle:
            message = i18n("Select or move this note");
            break;
        case Note::Group:
            message = i18n("Select or move this group");
            break;
        case Note::TagsArrow:
            message = i18n("Assign or remove tags from this note");
            if (note->states().count() > 0) {
                QString tagsString;
                for (State::List::iterator it = note->states().begin(); it != note->states().end(); ++it) {
                    QString tagName = "<nobr>" + Tools::textToHTMLWithoutP((*it)->fullName()) + "</nobr>";
                    if (tagsString.isEmpty())
                        tagsString = tagName;
                    else
                        tagsString = i18n("%1, %2", tagsString, tagName);
                }
                message = "<qt><nobr>" + message + "</nobr><br>" + i18n("<b>Assigned Tags</b>: %1", tagsString);
            }
            break;
        case Note::Custom0:
            message = note->content()->zoneTip(zone);
            break; //"Open this link/Open this file/Open this sound file/Launch this application"
        case Note::GroupExpander:
            message = (note->isFolded() ? i18n("Expand this group") : i18n("Collapse this group"));
            break;
        case Note::Link:
        case Note::Content:
            message = note->content()->editToolTipText();
            break;
        case Note::TopInsert:
        case Note::BottomInsert:
            message = i18n("Insert note here\nRight click for more options");
            break;
        case Note::TopGroup:
            message = i18n("Group note with the one below\nRight click for more options");
            break;
        case Note::BottomGroup:
            message = i18n("Group note with the one above\nRight click for more options");
            break;
        case Note::BottomColumn:
            message = i18n("Insert note here\nRight click for more options");
            break;
        case Note::None:
            message = "** Zone NONE: internal error **";
            break;
        default:
            if (zone >= Note::Emblem0)
                message = note->stateForEmblemNumber(zone - Note::Emblem0)->fullName();
            else
                message = QString();
            break;
        }

        if (zone == Note::Content || zone == Note::Link || zone == Note::Custom0) {
            const auto toolTip = QMultiMap<QString, QString> {
                { i18n("Added"), note->addedStringDate() },
                { i18n("Last Modification"), note->lastModificationStringDate() }
            }.unite(note->content()->toolTipInfos());

            message = "<qt><nobr>" + message;
            for (const QString &key : toolTip.keys()) {
                message += "<br>" + i18nc("of the form 'key: value'", "<b>%1</b>: %2", key, toolTip.value(key));
            }
            message += "</nobr></qt>";
        } else if (m_inserterSplit && (zone == Note::TopInsert || zone == Note::BottomInsert))
            message += '\n' + i18n("Click on the right to group instead of insert");
        else if (m_inserterSplit && (zone == Note::TopGroup || zone == Note::BottomGroup))
            message += '\n' + i18n("Click on the left to insert instead of group");

        rect = note->zoneRect(zone, contentPos - QPoint(note->x(), note->y()));

        rect.moveLeft(rect.left() - sceneRect().x());
        rect.moveTop(rect.top() - sceneRect().y());

        rect.moveLeft(rect.left() + note->x());
        rect.moveTop(rect.top() + note->y());
    }

    QToolTip::showText(event->screenPos(), message, m_view, rect.toRect());
}

Note *BasketScene::lastNote()
{
    Note *note = firstNote();
    while (note && note->next())
        note = note->next();
    return note;
}

void BasketScene::deleteNotes()
{
    Note *note = m_firstNote;

    while (note) {
        Note *tmp = note->next();
        delete note;
        note = tmp;
    }
    m_firstNote = nullptr;
    m_resizingNote = nullptr;
    m_movingNote = nullptr;
    m_focusedNote = nullptr;
    m_startOfShiftSelectionNote = nullptr;
    m_tagPopupNote = nullptr;
    m_clickedToInsert = nullptr;
    m_savedClickedToInsert = nullptr;
    m_hoveredNote = nullptr;
    m_count = 0;
    m_countFounds = 0;
    m_countSelecteds = 0;

    Q_EMIT resetStatusBarText();
    Q_EMIT countsChanged(this);
}

Note *BasketScene::noteAt(QPointF pos)
{
    qreal x = pos.x();
    qreal y = pos.y();
    // NO:
    //  // Do NOT check the bottom&right borders.
    //  // Because imagine someone drag&drop a big note from the top to the bottom of a big basket (with big vertical scrollbars),
    //  // the note is first removed, and relayoutNotes() compute the new height that is smaller
    //  // Then noteAt() is called for the mouse pointer position, because the basket is now smaller, the cursor is out of boundaries!!!
    //  // Should, of course, not return 0:
    if (x < 0 || x > sceneRect().width() || y < 0 || y > sceneRect().height())
        return nullptr;

    // When resizing a note/group, keep it highlighted:
    if (m_resizingNote)
        return m_resizingNote;

    // Search and return the hovered note:
    Note *note = m_firstNote;
    Note *possibleNote;
    while (note) {
        possibleNote = note->noteAt(pos);
        if (possibleNote) {
            if (NoteDrag::selectedNotes.contains(possibleNote) || draggedNotes().contains(possibleNote))
                return nullptr;
            else
                return possibleNote;
        }
        note = note->next();
    }

    // If the basket is layouted in columns, return one of the columns to be able to add notes in them:
    if (isColumnsLayout()) {
        Note *column = m_firstNote;
        while (column) {
            if (x >= column->x() && x < column->rightLimit())
                return column;
            column = column->next();
        }
    }

    // Nothing found, no note is hovered:
    return nullptr;
}

BasketScene::~BasketScene()
{
    m_commitdelay.stop(); // we don't know how long deleteNotes() last so we want to make extra sure that nobody will commit in between
    if (m_decryptBox)
        delete m_decryptBox;
#ifdef HAVE_LIBGPGME
    delete m_gpg;
#endif
    blockSignals(true);
    deleteNotes();

    if (m_view)
        delete m_view;
}

QColor BasketScene::selectionRectInsideColor()
{
    return Tools::mixColor(Tools::mixColor(backgroundColor(), palette().color(QPalette::Highlight)), backgroundColor());
}

QColor alphaBlendColors(const QColor &bgColor, const QColor &fgColor, const int a)
{
    // normal button...
    QRgb rgb = bgColor.rgb();
    QRgb rgb_b = fgColor.rgb();
    int alpha = a;
    if (alpha > 255)
        alpha = 255;
    if (alpha < 0)
        alpha = 0;
    int inv_alpha = 255 - alpha;
    QColor result = QColor(qRgb(qRed(rgb_b) * inv_alpha / 255 + qRed(rgb) * alpha / 255, qGreen(rgb_b) * inv_alpha / 255 + qGreen(rgb) * alpha / 255, qBlue(rgb_b) * inv_alpha / 255 + qBlue(rgb) * alpha / 255));

    return result;
}

void BasketScene::unlock()
{
    QTimer::singleShot(0, this, SLOT(load()));
}

void BasketScene::inactivityAutoLockTimeout()
{
    lock();
}

void BasketScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (!m_loadingLaunched) {
        if (!m_locked) {
            QTimer::singleShot(0, this, SLOT(load()));
            return;
        } else {
            Global::bnpView->notesStateChanged(); // Show "Locked" instead of "Loading..." in the statusbar
        }
    }

    if (!hasBackgroundImage()) {
        painter->fillRect(rect, backgroundColor());
        // It's either a background pixmap to draw or a background color to fill:
    } else if (isTiledBackground() || (rect.x() < backgroundPixmap()->width() && rect.y() < backgroundPixmap()->height())) {
        painter->fillRect(rect, backgroundColor());
        blendBackground(*painter, rect, 0, 0, /*opaque=*/true);
    } else {
        painter->fillRect(rect, backgroundColor());
    }
}

void BasketScene::drawForeground(QPainter *painter, const QRectF &rect)
{
    if (m_locked) {
        if (!m_decryptBox) {
            m_decryptBox = new QFrame(m_view);
            m_decryptBox->setFrameShape(QFrame::StyledPanel);
            m_decryptBox->setFrameShadow(QFrame::Plain);
            m_decryptBox->setLineWidth(1);

            QGridLayout *layout = new QGridLayout(m_decryptBox);
            layout->setContentsMargins(11, 11, 11, 11);
            layout->setSpacing(6);

#ifdef HAVE_LIBGPGME
            m_button = new QPushButton(m_decryptBox);
            m_button->setText(i18n("&Unlock"));
            layout->addWidget(m_button, 1, 2);
            connect(m_button, &QButton::clicked, this, &BasketScene::unlock);
#endif
            QLabel *label = new QLabel(m_decryptBox);
            QString text = "<b>" + i18n("Password protected basket.") + "</b><br/>";
#ifdef HAVE_LIBGPGME
            label->setText(text + i18n("Press Unlock to access it."));
#else
            label->setText(text + i18n("Encryption is not supported by<br/>this version of %1.", QGuiApplication::applicationDisplayName()));
#endif
            label->setAlignment(Qt::AlignTop);
            layout->addWidget(label, 0, 1, 1, 2);
            QLabel *pixmap = new QLabel(m_decryptBox);
            pixmap->setPixmap(KIconLoader::global()->loadIcon("encrypted", KIconLoader::NoGroup, KIconLoader::SizeHuge));
            layout->addWidget(pixmap, 0, 0, 2, 1);

            QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
            layout->addItem(spacer, 1, 1);

            label = new QLabel("<small>" +
                                   i18n("To make baskets stay unlocked, change the automatic<br>"
                                        "locking duration in the application settings.") +
                                   "</small>",
                               m_decryptBox);
            label->setAlignment(Qt::AlignTop);
            layout->addWidget(label, 2, 0, 1, 3);

            m_decryptBox->resize(layout->sizeHint());
        }
        if (m_decryptBox->isHidden()) {
            m_decryptBox->show();
        }
#ifdef HAVE_LIBGPGME
        m_button->setFocus();
#endif
        m_decryptBox->move((m_view->viewport()->width() - m_decryptBox->width()) / 2, (m_view->viewport()->height() - m_decryptBox->height()) / 2);
    } else {
        if (m_decryptBox && !m_decryptBox->isHidden())
            m_decryptBox->hide();
    }

    if (!m_loaded) {
        setSceneRect(0, 0, m_view->viewport()->width(), m_view->viewport()->height());
        QBrush brush(backgroundColor());
        QPixmap pixmap(m_view->viewport()->width(), m_view->viewport()->height()); // TODO: Clip it to asked size only!
        QPainter painter2(&pixmap);
        QTextDocument rt;
        rt.setHtml(QString("<center>%1</center>").arg(i18n("Loading...")));
        rt.setTextWidth(m_view->viewport()->width());
        int hrt = rt.size().height();
        painter2.fillRect(0, 0, m_view->viewport()->width(), m_view->viewport()->height(), brush);
        blendBackground(painter2, QRectF(0, 0, m_view->viewport()->width(), m_view->viewport()->height()), -1, -1, /*opaque=*/true);
        QPalette pal = palette();
        pal.setColor(QPalette::WindowText, textColor());
        painter2.translate(0, (m_view->viewport()->height() - hrt) / 2);
        QAbstractTextDocumentLayout::PaintContext context;
        context.palette = pal;
        rt.documentLayout()->draw(&painter2, context);
        painter2.end();
        painter->drawPixmap(0, 0, pixmap);
        return; // TODO: Clip to the wanted rectangle
    }

    enableActions();

    if ((inserterShown() && rect.intersects(inserterRect())) || (m_isSelecting && rect.intersects(m_selectionRect))) {
        // Draw inserter:
        if (inserterShown() && rect.intersects(inserterRect())) {
            drawInserter(*painter, 0, 0);
        }
        // Draw selection rect:
        if (m_isSelecting && rect.intersects(m_selectionRect)) {
            QRectF selectionRect = m_selectionRect;
            QRectF selectionRectInside(selectionRect.x() + 1, selectionRect.y() + 1, selectionRect.width() - 2, selectionRect.height() - 2);
            if (selectionRectInside.width() > 0 && selectionRectInside.height() > 0) {
                QColor insideColor = selectionRectInsideColor();
                painter->fillRect(selectionRectInside, QBrush(insideColor, Qt::Dense4Pattern));
            }
            painter->setPen(palette().color(QPalette::Highlight).darker());
            painter->drawRect(selectionRect);
            painter->setPen(Tools::mixColor(palette().color(QPalette::Highlight).darker(), backgroundColor()));
            painter->drawPoint(selectionRect.topLeft());
            painter->drawPoint(selectionRect.topRight());
            painter->drawPoint(selectionRect.bottomLeft());
            painter->drawPoint(selectionRect.bottomRight());
        }
    }
}

/*  rect(x,y,width,height)==(xBackgroundToDraw,yBackgroundToDraw,widthToDraw,heightToDraw)
 */
void BasketScene::blendBackground(QPainter &painter, const QRectF &rect, qreal xPainter, qreal yPainter, bool opaque, QPixmap *bg)
{
    painter.save();
    if (xPainter == -1 && yPainter == -1) {
        xPainter = rect.x();
        yPainter = rect.y();
    }

    if (hasBackgroundImage()) {
        const QPixmap *bgPixmap = (bg ? /* * */ bg : (opaque ? m_opaqueBackgroundPixmap : m_backgroundPixmap));
        if (isTiledBackground()) {
            painter.drawTiledPixmap(rect.x() - xPainter, rect.y() - yPainter, rect.width(), rect.height(), *bgPixmap, rect.x(), rect.y());
        } else {
            painter.drawPixmap(QPointF(rect.x() - xPainter, rect.y() - yPainter), *bgPixmap, rect);
        }
    }
    painter.restore();
}

void BasketScene::recomputeBlankRects()
{
    m_blankAreas.clear();
    return;

    m_blankAreas.append(QRectF(0, 0, sceneRect().width(), sceneRect().height()));

    FOR_EACH_NOTE(note)
    note->recomputeBlankRects(m_blankAreas);

    // See the drawing of blank areas in BasketScene::drawContents()
    if (hasBackgroundImage() && !isTiledBackground())
        substractRectOnAreas(QRectF(0, 0, backgroundPixmap()->width(), backgroundPixmap()->height()), m_blankAreas, false);
}

void BasketScene::unsetNotesWidth()
{
    Note *note = m_firstNote;
    while (note) {
        note->unsetWidth();
        note = note->next();
    }
}

void BasketScene::relayoutNotes()
{
    if (Global::bnpView->currentBasket() != this)
        return; // Optimize load time, and basket will be relaid out when activated, anyway

    int h = 0;
    tmpWidth = 0;
    tmpHeight = 0;
    Note *note = m_firstNote;
    while (note) {
        if (note->matching()) {
            note->relayoutAt(0, h);
            if (note->hasResizer()) {
                int minGroupWidth = note->minRight() - note->x();
                if (note->groupWidth() < minGroupWidth) {
                    note->setGroupWidth(minGroupWidth);
                    relayoutNotes(); // Redo the thing, but this time it should not recurse
                    return;
                }
            }
            h += note->height();
        }
        note = note->next();
    }

    if (isFreeLayout())
        tmpHeight += 100;
    else
        tmpHeight += 15;

    setSceneRect(0, 0, qMax((qreal)m_view->viewport()->width(), tmpWidth), qMax((qreal)m_view->viewport()->height(), tmpHeight));

    recomputeBlankRects();
    placeEditor();
    doHoverEffects();
    invalidate();
}

void BasketScene::popupEmblemMenu(Note *note, int emblemNumber)
{
    m_tagPopupNote = note;
    State *state = note->stateForEmblemNumber(emblemNumber);
    State *nextState = state->nextState(/*cycle=*/false);
    Tag *tag = state->parentTag();
    m_tagPopup = tag;

    QKeySequence sequence = tag->shortcut();
    bool sequenceOnDelete = (nextState == nullptr && !tag->shortcut().isEmpty());

    QMenu menu(m_view);
    if (tag->countStates() == 1) {
        menu.addSection(/*SmallIcon(state->icon()), */ tag->name());
        QAction *act;
        act = new QAction(QIcon::fromTheme("edit-delete"), i18n("&Remove"), &menu);
        act->setData(1);
        menu.addAction(act);
        act = new QAction(QIcon::fromTheme("configure"), i18n("&Customize..."), &menu);
        act->setData(2);
        menu.addAction(act);

        menu.addSeparator();

        act = new QAction(QIcon::fromTheme("search-filter"), i18n("&Filter by this Tag"), &menu);
        act->setData(3);
        menu.addAction(act);
    } else {
        menu.addSection(tag->name());
        QList<State *>::iterator it;
        State *currentState;

        int i = 10;
        // QActionGroup makes the actions exclusive; turns checkboxes into radio
        // buttons on some styles.
        QActionGroup *emblemGroup = new QActionGroup(&menu);
        for (it = tag->states().begin(); it != tag->states().end(); ++it) {
            currentState = *it;
            QKeySequence sequence;
            if (currentState == nextState && !tag->shortcut().isEmpty())
                sequence = tag->shortcut();

            StateAction *sa = new StateAction(currentState, QKeySequence(sequence), nullptr, false);
            sa->setChecked(state == currentState);
            sa->setActionGroup(emblemGroup);
            sa->setData(i);

            menu.addAction(sa);
            if (currentState == nextState && !tag->shortcut().isEmpty())
                sa->setShortcut(sequence);
            ++i;
        }

        menu.addSeparator();

        QAction *act = new QAction(&menu);
        act->setIcon(QIcon::fromTheme("edit-delete"));
        act->setText(i18n("&Remove"));
        act->setShortcut(sequenceOnDelete ? sequence : QKeySequence());
        act->setData(1);
        menu.addAction(act);
        act = new QAction(QIcon::fromTheme("configure"), i18n("&Customize..."), &menu);
        act->setData(2);
        menu.addAction(act);

        menu.addSeparator();

        act = new QAction(QIcon::fromTheme("search-filter"), i18n("&Filter by this Tag"), &menu);
        act->setData(3);
        menu.addAction(act);
        act = new QAction(QIcon::fromTheme("search-filter"), i18n("Filter by this &State"), &menu);
        act->setData(4);
        menu.addAction(act);
    }

    connect(&menu, &QMenu::triggered, this, &BasketScene::toggledStateInMenu);
    connect(&menu, &QMenu::aboutToHide, this, &BasketScene::unlockHovering);
    connect(&menu, &QMenu::aboutToHide, this, &BasketScene::disableNextClick);

    m_lockedHovering = true;
    menu.exec(QCursor::pos());
}

void BasketScene::toggledStateInMenu(QAction *action)
{
    int id = action->data().toInt();
    if (id == 1) {
        removeTagFromSelectedNotes(m_tagPopup);
        // m_tagPopupNote->removeTag(m_tagPopup);
        // m_tagPopupNote->setWidth(0); // To force a new layout computation
        updateEditorAppearance();
        filterAgain();
        save();
        return;
    }
    if (id == 2) { // Customize this State:
        TagsEditDialog dialog(m_view, m_tagPopupNote->stateOfTag(m_tagPopup));
        dialog.exec();
        return;
    }
    if (id == 3) { // Filter by this Tag
        decoration()->filterBar()->filterTag(m_tagPopup);
        return;
    }
    if (id == 4) { // Filter by this State
        decoration()->filterBar()->filterState(m_tagPopupNote->stateOfTag(m_tagPopup));
        return;
    }

    /*addStateToSelectedNotes*/ changeStateOfSelectedNotes(m_tagPopup->states()[id - 10] /*, orReplace=true*/);
    // m_tagPopupNote->addState(m_tagPopup->states()[id - 10], true);
    filterAgain();
    save();
}

State *BasketScene::stateForTagFromSelectedNotes(Tag *tag)
{
    State *state = nullptr;

    FOR_EACH_NOTE(note)
    if (note->stateForTagFromSelectedNotes(tag, &state) && state == nullptr)
        return nullptr;
    return state;
}

void BasketScene::activatedTagShortcut(Tag *tag)
{
    // Compute the next state to set:
    State *state = stateForTagFromSelectedNotes(tag);
    if (state)
        state = state->nextState(/*cycle=*/false);
    else
        state = tag->states().first();

    // Set or unset it:
    if (state) {
        FOR_EACH_NOTE(note)
        note->addStateToSelectedNotes(state, /*orReplace=*/true);
        updateEditorAppearance();
    } else
        removeTagFromSelectedNotes(tag);

    filterAgain();
    save();
}

void BasketScene::popupTagsMenu(Note *note)
{
    m_tagPopupNote = note;

    QMenu menu(m_view);
    menu.addSection(i18n("Tags"));

    Global::bnpView->populateTagsMenu(menu, note);

    m_lockedHovering = true;
    menu.exec(QCursor::pos());
}

void BasketScene::unlockHovering()
{
    m_lockedHovering = false;
    doHoverEffects();
}

void BasketScene::toggledTagInMenu(QAction *act)
{
    int id = act->data().toInt();
    if (id == 1) { // Assign new Tag...
        TagsEditDialog dialog(m_view, /*stateToEdit=*/nullptr, /*addNewTag=*/true);
        dialog.exec();
        if (!dialog.addedStates().isEmpty()) {
            State::List states = dialog.addedStates();
            for (State::List::iterator itState = states.begin(); itState != states.end(); ++itState)
                FOR_EACH_NOTE(note)
            note->addStateToSelectedNotes(*itState);
            updateEditorAppearance();
            filterAgain();
            save();
        }
        return;
    }
    if (id == 2) { // Remove All
        removeAllTagsFromSelectedNotes();
        filterAgain();
        save();
        return;
    }
    if (id == 3) { // Customize...
        TagsEditDialog dialog(m_view);
        dialog.exec();
        return;
    }

    Tag *tag = Tag::all[id - 10];
    if (!tag)
        return;

    if (m_tagPopupNote->hasTag(tag))
        removeTagFromSelectedNotes(tag);
    else
        addTagToSelectedNotes(tag);
    m_tagPopupNote->setWidth(0); // To force a new layout computation
    filterAgain();
    save();
}

void BasketScene::addTagToSelectedNotes(Tag *tag)
{
    FOR_EACH_NOTE(note)
    note->addTagToSelectedNotes(tag);
    updateEditorAppearance();
}

void BasketScene::removeTagFromSelectedNotes(Tag *tag)
{
    FOR_EACH_NOTE(note)
    note->removeTagFromSelectedNotes(tag);
    updateEditorAppearance();
}

void BasketScene::addStateToSelectedNotes(State *state)
{
    FOR_EACH_NOTE(note)
    note->addStateToSelectedNotes(state);
    updateEditorAppearance();
}

void BasketScene::updateEditorAppearance()
{
    if (isDuringEdit() && m_editor->graphicsWidget()) {
        m_editor->graphicsWidget()->setFont(m_editor->note()->font());

        if (m_editor->graphicsWidget()->widget()) {
            QPalette palette;
            palette.setColor(m_editor->graphicsWidget()->widget()->backgroundRole(), m_editor->note()->backgroundColor());
            palette.setColor(m_editor->graphicsWidget()->widget()->foregroundRole(), m_editor->note()->textColor());
            m_editor->graphicsWidget()->setPalette(palette);
        }

        // Ugly Hack around Qt bugs: placeCursor() don't call any signal:
        HtmlEditor *htmlEditor = dynamic_cast<HtmlEditor *>(m_editor);
        if (htmlEditor) {
            if (m_editor->textEdit()->textCursor().atStart()) {
                m_editor->textEdit()->moveCursor(QTextCursor::Right);
                m_editor->textEdit()->moveCursor(QTextCursor::Left);
            } else {
                m_editor->textEdit()->moveCursor(QTextCursor::Left);
                m_editor->textEdit()->moveCursor(QTextCursor::Right);
            }
            htmlEditor->cursorPositionChanged(); // Does not work anyway :-( (when clicking on a red bold text, the toolbar still show black normal text)
        }
    }
}

void BasketScene::editorPropertiesChanged()
{
    if (isDuringEdit() && m_editor->note()->content()->type() == NoteType::Html) {
        m_editor->textEdit()->setAutoFormatting(Settings::autoBullet() ? QTextEdit::AutoAll : QTextEdit::AutoNone);
    }
}

void BasketScene::changeStateOfSelectedNotes(State *state)
{
    FOR_EACH_NOTE(note)
    note->changeStateOfSelectedNotes(state);
    updateEditorAppearance();
}

void BasketScene::removeAllTagsFromSelectedNotes()
{
    FOR_EACH_NOTE(note)
    note->removeAllTagsFromSelectedNotes();
    updateEditorAppearance();
}

bool BasketScene::selectedNotesHaveTags()
{
    FOR_EACH_NOTE(note)
    if (note->selectedNotesHaveTags())
        return true;
    return false;
}

QColor BasketScene::backgroundColor() const
{
    if (m_backgroundColorSetting.isValid())
        return m_backgroundColorSetting;
    else
        return palette().color(QPalette::Base);
}

QColor BasketScene::textColor() const
{
    if (m_textColorSetting.isValid())
        return m_textColorSetting;
    else
        return palette().color(QPalette::Text);
}

void BasketScene::unbufferizeAll()
{
    FOR_EACH_NOTE(note)
    note->unbufferizeAll();
}

Note *BasketScene::editedNote()
{
    if (m_editor)
        return m_editor->note();
    else
        return nullptr;
}

bool BasketScene::hasTextInEditor()
{
    if (!isDuringEdit() || !redirectEditActions())
        return false;

    if (m_editor->textEdit())
        return !m_editor->textEdit()->document()->isEmpty();
    else if (m_editor->lineEdit())
        return !m_editor->lineEdit()->displayText().isEmpty();
    else
        return false;
}

bool BasketScene::hasSelectedTextInEditor()
{
    if (!isDuringEdit() || !redirectEditActions())
        return false;

    if (m_editor->textEdit()) {
        // The following line does NOT work if one letter is selected and the user press Shift+Left or Shift+Right to unselect than letter:
        // Qt mysteriously tell us there is an invisible selection!!
        // return m_editor->textEdit()->hasSelectedText();
        return !m_editor->textEdit()->textCursor().selectedText().isEmpty();
    } else if (m_editor->lineEdit())
        return m_editor->lineEdit()->hasSelectedText();
    else
        return false;
}

bool BasketScene::selectedAllTextInEditor()
{
    if (!isDuringEdit() || !redirectEditActions())
        return false;

    if (m_editor->textEdit()) {
        return m_editor->textEdit()->document()->isEmpty() || m_editor->textEdit()->toPlainText() == m_editor->textEdit()->textCursor().selectedText();
    } else if (m_editor->lineEdit())
        return m_editor->lineEdit()->displayText().isEmpty() || m_editor->lineEdit()->displayText() == m_editor->lineEdit()->selectedText();
    else
        return false;
}

void BasketScene::selectionChangedInEditor()
{
    Global::bnpView->notesStateChanged();
}

void BasketScene::contentChangedInEditor()
{
    // Do not wait 3 seconds, because we need the note to expand as needed (if a line is too wider... the note should grow wider):
    if (m_editor->textEdit())
        m_editor->autoSave(/*toFileToo=*/false);
    //  else {
    if (m_inactivityAutoSaveTimer.isActive())
        m_inactivityAutoSaveTimer.stop();
    m_inactivityAutoSaveTimer.setSingleShot(true);
    m_inactivityAutoSaveTimer.start(3 * 1000);
    Global::bnpView->setUnsavedStatus(true);
    //  }
}

void BasketScene::inactivityAutoSaveTimeout()
{
    if (m_editor)
        m_editor->autoSave(/*toFileToo=*/true);
}

void BasketScene::placeEditorAndEnsureVisible()
{
    placeEditor(/*andEnsureVisible=*/true);
}

// TODO: [kw] Oh boy, this will probably require some tweaking.
void BasketScene::placeEditor(bool /*andEnsureVisible*/ /*= false*/)
{
    if (!isDuringEdit())
        return;

    QFrame *editorQFrame = dynamic_cast<QFrame *>(m_editor->graphicsWidget()->widget());
    KTextEdit *textEdit = m_editor->textEdit();
    Note *note = m_editor->note();

    qreal frameWidth = (editorQFrame ? editorQFrame->frameWidth() : 0);
    qreal x = note->x() + note->contentX() + note->content()->xEditorIndent() - frameWidth;
    qreal y;
    qreal maxHeight = qMax((qreal)m_view->viewport()->height(), sceneRect().height());
    qreal height, width;

    if (textEdit) {
        // Need to do it 2 times, because it's wrong otherwise
        // (sometimes, width depends on height, and sometimes, height depends on with):
        for (int i = 0; i < 2; i++) {
            // FIXME: CRASH: Select all text, press Del or [<--] and editor->removeSelectedText() is called:
            //        editor->sync() CRASH!!
            //      editor->sync();
            y = note->y() + Note::NOTE_MARGIN - frameWidth;
            height = note->height() - 2 * frameWidth - 2 * Note::NOTE_MARGIN;
            width = note->x() + note->width() - x + 1;
            if (y + height > maxHeight)
                y = maxHeight - height;

            m_editor->graphicsWidget()->setMaximumSize(width, height);
            textEdit->setFixedSize(width, height);
            textEdit->viewport()->setFixedSize(width, height);
        }
    } else {
        height = note->height() - 2 * Note::NOTE_MARGIN + 2 * frameWidth;
        width = note->x() + note->width() - x; // note->rightLimit() - x + 2*m_view->frameWidth;
        if (m_editor->graphicsWidget())
            m_editor->graphicsWidget()->widget()->setFixedSize(width, height);
        x -= 1;
        y = note->y() + Note::NOTE_MARGIN - frameWidth;
    }
    if ((m_editorWidth > 0 && m_editorWidth != width) || (m_editorHeight > 0 && m_editorHeight != height)) {
        m_editorWidth = width; // Avoid infinite recursion!!!
        m_editorHeight = height;
        m_editor->autoSave(/*toFileToo=*/true);
    }
    m_editorWidth = width;
    m_editorHeight = height;
    m_editor->graphicsWidget()->setPos(x, y);
    m_editorX = x;
    m_editorY = y;

    //  if (andEnsureVisible)
    //      ensureNoteVisible(note);
}

void BasketScene::editorCursorPositionChanged()
{
    if (!isDuringEdit())
        return;

    FocusedTextEdit *textEdit = dynamic_cast<FocusedTextEdit *>(m_editor->textEdit());

    if (textEdit) {
        QPoint cursorPoint = textEdit->viewport()->mapToGlobal(textEdit->cursorRect().center());

        // QPointF contentsCursor = m_view->mapToScene( m_view->viewport()->mapFromGlobal(cursorPoint) );
        // m_view->ensureVisible(contentsCursor.x(), contentsCursor.y(),1,1);
    }
}

void BasketScene::closeEditorDelayed()
{
    setFocus();
    QTimer::singleShot(0, this, SLOT(closeEditor()));
}

bool BasketScene::closeEditor(bool deleteEmptyNote /* =true*/)
{
    if (!isDuringEdit())
        return true;

    if (m_doNotCloseEditor)
        return true;

    if (m_redirectEditActions) {
        if (m_editor->textEdit()) {
            disconnect(m_editor->textEdit(), &KTextEdit::selectionChanged, this, &BasketScene::selectionChangedInEditor);
            disconnect(m_editor->textEdit(), &KTextEdit::textChanged, this, &BasketScene::selectionChangedInEditor);
            disconnect(m_editor->textEdit(), &KTextEdit::textChanged, this, &BasketScene::contentChangedInEditor);
        } else if (m_editor->lineEdit()) {
            disconnect(m_editor->lineEdit(), &QLineEdit::selectionChanged, this, &BasketScene::selectionChangedInEditor);
            disconnect(m_editor->lineEdit(), &QLineEdit::textChanged, this, &BasketScene::selectionChangedInEditor);
            disconnect(m_editor->lineEdit(), &QLineEdit::textChanged, this, &BasketScene::contentChangedInEditor);
        }
    }
    m_editorTrackMouseEvent = false;
    m_editor->graphicsWidget()->widget()->disconnect();
    removeItem(m_editor->graphicsWidget());
    m_editor->validate();

    Note *note = m_editor->note();

    // Delete the editor BEFORE unselecting the note because unselecting the note would trigger closeEditor() recursivly:
    bool isEmpty = m_editor->isEmpty();
    delete m_editor;

    m_editor = nullptr;
    m_redirectEditActions = false;
    m_editorWidth = -1;
    m_editorHeight = -1;
    m_inactivityAutoSaveTimer.stop();

    // Delete the note if it is now empty:
    if (isEmpty && deleteEmptyNote) {
        focusANonSelectedNoteAboveOrThenBelow();
        note->setSelected(true);
        note->deleteSelectedNotes();
        if (m_hoveredNote == note)
            m_hoveredNote = nullptr;
        if (m_focusedNote == note)
            m_focusedNote = nullptr;
        delete note;
        save();
        note = nullptr;
    }

    unlockHovering();
    filterAgain(/*andEnsureVisible=*/false);

    // Does not work:
    //  if (Settings::playAnimations())
    //      note->setOnTop(true); // So if it grew, do not obscure it temporarily while the notes below it are moving

    if (note)
        note->setSelected(false); // unselectAll();
    doHoverEffects();
    //  save();

    Global::bnpView->m_actEditNote->setEnabled(!isLocked() && countSelecteds() == 1 /*&& !isDuringEdit()*/);

    Q_EMIT resetStatusBarText(); // Remove the "Editing. ... to validate." text.

    // if (qApp->activeWindow() == Global::mainContainer)

    // Set focus to the basket, unless the user pressed a letter key in the filter bar and the currently edited note came hidden, then editing closed:
    if (!decoration()->filterBar()->lineEdit()->hasFocus())
        setFocus();

    // Return true if the note is still there:
    return (note != nullptr);
}

void BasketScene::closeBasket()
{
    closeEditor();
    unbufferizeAll(); // Keep the memory footprint low
    if (isEncrypted()) {
        if (Settings::enableReLockTimeout()) {
            int seconds = Settings::reLockTimeoutMinutes() * 60;
            m_inactivityAutoLockTimer.setSingleShot(true);
            m_inactivityAutoLockTimer.start(seconds * 1000);
        }
    }
}

void BasketScene::openBasket()
{
    if (m_inactivityAutoLockTimer.isActive())
        m_inactivityAutoLockTimer.stop();
}

Note *BasketScene::theSelectedNote()
{
    if (countSelecteds() != 1) {
        qDebug() << "NO SELECTED NOTE !!!!";
        return nullptr;
    }

    Note *selectedOne;
    FOR_EACH_NOTE(note)
    {
        selectedOne = note->theSelectedNote();
        if (selectedOne)
            return selectedOne;
    }

    qDebug() << "One selected note, BUT NOT FOUND !!!!";

    return nullptr;
}

NoteSelection *BasketScene::selectedNotes()
{
    NoteSelection selection;

    FOR_EACH_NOTE(note)
    selection.append(note->selectedNotes());

    if (!selection.firstChild)
        return nullptr;

    for (NoteSelection *node = selection.firstChild; node; node = node->next)
        node->parent = nullptr;

    // If the top-most groups are columns, export only children of those groups
    // (because user is not aware that columns are groups, and don't care: it's not what she want):
    if (selection.firstChild->note->isColumn()) {
        NoteSelection tmpSelection;
        NoteSelection *nextNode;
        NoteSelection *nextSubNode;
        for (NoteSelection *node = selection.firstChild; node; node = nextNode) {
            nextNode = node->next;
            if (node->note->isColumn()) {
                for (NoteSelection *subNode = node->firstChild; subNode; subNode = nextSubNode) {
                    nextSubNode = subNode->next;
                    tmpSelection.append(subNode);
                    subNode->parent = nullptr;
                    subNode->next = nullptr;
                }
            } else {
                tmpSelection.append(node);
                node->parent = nullptr;
                node->next = nullptr;
            }
        }
        //      debugSel(tmpSelection.firstChild);
        return tmpSelection.firstChild;
    } else {
        //      debugSel(selection.firstChild);
        return selection.firstChild;
    }
}

void BasketScene::showEditedNoteWhileFiltering()
{
    if (m_editor) {
        Note *note = m_editor->note();
        filterAgain();
        note->setSelected(true);
        relayoutNotes();
        note->setX(note->x());
        note->setY(note->y());
        filterAgainDelayed();
    }
}

void BasketScene::noteEdit(Note *note, bool justAdded, const QPointF &clickedPoint) // TODO: Remove the first parameter!!!
{
    if (!note)
        note = theSelectedNote(); // TODO: Or pick the focused note!
    if (!note)
        return;

    if (isDuringEdit()) {
        closeEditor(); // Validate the noteeditors in QLineEdit that does not intercept Enter key press (and edit is triggered with Enter too... Can conflict)
        return;
    }

    if (note != m_focusedNote) {
        setFocusedNote(note);
        m_startOfShiftSelectionNote = note;
    }

    if (justAdded && isFiltering()) {
        QTimer::singleShot(0, this, SLOT(showEditedNoteWhileFiltering()));
    }

    doHoverEffects(note, Note::Content); // Be sure (in the case Edit was triggered by menu or Enter key...): better feedback!

    NoteEditor *editor = NoteEditor::editNoteContent(note->content(), nullptr);
    if (editor->graphicsWidget()) {
        m_editor = editor;

        addItem(m_editor->graphicsWidget());

        placeEditorAndEnsureVisible(); //       placeEditor(); // FIXME: After?
        m_redirectEditActions = m_editor->lineEdit() || m_editor->textEdit();
        if (m_redirectEditActions) {
            // In case there is NO text, "Select All" is disabled. But if the user press a key the there is now a text:
            // selection has not changed but "Select All" should be re-enabled:
            m_editor->connectActions(this);
        }

        m_editor->graphicsWidget()->setFocus();
        connect(m_editor, &NoteEditor::askValidation, this, &BasketScene::closeEditorDelayed);
        connect(m_editor, &NoteEditor::mouseEnteredEditorWidget, this, &BasketScene::mouseEnteredEditorWidget);

        if (clickedPoint != QPoint()) {
            m_editor->setCursorTo(clickedPoint);
            updateEditorAppearance();
        }

        //      qApp->processEvents();     // Show the editor toolbar before ensuring the note is visible
        ensureNoteVisible(note);                //  because toolbar can create a new line and then partially hide the note
        m_editor->graphicsWidget()->setFocus(); // When clicking in the basket, a QTimer::singleShot(0, ...) focus the basket! So we focus the widget after qApp->processEvents()
        Q_EMIT resetStatusBarText();              // Display "Editing. ... to validate."
    } else {
        // Delete the note user have canceled the addition:
        if ((justAdded && editor->canceled()) || editor->isEmpty() /*) && editor->note()->states().count() <= 0*/) {
            focusANonSelectedNoteAboveOrThenBelow();
            editor->note()->setSelected(true);
            editor->note()->deleteSelectedNotes();
            if (m_hoveredNote == editor->note())
                m_hoveredNote = nullptr;
            if (m_focusedNote == editor->note())
                m_focusedNote = nullptr;
            delete editor->note();
            save();
        }
        editor->deleteLater();
        unlockHovering();
        filterAgain();
        unselectAll();
    }
    // Must set focus to the editor, otherwise edit cursor is not seen and precomposed characters cannot be entered
    if (m_editor != nullptr && m_editor->textEdit() != nullptr)
        m_editor->textEdit()->setFocus();

    Global::bnpView->m_actEditNote->setEnabled(false);
}

void BasketScene::noteDelete()
{
    if (redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->textCursor().deleteChar();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->del();
        return;
    }

    if (countSelecteds() <= 0)
        return;
    int really = KMessageBox::Yes;
    if (Settings::confirmNoteDeletion())
        really = KMessageBox::questionYesNo(m_view,
                                            i18np("<qt>Do you really want to delete this note?</qt>", "<qt>Do you really want to delete these <b>%1</b> notes?</qt>", countSelecteds()),
                                            i18np("Delete Note", "Delete Notes", countSelecteds()),
                                            KStandardGuiItem::del(),
                                            KStandardGuiItem::cancel());
    if (really == KMessageBox::No)
        return;

    noteDeleteWithoutConfirmation();
}

void BasketScene::focusANonSelectedNoteBelow(bool inSameColumn)
{
    // First focus another unselected one below it...:
    if (m_focusedNote && m_focusedNote->isSelected()) {
        Note *next = m_focusedNote->nextShownInStack();
        while (next && next->isSelected())
            next = next->nextShownInStack();
        if (next) {
            if (inSameColumn && isColumnsLayout() && m_focusedNote->parentPrimaryNote() == next->parentPrimaryNote()) {
                setFocusedNote(next);
                m_startOfShiftSelectionNote = next;
            }
        }
    }
}

void BasketScene::focusANonSelectedNoteAbove(bool inSameColumn)
{
    // ... Or above it:
    if (m_focusedNote && m_focusedNote->isSelected()) {
        Note *prev = m_focusedNote->prevShownInStack();
        while (prev && prev->isSelected())
            prev = prev->prevShownInStack();
        if (prev) {
            if (inSameColumn && isColumnsLayout() && m_focusedNote->parentPrimaryNote() == prev->parentPrimaryNote()) {
                setFocusedNote(prev);
                m_startOfShiftSelectionNote = prev;
            }
        }
    }
}

void BasketScene::focusANonSelectedNoteBelowOrThenAbove()
{
    focusANonSelectedNoteBelow(/*inSameColumn=*/true);
    focusANonSelectedNoteAbove(/*inSameColumn=*/true);
    focusANonSelectedNoteBelow(/*inSameColumn=*/false);
    focusANonSelectedNoteAbove(/*inSameColumn=*/false);
}

void BasketScene::focusANonSelectedNoteAboveOrThenBelow()
{
    focusANonSelectedNoteAbove(/*inSameColumn=*/true);
    focusANonSelectedNoteBelow(/*inSameColumn=*/true);
    focusANonSelectedNoteAbove(/*inSameColumn=*/false);
    focusANonSelectedNoteBelow(/*inSameColumn=*/false);
}

void BasketScene::noteDeleteWithoutConfirmation(bool deleteFilesToo)
{
    // If the currently focused note is selected, it will be deleted.
    focusANonSelectedNoteBelowOrThenAbove();

    // Do the deletion:
    Note *note = firstNote();
    Note *next;
    while (note) {
        next = note->next(); // If we delete 'note' on the next line, note->next() will be 0!
        note->deleteSelectedNotes(deleteFilesToo, &m_notesToBeDeleted);
        note = next;
    }

    if (!m_notesToBeDeleted.isEmpty()) {
        doCleanUp();
    }

    relayoutNotes(); // FIXME: filterAgain()?
    save();
}

void BasketScene::doCopy(CopyMode copyMode)
{
    QClipboard *cb = QApplication::clipboard();
    QClipboard::Mode mode = ((copyMode == CopyToSelection) ? QClipboard::Selection : QClipboard::Clipboard);

    NoteSelection *selection = selectedNotes();
    int countCopied = countSelecteds();
    if (selection->firstStacked()) {
        QDrag *d = NoteDrag::dragObject(selection, copyMode == CutToClipboard, /*source=*/nullptr); // d will be deleted by QT

        cb->setMimeData(d->mimeData(), mode);

        if (copyMode == CutToClipboard) {
            noteDeleteWithoutConfirmation(/*deleteFilesToo=*/false);
            focusANote();
        }

        switch (copyMode) {
        default:
        case CopyToClipboard:
            Q_EMIT postMessage(i18np("Copied note to clipboard.", "Copied notes to clipboard.", countCopied));
            break;
        case CutToClipboard:
            Q_EMIT postMessage(i18np("Cut note to clipboard.", "Cut notes to clipboard.", countCopied));
            break;
        case CopyToSelection:
            Q_EMIT postMessage(i18np("Copied note to selection.", "Copied notes to selection.", countCopied));
            break;
        }
    }
}

void BasketScene::noteCopy()
{
    if (redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->copy();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->copy();
    } else
        doCopy(CopyToClipboard);
}

void BasketScene::noteCut()
{
    if (redirectEditActions()) {
        if (m_editor->textEdit())
            m_editor->textEdit()->cut();
        else if (m_editor->lineEdit())
            m_editor->lineEdit()->cut();
    } else
        doCopy(CutToClipboard);
}

void BasketScene::clearFormattingNote(Note *note)
{
    if (!note)
        note = theSelectedNote();
    if (!note)
        return;

    HtmlContent* contentHtml = dynamic_cast<HtmlContent*>(note->content());
    if (contentHtml){
        contentHtml->setHtml(Tools::textToHTML(note->content()->toText("")));
    }

    noteEdit(note);
}

void BasketScene::noteOpen(Note *note)
{
    /*
    GetSelectedNotes
    NoSelectedNote || Count == 0 ? return
    AllTheSameType ?
    Get { url, message(count) }
    */

    // TODO: Open ALL selected notes!
    if (note == nullptr) {
        note = theSelectedNote();
    }
    if (note == nullptr) {
        return;
    }

    QUrl url = note->content()->urlToOpen(/*with=*/false);
    QString message = note->content()->messageWhenOpening(NoteContent::OpenOne /*NoteContent::OpenSeveral*/);
    if (url.isEmpty()) {
        if (message.isEmpty()) {
            Q_EMIT postMessage(i18n("Unable to open this note.") /*"Unable to open those notes."*/);
        } else {
            int result = KMessageBox::warningContinueCancel(m_view, message, /*caption=*/QString(), KGuiItem(i18n("&Edit"), "edit"));
            if (result == KMessageBox::Continue) {
                noteEdit(note);
            }
        }
    } else {
        Q_EMIT postMessage(message); // "Opening link target..." / "Launching application..." / "Opening note file..."
        // Finally do the opening job:
        QString customCommand = note->content()->customOpenCommand();

        if (url.url().startsWith(QStringLiteral("basket://"))) {
            Q_EMIT crossReference(url.url());
        } else if (customCommand.isEmpty()) {
            KRun *run = new KRun(url, m_view->window());
            run->setAutoDelete(true);
        } else {
            QList<QUrl> urls {url};
            KService::Ptr service(new KService(QStringLiteral("noteOpen"), customCommand, QString()));
            auto *job = new KIO::ApplicationLauncherJob(service);
            job->setUrls(urls);
            job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_view->window()));
            job->start();
        }
    }
}

/** Code from bool KRun::displayOpenWithDialog(const KUrl::List& lst, bool tempFiles)
 * It does not allow to set a text, so I ripped it to do that:
 */
bool KRun__displayOpenWithDialog(const QList<QUrl> &lst, QWidget *window, bool tempFiles, const QString &text)
{
    if (qApp && !KAuthorized::authorizeAction(QStringLiteral("openwith"))) {
        KMessageBox::error(window, i18n("You are not authorized to open this file.")); // TODO: Better message, i18n freeze :-(
        return false;
    }
    KOpenWithDialog l(lst, text, QString(), nullptr);
    if (l.exec() > 0) {
        KService::Ptr service = l.service();
        if (!service) {
            service = new KService(QStringLiteral("noteOpenWith"), l.text(), QString());
        }

        auto *job = new KIO::ApplicationLauncherJob(service);
        job->setUrls(lst);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window));
        if (tempFiles) {
            job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
        }

        /// \todo job runs synchronously... API suggests asynchronous start() instead.
        /// This requires a result handler. noteOpenWith also just emits a message to the status bar.
        return job->exec();

    }
    return false;
}

void BasketScene::noteOpenWith(Note *note)
{
    if (note == nullptr) {
        note = theSelectedNote();
    }
    if (note == nullptr) {
        return;
    }

    QUrl url = note->content()->urlToOpen(/*with=*/true);
    QString message = note->content()->messageWhenOpening(NoteContent::OpenOneWith);
    QString text = note->content()->messageWhenOpening(NoteContent::OpenOneWithDialog);
    if (url.isEmpty()) {
        Q_EMIT postMessage(i18n("Unable to open this note."));
    } else {
        QList<QUrl> urls{url};
        if (KRun__displayOpenWithDialog(urls, m_view->window(), false, text)) {
            Q_EMIT postMessage(message); // "Opening link target with..." / "Opening note file with..."
        }
    }
}

void BasketScene::noteSaveAs()
{
    //  if (!note)
    //      note = theSelectedNote();
    Note *note = theSelectedNote();
    if (!note)
        return;

    QUrl url = note->content()->urlToOpen(/*with=*/false);
    if (url.isEmpty())
        return;

    QString fileName = QFileDialog::getSaveFileName(m_view, i18n("Save to File"), url.fileName(), note->content()->saveAsFilters());
    // TODO: Ask to overwrite !
    if (fileName.isEmpty())
        return;

    // TODO: Convert format, etc. (use NoteContent::saveAs(fileName))
    KIO::copy(url, QUrl::fromLocalFile(fileName));
}

Note *BasketScene::selectedGroup()
{
    FOR_EACH_NOTE(note)
    {
        Note *selectedGroup = note->selectedGroup();
        if (selectedGroup) {
            // If the selected group is one group in a column, then return that group, and not the column,
            // because the column is not ungrouppage, and the Ungroup action would be disabled.
            if (selectedGroup->isColumn() && selectedGroup->firstChild() && !selectedGroup->firstChild()->next()) {
                return selectedGroup->firstChild();
            }
            return selectedGroup;
        }
    }
    return nullptr;
}

bool BasketScene::selectionIsOneGroup()
{
    return (selectedGroup() != nullptr);
}

Note *BasketScene::firstSelected()
{
    Note *first = nullptr;
    FOR_EACH_NOTE(note)
    {
        first = note->firstSelected();
        if (first)
            return first;
    }
    return nullptr;
}

Note *BasketScene::lastSelected()
{
    Note *last = nullptr, *tmp = nullptr;
    FOR_EACH_NOTE(note)
    {
        tmp = note->lastSelected();
        if (tmp)
            last = tmp;
    }
    return last;
}

bool BasketScene::convertTexts()
{
    m_watcher->stopScan();
    bool convertedNotes = false;

    if (!isLoaded())
        load();

    FOR_EACH_NOTE(note)
    if (note->convertTexts())
        convertedNotes = true;

    if (convertedNotes)
        save();
    m_watcher->startScan();
    return convertedNotes;
}

void BasketScene::noteGroup()
{
    /*  // Nothing to do?
        if (isLocked() || countSelecteds() <= 1)
            return;

        // If every selected notes are ALREADY in one group, then don't touch anything:
        Note *selectedGroup = this->selectedGroup();
        if (selectedGroup && !selectedGroup->isColumn())
            return;
    */

    // Copied from BNPView::updateNotesActions()
    bool severalSelected = countSelecteds() >= 2;
    Note *selectedGroup = (severalSelected ? this->selectedGroup() : nullptr);
    bool enabled = !isLocked() && severalSelected && (!selectedGroup || selectedGroup->isColumn());
    if (!enabled)
        return;

    // Get the first selected note: we will group selected items just before:
    Note *first = firstSelected();
    //  if (selectedGroup != 0 || first == 0)
    //      return;

    m_loaded = false; // Hack to avoid notes to be unselected and new notes to be selected:

    // Create and insert the receiving group:
    Note *group = new Note(this);
    if (first->isFree()) {
        insertNote(group, nullptr, Note::BottomColumn, QPointF(first->x(), first->y()));
    } else {
        insertNote(group, first, Note::TopInsert);
    }

    // Put a FAKE UNSELECTED note in the new group, so if the new group is inside an allSelected() group, the parent group is not moved inside the new group!
    Note *fakeNote = NoteFactory::createNoteColor(Qt::red, this);
    insertNote(fakeNote, group, Note::BottomColumn);

    // Group the notes:
    Note *nextNote;
    Note *note = firstNote();
    while (note) {
        nextNote = note->next();
        note->groupIn(group);
        note = nextNote;
    }

    m_loaded = true; // Part 2 / 2 of the workaround!

    // Do cleanup:
    unplugNote(fakeNote);
    delete fakeNote;
    unselectAll();
    group->setSelectedRecursively(true); // Notes were unselected by unplugging

    relayoutNotes();
    save();
}

void BasketScene::noteUngroup()
{
    Note *group = selectedGroup();
    if (group && !group->isColumn()) {
        ungroupNote(group);
        relayoutNotes();
    }
    save();
}

void BasketScene::unplugSelection(NoteSelection *selection)
{
    for (NoteSelection *toUnplug = selection->firstStacked(); toUnplug; toUnplug = toUnplug->nextStacked()) {
        unplugNote(toUnplug->note);
    }
}

void BasketScene::insertSelection(NoteSelection *selection, Note *after)
{
    for (NoteSelection *toUnplug = selection->firstStacked(); toUnplug; toUnplug = toUnplug->nextStacked()) {
        if (toUnplug->note->isGroup()) {
            Note *group = new Note(this);
            insertNote(group, after, Note::BottomInsert);
            Note *fakeNote = NoteFactory::createNoteColor(Qt::red, this);
            insertNote(fakeNote, group, Note::BottomColumn);
            insertSelection(toUnplug->firstChild, fakeNote);
            unplugNote(fakeNote);
            delete fakeNote;
            after = group;
        } else {
            Note *note = toUnplug->note;
            note->setPrev(nullptr);
            note->setNext(nullptr);
            insertNote(note, after, Note::BottomInsert);
            after = note;
        }
    }
}

void BasketScene::selectSelection(NoteSelection *selection)
{
    for (NoteSelection *toUnplug = selection->firstStacked(); toUnplug; toUnplug = toUnplug->nextStacked()) {
        if (toUnplug->note->isGroup())
            selectSelection(toUnplug);
        else
            toUnplug->note->setSelected(true);
    }
}

void BasketScene::noteMoveOnTop()
{
    // TODO: Get the group containing the selected notes and first move inside the group, then inside parent group, then in the basket
    // TODO: Move on top/bottom... of the column or basjet

    NoteSelection *selection = selectedNotes();
    unplugSelection(selection);
    // Replug the notes:
    Note *fakeNote = NoteFactory::createNoteColor(Qt::red, this);
    if (isColumnsLayout()) {
        if (firstNote()->firstChild())
            insertNote(fakeNote, firstNote()->firstChild(), Note::TopInsert);
        else
            insertNote(fakeNote, firstNote(), Note::BottomColumn);
    } else {
        // TODO: Also allow to move notes on top of a group!!!!!!!
        insertNote(fakeNote, nullptr, Note::BottomInsert);
    }
    insertSelection(selection, fakeNote);
    unplugNote(fakeNote);
    delete fakeNote;
    selectSelection(selection);
    relayoutNotes();
    save();
}

void BasketScene::noteMoveOnBottom()
{
    // TODO: Duplicate code: void noteMoveOn();

    // TODO: Get the group containing the selected notes and first move inside the group, then inside parent group, then in the basket
    // TODO: Move on top/bottom... of the column or basjet

    NoteSelection *selection = selectedNotes();
    unplugSelection(selection);
    // Replug the notes:
    Note *fakeNote = NoteFactory::createNoteColor(Qt::red, this);
    if (isColumnsLayout())
        insertNote(fakeNote, firstNote(), Note::BottomColumn);
    else {
        // TODO: Also allow to move notes on top of a group!!!!!!!
        insertNote(fakeNote, nullptr, Note::BottomInsert);
    }
    insertSelection(selection, fakeNote);
    unplugNote(fakeNote);
    delete fakeNote;
    selectSelection(selection);
    relayoutNotes();
    save();
}

void BasketScene::moveSelectionTo(Note *here, bool below /* = true*/)
{
    NoteSelection *selection = selectedNotes();
    unplugSelection(selection);
    // Replug the notes:
    Note *fakeNote = NoteFactory::createNoteColor(Qt::red, this);
    //  if (isColumnsLayout())
    insertNote(fakeNote, here, (below ? Note::BottomInsert : Note::TopInsert));
    //  else {
    //      // TODO: Also allow to move notes on top of a group!!!!!!!
    //      insertNote(fakeNote, 0, Note::BottomInsert, QPoint(0, 0), /*animateNewPosition=*/false);
    //  }
    insertSelection(selection, fakeNote);
    unplugNote(fakeNote);
    delete fakeNote;
    selectSelection(selection);
    relayoutNotes();
    save();
}

void BasketScene::noteMoveNoteUp()
{
    // TODO: Move between columns, even if they are empty !!!!!!!

    // TODO: if first note of a group, move just above the group! And let that even if there is no note before that group!!!

    Note *first = firstSelected();
    Note *previous = first->prevShownInStack();
    if (previous)
        moveSelectionTo(previous, /*below=*/false);
}

void BasketScene::noteMoveNoteDown()
{
    Note *first = lastSelected();
    Note *next = first->nextShownInStack();
    if (next)
        moveSelectionTo(next, /*below=*/true);
}

void BasketScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // Q3ScrollView::wheelEvent(event);
    QGraphicsScene::wheelEvent(event);
}

void BasketScene::linkLookChanged()
{
    Note *note = m_firstNote;
    while (note) {
        note->linkLookChanged();
        note = note->next();
    }
    relayoutNotes();
}

void BasketScene::slotCopyingDone2(KIO::Job *job, const QUrl & /*from*/, const QUrl &to)
{
    if (job->error()) {
        DEBUG_WIN << "Copy finished, ERROR";
        return;
    }
    Note *note = noteForFullPath(to.path());
    DEBUG_WIN << "Copy finished, load note: " + to.path() + (note ? QString() : " --- NO CORRESPONDING NOTE");
    if (note != nullptr) {
        note->content()->loadFromFile(/*lazyLoad=*/false);
        if (isEncrypted())
            note->content()->saveToFile();
        if (m_focusedNote == note)   // When inserting a new note we ensure it visible
            ensureNoteVisible(note); //  But after loading it has certainly grown and if it was
    }                                //  on bottom of the basket it's not visible entirely anymore
}

Note *BasketScene::noteForFullPath(const QString &path)
{
    Note *note = firstNote();
    Note *found;
    while (note) {
        found = note->noteForFullPath(path);
        if (found)
            return found;
        note = note->next();
    }
    return nullptr;
}

void BasketScene::deleteFiles()
{
    m_watcher->stopScan();
    Tools::deleteRecursively(fullPath());
}

QList<State *> BasketScene::usedStates()
{
    QList<State *> states;
    FOR_EACH_NOTE(note)
    note->usedStates(states);
    return states;
}

void BasketScene::listUsedTags(QList<Tag *> &list)
{
    if (!isLoaded()) {
        load();
    }

    FOR_EACH_NOTE(child)
    child->listUsedTags(list);
}

/** Unfocus the previously focused note (unless it was null)
 * and focus the new @param note (unless it is null) if hasFocus()
 * Update m_focusedNote to the new one
 */
void BasketScene::setFocusedNote(Note *note) // void BasketScene::changeFocusTo(Note *note)
{
    // Don't focus an hidden note:
    if (note != nullptr && !note->isShown())
        return;
    // When clicking a group, this group gets focused. But only content-based notes should be focused:
    if (note && note->isGroup())
        note = note->firstRealChild();
    // The first time a note is focused, it becomes the start of the Shift selection:
    if (m_startOfShiftSelectionNote == nullptr)
        m_startOfShiftSelectionNote = note;
    // Unfocus the old focused note:
    if (m_focusedNote != nullptr)
        m_focusedNote->setFocused(false);
    // Notify the new one to draw a focus rectangle... only if the basket is focused:
    if (hasFocus() && note != nullptr)
        note->setFocused(true);
    // Save the new focused note:
    m_focusedNote = note;
}

/** If no shown note is currently focused, try to find a shown note and focus it
 * Also update m_focusedNote to the new one (or null if there isn't)
 */
void BasketScene::focusANote()
{
    if (countFounds() == 0) { // No note to focus
        setFocusedNote(nullptr);
        //      m_startOfShiftSelectionNote = 0;
        return;
    }

    if (m_focusedNote == nullptr) { // No focused note yet : focus the first shown
        Note *toFocus = (isFreeLayout() ? noteOnHome() : firstNoteShownInStack());
        setFocusedNote(toFocus);
        //      m_startOfShiftSelectionNote = m_focusedNote;
        return;
    }

    // Search a visible note to focus if the focused one isn't shown :
    Note *toFocus = m_focusedNote;
    if (toFocus && !toFocus->isShown())
        toFocus = toFocus->nextShownInStack();
    if (!toFocus && m_focusedNote)
        toFocus = m_focusedNote->prevShownInStack();
    setFocusedNote(toFocus);
    //  m_startOfShiftSelectionNote = toFocus;
}

Note *BasketScene::firstNoteInStack()
{
    if (!firstNote())
        return nullptr;

    if (firstNote()->content())
        return firstNote();
    else
        return firstNote()->nextInStack();
}

Note *BasketScene::lastNoteInStack()
{
    Note *note = lastNote();
    while (note) {
        if (note->content())
            return note;
        Note *possibleNote = note->lastRealChild();
        if (possibleNote && possibleNote->content())
            return possibleNote;
        note = note->prev();
    }
    return nullptr;
}

Note *BasketScene::firstNoteShownInStack()
{
    Note *first = firstNoteInStack();
    while (first && !first->isShown())
        first = first->nextInStack();
    return first;
}

Note *BasketScene::lastNoteShownInStack()
{
    Note *last = lastNoteInStack();
    while (last && !last->isShown())
        last = last->prevInStack();
    return last;
}

Note *BasketScene::noteOn(NoteOn side)
{
    Note *bestNote = nullptr;
    int distance = -1;
    //    int   bestDistance = contentsWidth() * contentsHeight() * 10;
    int bestDistance = sceneRect().width() * sceneRect().height() * 10;

    Note *note = firstNoteShownInStack();
    Note *primary = m_focusedNote->parentPrimaryNote();
    while (note) {
        switch (side) {
        case LEFT_SIDE:
            distance = m_focusedNote->distanceOnLeftRight(note, LEFT_SIDE);
            break;
        case RIGHT_SIDE:
            distance = m_focusedNote->distanceOnLeftRight(note, RIGHT_SIDE);
            break;
        case TOP_SIDE:
            distance = m_focusedNote->distanceOnTopBottom(note, TOP_SIDE);
            break;
        case BOTTOM_SIDE:
            distance = m_focusedNote->distanceOnTopBottom(note, BOTTOM_SIDE);
            break;
        }
        if ((side == TOP_SIDE || side == BOTTOM_SIDE || primary != note->parentPrimaryNote()) && note != m_focusedNote && distance > 0 && distance < bestDistance) {
            bestNote = note;
            bestDistance = distance;
        }
        note = note->nextShownInStack();
    }

    return bestNote;
}

Note *BasketScene::firstNoteInGroup()
{
    Note *child = m_focusedNote;
    Note *parent = (m_focusedNote ? m_focusedNote->parentNote() : nullptr);
    while (parent) {
        if (parent->firstChild() != child && !parent->isColumn())
            return parent->firstRealChild();
        child = parent;
        parent = parent->parentNote();
    }
    return nullptr;
}

Note *BasketScene::noteOnHome()
{
    // First try to find the first note of the group containing the focused note:
    Note *parent = (m_focusedNote ? m_focusedNote->parentNote() : nullptr);
    while (parent) {
        if (parent->nextShownInStack() != m_focusedNote)
            return parent->nextShownInStack();
        parent = parent->parentNote();
    }

    // If it was not found, then focus the very first note in the basket:
    if (isFreeLayout()) {
        Note *first = firstNoteShownInStack(); // The effective first note found
        Note *note = first;                    // The current note, to compare with the previous first note, if this new note is more on top
        if (note)
            note = note->nextShownInStack();
        while (note) {
            if (note->y() < first->y() || (note->y() == first->y() && note->x() < first->x()))
                first = note;
            note = note->nextShownInStack();
        }
        return first;
    } else
        return firstNoteShownInStack();
}

Note *BasketScene::noteOnEnd()
{
    Note *parent = (m_focusedNote ? m_focusedNote->parentNote() : nullptr);
    Note *lastChild;
    while (parent) {
        lastChild = parent->lastRealChild();
        if (lastChild && lastChild != m_focusedNote) {
            if (lastChild->isShown())
                return lastChild;
            lastChild = lastChild->prevShownInStack();
            if (lastChild && lastChild->isShown() && lastChild != m_focusedNote)
                return lastChild;
        }
        parent = parent->parentNote();
    }
    if (isFreeLayout()) {
        Note *last;
        Note *note;
        last = note = firstNoteShownInStack();
        note = note->nextShownInStack();
        while (note) {
            if (note->bottom() > last->bottom() || (note->bottom() == last->bottom() && note->x() > last->x()))
                last = note;
            note = note->nextShownInStack();
        }
        return last;
    } else
        return lastNoteShownInStack();
}

void BasketScene::keyPressEvent(QKeyEvent *event)
{
    if (isDuringEdit()) {
        QGraphicsScene::keyPressEvent(event);
        /*if( event->key() == Qt::Key_Return )
        {
      m_editor->graphicsWidget()->setFocus();
        }
        else if( event->key() == Qt::Key_Escape)
        {
      closeEditor();
        }*/
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        if (decoration()->filterData().isFiltering)
            decoration()->filterBar()->reset();
        else
            unselectAll();
    }

    if (countFounds() == 0)
        return;

    if (!m_focusedNote)
        return;

    Note *toFocus = nullptr;

    switch (event->key()) {
    case Qt::Key_Down:
        toFocus = (isFreeLayout() ? noteOn(BOTTOM_SIDE) : m_focusedNote->nextShownInStack());
        if (toFocus)
            break;
        //        scrollBy(0, 30); // This cases do not move focus to another note...
        return;
    case Qt::Key_Up:
        toFocus = (isFreeLayout() ? noteOn(TOP_SIDE) : m_focusedNote->prevShownInStack());
        if (toFocus)
            break;
        //	scrollBy(0, -30); // This cases do not move focus to another note...
        return;
    case Qt::Key_PageDown:
        if (isFreeLayout()) {
            Note *lastFocused = m_focusedNote;
            for (int i = 0; i < 10 && m_focusedNote; ++i)
                m_focusedNote = noteOn(BOTTOM_SIDE);
            toFocus = m_focusedNote;
            m_focusedNote = lastFocused;
        } else {
            toFocus = m_focusedNote;
            for (int i = 0; i < 10 && toFocus; ++i)
                toFocus = toFocus->nextShownInStack();
        }
        if (toFocus == nullptr)
            toFocus = (isFreeLayout() ? noteOnEnd() : lastNoteShownInStack());
        if (toFocus && toFocus != m_focusedNote)
            break;
        //        scrollBy(0, visibleHeight() / 2); // This cases do not move focus to another note...
        //        scrollBy(0, viewport()->height() / 2); // This cases do not move focus to another note...
        return;
    case Qt::Key_PageUp:
        if (isFreeLayout()) {
            Note *lastFocused = m_focusedNote;
            for (int i = 0; i < 10 && m_focusedNote; ++i)
                m_focusedNote = noteOn(TOP_SIDE);
            toFocus = m_focusedNote;
            m_focusedNote = lastFocused;
        } else {
            toFocus = m_focusedNote;
            for (int i = 0; i < 10 && toFocus; ++i)
                toFocus = toFocus->prevShownInStack();
        }
        if (toFocus == nullptr)
            toFocus = (isFreeLayout() ? noteOnHome() : firstNoteShownInStack());
        if (toFocus && toFocus != m_focusedNote)
            break;
        //        scrollBy(0, - visibleHeight() / 2); // This cases do not move focus to another note...
        //	  scrollBy(0, - viewport()->height() / 2); // This cases do not move focus to another note...
        return;
    case Qt::Key_Home:
        toFocus = noteOnHome();
        break;
    case Qt::Key_End:
        toFocus = noteOnEnd();
        break;
    case Qt::Key_Left:
        if (m_focusedNote->tryFoldParent())
            return;
        if ((toFocus = noteOn(LEFT_SIDE)))
            break;
        if ((toFocus = firstNoteInGroup()))
            break;
        //        scrollBy(-30, 0); // This cases do not move focus to another note...
        return;
    case Qt::Key_Right:
        if (m_focusedNote->tryExpandParent())
            return;
        if ((toFocus = noteOn(RIGHT_SIDE)))
            break;
        //	scrollBy(30, 0); // This cases do not move focus to another note...
        return;
    case Qt::Key_Space: // This case do not move focus to another note...
        if (m_focusedNote) {
            m_focusedNote->setSelected(!m_focusedNote->isSelected());
            event->accept();
        } else
            event->ignore();
        return; // ... so we return after the process
    default:
        return;
    }

    if (toFocus == nullptr) { // If no direction keys have been pressed OR reached out the begin or end
        event->ignore();      // Important !!
        return;
    }

    if (event->modifiers() & Qt::ShiftModifier) { // Shift+arrowKeys selection
        if (m_startOfShiftSelectionNote == nullptr)
            m_startOfShiftSelectionNote = toFocus;
        ensureNoteVisible(toFocus); // Important: this line should be before the other ones because else repaint would be done on the wrong part!
        selectRange(m_startOfShiftSelectionNote, toFocus);
        setFocusedNote(toFocus);
        event->accept();
        return;
    } else /*if (toFocus != m_focusedNote)*/ { // Move focus to ANOTHER note...
        ensureNoteVisible(toFocus);            // Important: this line should be before the other ones because else repaint would be done on the wrong part!
        setFocusedNote(toFocus);
        m_startOfShiftSelectionNote = toFocus;
        if (!(event->modifiers() & Qt::ControlModifier)) // ... select only current note if Control
            unselectAllBut(m_focusedNote);
        event->accept();
        return;
    }

    event->ignore(); // Important !!
}

/** Select a range of notes and deselect the others.
 * The order between start and end has no importance (end could be before start)
 */
void BasketScene::selectRange(Note *start, Note *end, bool unselectOthers /*= true*/)
{
    Note *cur;
    Note *realEnd = nullptr;

    // Avoid crash when start (or end) is null
    if (start == nullptr)
        start = end;
    else if (end == nullptr)
        end = start;
    // And if *both* are null
    if (start == nullptr) {
        if (unselectOthers)
            unselectAll();
        return;
    }
    // In case there is only one note to select
    if (start == end) {
        if (unselectOthers)
            unselectAllBut(start);
        else
            start->setSelected(true);
        return;
    }

    // Free layout baskets should select range as if we were drawing a rectangle between start and end:
    if (isFreeLayout()) {
        QRectF startRect(start->x(), start->y(), start->width(), start->height());
        QRectF endRect(end->x(), end->y(), end->width(), end->height());
        QRectF toSelect = startRect.united(endRect);
        selectNotesIn(toSelect, /*invertSelection=*/false, unselectOthers);
        return;
    }

    // Search the REAL first (and deselect the others before it) :
    for (cur = firstNoteInStack(); cur != nullptr; cur = cur->nextInStack()) {
        if (cur == start || cur == end)
            break;
        if (unselectOthers)
            cur->setSelected(false);
    }

    // Select the notes after REAL start, until REAL end :
    if (cur == start)
        realEnd = end;
    else if (cur == end)
        realEnd = start;

    for (/*cur = cur*/; cur != nullptr; cur = cur->nextInStack()) {
        cur->setSelected(cur->isShown()); // Select all notes in the range, but only if they are shown
        if (cur == realEnd)
            break;
    }

    if (!unselectOthers)
        return;

    // Deselect the remaining notes :
    if (cur)
        cur = cur->nextInStack();
    for (/*cur = cur*/; cur != nullptr; cur = cur->nextInStack())
        cur->setSelected(false);
}

void BasketScene::focusInEvent(QFocusEvent *event)
{
    // Focus cannot be get with Tab when locked, but a click can focus the basket!
    if (isLocked()) {
        if (m_button) {
            QGraphicsScene::focusInEvent(event);
            QTimer::singleShot(0, m_button, SLOT(setFocus()));
        }
    } else {
        QGraphicsScene::focusInEvent(event);
        focusANote(); // hasFocus() is true at this stage, note will be focused
    }
}

void BasketScene::focusOutEvent(QFocusEvent *)
{
    if (m_focusedNote != nullptr)
        m_focusedNote->setFocused(false);
}

void BasketScene::ensureNoteVisible(Note *note)
{
    if (!note->isShown()) // Logical!
        return;

    if (note == editedNote()) // HACK: When filtering while editing big notes, etc... cause unwanted scrolls
        return;

    m_view->ensureVisible(note);
    /*//    int bottom = note->y() + qMin(note->height(),                                             visibleHeight());
    //    int finalRight  = note->x() + qMin(note->width() + (note->hasResizer() ? Note::RESIZER_WIDTH : 0),  visibleWidth());
        qreal bottom = note->y() + qMin(note->height(),                                             (qreal)m_view->viewport()->height());
        qreal finalRight  = note->x() + qMin(note->width() + (note->hasResizer() ? Note::RESIZER_WIDTH : 0),  (qreal)m_view->viewport()->width());
        m_view->ensureVisible(finalRight,     bottom,    0, 0);
        m_view->ensureVisible(note->x(), note->y(), 0, 0);*/
}

void BasketScene::addWatchedFile(const QString &fullPath)
{
    //  DEBUG_WIN << "Watcher>Add Monitoring Of : <font color=blue>" + fullPath + "</font>";
    m_watcher->addFile(fullPath);
}

void BasketScene::removeWatchedFile(const QString &fullPath)
{
    //  DEBUG_WIN << "Watcher>Remove Monitoring Of : <font color=blue>" + fullPath + "</font>";
    m_watcher->removeFile(fullPath);
}

void BasketScene::watchedFileModified(const QString &fullPath)
{
    if (!m_modifiedFiles.contains(fullPath))
        m_modifiedFiles.append(fullPath);
    // If a big file is saved by an application, notifications are send several times.
    // We wait they are not sent anymore to consider the file complete!
    m_watcherTimer.setSingleShot(true);
    m_watcherTimer.start(200);
    DEBUG_WIN << "Watcher>Modified : <font color=blue>" + fullPath + "</font>";
}

void BasketScene::watchedFileDeleted(const QString &fullPath)
{
    Note *note = noteForFullPath(fullPath);
    removeWatchedFile(fullPath);
    if (note) {
        NoteSelection *selection = selectedNotes();
        unselectAllBut(note);
        noteDeleteWithoutConfirmation();
        while (selection) {
            selection->note->setSelected(true);
            selection = selection->nextStacked();
        }
    }
    DEBUG_WIN << "Watcher>Removed : <font color=blue>" + fullPath + "</font>";
}

void BasketScene::updateModifiedNotes()
{
    for (QList<QString>::iterator it = m_modifiedFiles.begin(); it != m_modifiedFiles.end(); ++it) {
        Note *note = noteForFullPath(*it);
        if (note)
            note->content()->loadFromFile(/*lazyLoad=*/false);
    }
    m_modifiedFiles.clear();
}

bool BasketScene::setProtection(int type, QString key)
{
#ifdef HAVE_LIBGPGME
    if (type == PasswordEncryption || // Ask a new password
        m_encryptionType != type || m_encryptionKey != key) {
        int savedType = m_encryptionType;
        QString savedKey = m_encryptionKey;

        m_encryptionType = type;
        m_encryptionKey = key;
        m_gpg->clearCache();

        if (saveAgain()) {
            Q_EMIT propertiesChanged(this);
        } else {
            m_encryptionType = savedType;
            m_encryptionKey = savedKey;
            m_gpg->clearCache();
            return false;
        }
    }
    return true;
#else
    m_encryptionType = type;
    m_encryptionKey = key;

    return false;
#endif
}

bool BasketScene::saveAgain()
{
    bool result = false;

    m_watcher->stopScan();
    // Re-encrypt basket file:
    result = save();
    // Re-encrypt every note files recursively:
    if (result) {
        FOR_EACH_NOTE(note)
        {
            result = note->saveAgain();
            if (!result)
                break;
        }
    }
    m_watcher->startScan();
    return result;
}

bool BasketScene::isEncrypted()
{
    return (m_encryptionType != NoEncryption);
}

bool BasketScene::isFileEncrypted()
{
    QFile file(fullPath() + ".basket");

    if (file.open(QIODevice::ReadOnly)) {
        // Should be ASCII anyways
        QString line = file.readLine(32);
        if (line.startsWith("-----BEGIN PGP MESSAGE-----"))
            return true;
    }
    return false;
}

void BasketScene::lock()
{
#ifdef HAVE_LIBGPGME
    closeEditor();
    m_gpg->clearCache();
    m_locked = true;
    enableActions();
    deleteNotes();
    m_loaded = false;
    m_loadingLaunched = false;
#endif
}
