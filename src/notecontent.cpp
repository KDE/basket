/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "notecontent.h"

#include <QLocale>
#include <QMimeData>
#include <QMimeDatabase>
#include <QTextBlock>
#include <QTextCodec>
#include <QWidget>
#include <QtCore/QBuffer>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtGui/QAbstractTextDocumentLayout> //For m_simpleRichText->documentLayout()
#include <QtGui/QBitmap>                     //For QPixmap::createHeuristicMask()
#include <QtGui/QFontMetrics>
#include <QtGui/QMovie>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtNetwork/QNetworkReply>
#include <QtXml/QDomElement>

#include <KEncodingProber>
#include <KFileItem>
#include <KFileMetaData/KFileMetaData/Extractor>
#include <KIO/AccessManager>
#include <KIO/PreviewJob> //For KIO::file_preview(...)
#include <KLocalizedString>
#include <KService>

#include <phonon/AudioOutput>
#include <phonon/MediaObject>

#include "basketscene.h"
#include "common.h"
#include "config.h"
#include "debugwindow.h"
#include "file_metadata.h"
#include "filter.h"
#include "global.h"
#include "htmlexporter.h"
#include "note.h"
#include "notefactory.h"
#include "settings.h"
#include "tools.h"
#include "xmlwork.h"

/**
 * LinkDisplayItem definition
 *
 */

QRectF LinkDisplayItem::boundingRect() const
{
    if (m_note) {
        return QRect(0, 0, m_note->width() - m_note->contentX() - Note::NOTE_MARGIN, m_note->height() - 2 * Note::NOTE_MARGIN);
    }
    return QRectF();
}

void LinkDisplayItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!m_note)
        return;

    QRectF rect = boundingRect();
    m_linkDisplay.paint(painter, 0, 0, rect.width(), rect.height(), m_note->palette(), true, m_note->isSelected(), m_note->hovered(), m_note->hovered() && m_note->hoveredZone() == Note::Custom0);
}

//** NoteType functions
QString NoteType::typeToName(const NoteType::Id noteType)
{
    switch (noteType) {
    case NoteType::Group:
        return i18n("Group");
    case NoteType::Text:
        return i18n("Plain Text");
    case NoteType::Html:
        return i18n("Text");
    case NoteType::Image:
        return i18n("Image");
    case NoteType::Animation:
        return i18n("Animation");
    case NoteType::Sound:
        return i18n("Sound");
    case NoteType::File:
        return i18n("File");
    case NoteType::Link:
        return i18n("Link");
    case NoteType::CrossReference:
        return i18n("Cross Reference");
    case NoteType::Launcher:
        return i18n("Launcher");
    case NoteType::Color:
        return i18n("Color");
    case NoteType::Unknown:
        return i18n("Unknown");
    }
    return i18n("Unknown");
}

QString NoteType::typeToLowerName(const NoteType::Id noteType)
{
    switch (noteType) {
    case NoteType::Group:
        return "group";
    case NoteType::Text:
        return "text";
    case NoteType::Html:
        return "html";
    case NoteType::Image:
        return "image";
    case NoteType::Animation:
        return "animation";
    case NoteType::Sound:
        return "sound";
    case NoteType::File:
        return "file";
    case NoteType::Link:
        return "link";
    case NoteType::CrossReference:
        return "cross_reference";
    case NoteType::Launcher:
        return "launcher";
    case NoteType::Color:
        return "color";
    case NoteType::Unknown:
        return "unknown";
    }
    return "unknown";
}

NoteType::Id NoteType::typeFromLowerName(const QString& lowerTypeName)
{
    if (lowerTypeName == "group") {
        return NoteType::Group;
    } else if (lowerTypeName == "text") {
        return NoteType::Text;
    } else if (lowerTypeName == "html") {
        return NoteType::Html;
    } else if (lowerTypeName == "image") {
        return NoteType::Image;
    } else if (lowerTypeName == "animation") {
        return NoteType::Animation;
    } else if (lowerTypeName == "sound") {
        return NoteType::Sound;
    } else if (lowerTypeName == "file")  {
        return NoteType::File;
    } else if (lowerTypeName == "link") {
        return NoteType::Link;
    } else if (lowerTypeName == "cross_reference") {
        return NoteType::CrossReference;
    } else if (lowerTypeName == "launcher") {
        return NoteType::Launcher;
    } else if (lowerTypeName == "color") {
        return NoteType::Color;
    } else if (lowerTypeName == "unknown") {
        return NoteType::Unknown;
    }
    return NoteType::Unknown;
}


/** class NoteContent:
 */

const int NoteContent::FEEDBACK_DARKING = 105;

NoteContent::NoteContent(Note *parent, const NoteType::Id type, const QString &fileName)
    : m_type(type)
    , m_note(parent)
{
    if (parent) {
        parent->setContent(this);
    }
    setFileName(fileName);
}

void NoteContent::saveToNode(QXmlStreamWriter &stream)
{
    if (useFile()) {
        stream.writeStartElement("content");
        stream.writeCharacters(fileName());
        stream.writeEndElement();
    }
}

QRectF NoteContent::zoneRect(int zone, const QPointF & /*pos*/)
{
    if (zone == Note::Content)
        return QRectF(0, 0, note()->width(), note()->height()); // Too wide and height, but it will be clipped by Note::zoneRect()
    else
        return QRectF();
}

QUrl NoteContent::urlToOpen(bool /*with*/)
{
    return (useFile() ? QUrl::fromLocalFile(fullPath()) : QUrl());
}

void NoteContent::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

bool NoteContent::trySetFileName(const QString &fileName)
{
    if (useFile() && fileName != m_fileName) {
        QString newFileName = Tools::fileNameForNewFile(fileName, basket()->fullPath());
        QDir dir;
        dir.rename(fullPath(), basket()->fullPathForFileName(newFileName));
        return true;
    }

    return false; // !useFile() or unsuccessful rename
}

QString NoteContent::fullPath()
{
    if (note() && useFile())
        return note()->fullPath();
    else
        return QString();
}

void NoteContent::contentChanged(qreal newMinWidth)
{
    m_minWidth = newMinWidth;
    if (note()) {
        //      note()->unbufferize();
        note()->requestRelayout(); // TODO: It should re-set the width!  m_width = 0 ?   contentChanged: setWidth, geteight, if size havent changed, only repaint and not relayout
    }
}

BasketScene *NoteContent::basket()
{
    if (note())
        return note()->basket();
    else
        return nullptr;
}

void NoteContent::setEdited()
{
    note()->setLastModificationDate(QDateTime::currentDateTime());
    basket()->save();
}

/** All the Content Classes:
 */

QString NoteContent::toText(const QString &cuttedFullPath)
{
    return (cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
}

QString TextContent::toText(const QString & /*cuttedFullPath*/)
{
    return text();
}
QString HtmlContent::toText(const QString & /*cuttedFullPath*/)
{
    return Tools::htmlToText(html());
}
QString LinkContent::toText(const QString & /*cuttedFullPath*/)
{
    if (autoTitle())
        return url().toDisplayString();
    else if (title().isEmpty() && url().isEmpty())
        return QString();
    else if (url().isEmpty())
        return title();
    else if (title().isEmpty())
        return url().toDisplayString();
    else
        return QString("%1 <%2>").arg(title(), url().toDisplayString());
}
QString CrossReferenceContent::toText(const QString & /*cuttedFullPath*/)
{
    if (title().isEmpty() && url().isEmpty())
        return QString();
    else if (url().isEmpty())
        return title();
    else if (title().isEmpty())
        return url().toDisplayString();
    else
        return QString("%1 <%2>").arg(title(), url().toDisplayString());
}
QString ColorContent::toText(const QString & /*cuttedFullPath*/)
{
    return color().name();
}
QString UnknownContent::toText(const QString & /*cuttedFullPath*/)
{
    return QString();
}

// TODO: If imageName.isEmpty() return fullPath() because it's for external use, else return fileName() because it's to display in a tooltip
QString TextContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    return Tools::textToHTMLWithoutP(text());
}

QString HtmlContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    // return Tools::htmlToParagraph(html());
    QTextDocument simpleRichText;
    simpleRichText.setHtml(html());
    return Tools::textDocumentToMinimalHTML(&simpleRichText);
}

QString ImageContent::toHtml(const QString & /*imageName*/, const QString &cuttedFullPath)
{
    return QString("<img src=\"%1\">").arg(cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
}

QString AnimationContent::toHtml(const QString & /*imageName*/, const QString &cuttedFullPath)
{
    return QString("<img src=\"%1\">").arg(cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
}

QString SoundContent::toHtml(const QString & /*imageName*/, const QString &cuttedFullPath)
{
    return QString("<a href=\"%1\">%2</a>").arg((cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath), fileName());
} // With the icon?

QString FileContent::toHtml(const QString & /*imageName*/, const QString &cuttedFullPath)
{
    return QString("<a href=\"%1\">%2</a>").arg((cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath), fileName());
} // With the icon?

QString LinkContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    return QString("<a href=\"%1\">%2</a>").arg(url().toDisplayString(), title());
} // With the icon?

QString CrossReferenceContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    return QString("<a href=\"%1\">%2</a>").arg(url().toDisplayString(), title());
} // With the icon?

QString LauncherContent::toHtml(const QString & /*imageName*/, const QString &cuttedFullPath)
{
    return QString("<a href=\"%1\">%2</a>").arg((cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath), name());
} // With the icon?

QString ColorContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    return QString("<span style=\"color: %1\">%2</span>").arg(color().name(), color().name());
}

QString UnknownContent::toHtml(const QString & /*imageName*/, const QString & /*cuttedFullPath*/)
{
    return QString();
}

QPixmap ImageContent::toPixmap()
{
    return pixmap();
}
QPixmap AnimationContent::toPixmap()
{
    return m_movie->currentPixmap();
}

void NoteContent::toLink(QUrl *url, QString *title, const QString &cuttedFullPath)
{
    if (useFile()) {
        *url = QUrl::fromUserInput(cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
        *title = (cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
    } else {
        *url = QUrl();
        title->clear();
    }
}
void LinkContent::toLink(QUrl *url, QString *title, const QString & /*cuttedFullPath*/)
{
    *url = this->url();
    *title = this->title();
}
void CrossReferenceContent::toLink(QUrl *url, QString *title, const QString & /*cuttedFullPath*/)
{
    *url = this->url();
    *title = this->title();
}

void LauncherContent::toLink(QUrl *url, QString *title, const QString &cuttedFullPath)
{
    *url = QUrl::fromUserInput(cuttedFullPath.isEmpty() ? fullPath() : cuttedFullPath);
    *title = name();
}
void UnknownContent::toLink(QUrl *url, QString *title, const QString & /*cuttedFullPath*/)
{
    *url = QUrl();
    *title = QString();
}

bool TextContent::useFile() const
{
    return true;
}
bool HtmlContent::useFile() const
{
    return true;
}
bool ImageContent::useFile() const
{
    return true;
}
bool AnimationContent::useFile() const
{
    return true;
}
bool SoundContent::useFile() const
{
    return true;
}
bool FileContent::useFile() const
{
    return true;
}
bool LinkContent::useFile() const
{
    return false;
}
bool CrossReferenceContent::useFile() const
{
    return false;
}
bool LauncherContent::useFile() const
{
    return true;
}
bool ColorContent::useFile() const
{
    return false;
}
bool UnknownContent::useFile() const
{
    return true;
}

bool TextContent::canBeSavedAs() const
{
    return true;
}
bool HtmlContent::canBeSavedAs() const
{
    return true;
}
bool ImageContent::canBeSavedAs() const
{
    return true;
}
bool AnimationContent::canBeSavedAs() const
{
    return true;
}
bool SoundContent::canBeSavedAs() const
{
    return true;
}
bool FileContent::canBeSavedAs() const
{
    return true;
}
bool LinkContent::canBeSavedAs() const
{
    return true;
}
bool CrossReferenceContent::canBeSavedAs() const
{
    return true;
}
bool LauncherContent::canBeSavedAs() const
{
    return true;
}
bool ColorContent::canBeSavedAs() const
{
    return false;
}
bool UnknownContent::canBeSavedAs() const
{
    return false;
}

QString TextContent::saveAsFilters() const
{
    return "text/plain";
}
QString HtmlContent::saveAsFilters() const
{
    return "text/html";
}
QString ImageContent::saveAsFilters() const
{
    return "image/png";
} // TODO: Offer more types
QString AnimationContent::saveAsFilters() const
{
    return "image/gif";
} // TODO: MNG...
QString SoundContent::saveAsFilters() const
{
    return "audio/mp3 audio/ogg";
} // TODO: OGG...
QString FileContent::saveAsFilters() const
{
    return "*";
} // TODO: Get MIME type of the url target
QString LinkContent::saveAsFilters() const
{
    return "*";
} // TODO: idem File + If isDir() const: return
QString CrossReferenceContent::saveAsFilters() const
{
    return "*";
} // TODO: idem File + If isDir() const: return
QString LauncherContent::saveAsFilters() const
{
    return "application/x-desktop";
}
QString ColorContent::saveAsFilters() const
{
    return QString();
}
QString UnknownContent::saveAsFilters() const
{
    return QString();
}

bool TextContent::match(const FilterData &data)
{
    return text().contains(data.string);
}
bool HtmlContent::match(const FilterData &data)
{
    return m_textEquivalent /*toText(QString())*/.contains(data.string);
} // OPTIM_FILTER
bool ImageContent::match(const FilterData & /*data*/)
{
    return false;
}
bool AnimationContent::match(const FilterData & /*data*/)
{
    return false;
}
bool SoundContent::match(const FilterData &data)
{
    return fileName().contains(data.string);
}
bool FileContent::match(const FilterData &data)
{
    return fileName().contains(data.string);
}
bool LinkContent::match(const FilterData &data)
{
    return title().contains(data.string) || url().toDisplayString().contains(data.string);
}
bool CrossReferenceContent::match(const FilterData &data)
{
    return title().contains(data.string) || url().toDisplayString().contains(data.string);
}
bool LauncherContent::match(const FilterData &data)
{
    return exec().contains(data.string) || name().contains(data.string);
}
bool ColorContent::match(const FilterData &data)
{
    return color().name().contains(data.string);
}
bool UnknownContent::match(const FilterData &data)
{
    return mimeTypes().contains(data.string);
}

QString TextContent::editToolTipText() const
{
    return i18n("Edit this plain text");
}
QString HtmlContent::editToolTipText() const
{
    return i18n("Edit this text");
}
QString ImageContent::editToolTipText() const
{
    return i18n("Edit this image");
}
QString AnimationContent::editToolTipText() const
{
    return i18n("Edit this animation");
}
QString SoundContent::editToolTipText() const
{
    return i18n("Edit the file name of this sound");
}
QString FileContent::editToolTipText() const
{
    return i18n("Edit the name of this file");
}
QString LinkContent::editToolTipText() const
{
    return i18n("Edit this link");
}
QString CrossReferenceContent::editToolTipText() const
{
    return i18n("Edit this cross reference");
}
QString LauncherContent::editToolTipText() const
{
    return i18n("Edit this launcher");
}
QString ColorContent::editToolTipText() const
{
    return i18n("Edit this color");
}
QString UnknownContent::editToolTipText() const
{
    return i18n("Edit this unknown object");
}

QString TextContent::cssClass() const
{
    return QString();
}
QString HtmlContent::cssClass() const
{
    return QString();
}
QString ImageContent::cssClass() const
{
    return QString();
}
QString AnimationContent::cssClass() const
{
    return QString();
}
QString SoundContent::cssClass() const
{
    return "sound";
}
QString FileContent::cssClass() const
{
    return "file";
}
QString LinkContent::cssClass() const
{
    return (LinkLook::lookForURL(m_url) == LinkLook::localLinkLook ? "local" : "network");
}
QString CrossReferenceContent::cssClass() const
{
    return "cross_reference";
}
QString LauncherContent::cssClass() const
{
    return "launcher";
}
QString ColorContent::cssClass() const
{
    return QString();
}
QString UnknownContent::cssClass() const
{
    return QString();
}

void TextContent::fontChanged()
{
    setText(text());
}
void HtmlContent::fontChanged()
{
    QTextDocument *richDoc = m_graphicsTextItem.document();
    // This check is important when applying style to a note which is not loaded yet. Example:
    // Filter all -> open some basket for the first time -> close filter: if a note was tagged as TODO, then it would display no text
    if (!richDoc->isEmpty())
        setHtml(Tools::textDocumentToMinimalHTML(richDoc));
}
void ImageContent::fontChanged()
{
    setPixmap(pixmap());
}
void AnimationContent::fontChanged()
{
    /*startMovie();*/
}
void FileContent::fontChanged()
{
    setFileName(fileName());
}
void LinkContent::fontChanged()
{
    setLink(url(), title(), icon(), autoTitle(), autoIcon());
}
void CrossReferenceContent::fontChanged()
{
    setCrossReference(url(), title(), icon());
}
void LauncherContent::fontChanged()
{
    setLauncher(name(), icon(), exec());
}
void ColorContent::fontChanged()
{
    setColor(color());
}
void UnknownContent::fontChanged()
{
    loadFromFile(/*lazyLoad=*/false);
} // TODO: Optimize: setMimeTypes()

// QString TextContent::customOpenCommand()      { return (Settings::isTextUseProg()      && ! Settings::textProg().isEmpty()      ? Settings::textProg()      : QString()); }
QString HtmlContent::customOpenCommand()
{
    return (Settings::isHtmlUseProg() && !Settings::htmlProg().isEmpty() ? Settings::htmlProg() : QString());
}
QString ImageContent::customOpenCommand()
{
    return (Settings::isImageUseProg() && !Settings::imageProg().isEmpty() ? Settings::imageProg() : QString());
}
QString AnimationContent::customOpenCommand()
{
    return (Settings::isAnimationUseProg() && !Settings::animationProg().isEmpty() ? Settings::animationProg() : QString());
}
QString SoundContent::customOpenCommand()
{
    return (Settings::isSoundUseProg() && !Settings::soundProg().isEmpty() ? Settings::soundProg() : QString());
}

void LinkContent::serialize(QDataStream &stream)
{
    stream << url() << title() << icon() << (quint64)autoTitle() << (quint64)autoIcon();
}
void CrossReferenceContent::serialize(QDataStream &stream)
{
    stream << url() << title() << icon();
}
void ColorContent::serialize(QDataStream &stream)
{
    stream << color();
}

QPixmap TextContent::feedbackPixmap(qreal width, qreal height)
{
    QRectF textRect = QFontMetrics(note()->font()).boundingRect(0, 0, width, height, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text());
    QPixmap pixmap(qMin(width, textRect.width()), qMin(height, textRect.height()));
    pixmap.fill(note()->backgroundColor().darker(FEEDBACK_DARKING));
    QPainter painter(&pixmap);
    painter.setPen(note()->textColor());
    painter.setFont(note()->font());
    painter.drawText(0, 0, pixmap.width(), pixmap.height(), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text());
    painter.end();

    return pixmap;
}

QPixmap HtmlContent::feedbackPixmap(qreal width, qreal height)
{
    QTextDocument richText;
    richText.setHtml(html());
    richText.setDefaultFont(note()->font());
    richText.setTextWidth(width);
    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::Text, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));
    QPixmap pixmap(qMin(width, richText.idealWidth()), qMin(height, richText.size().height()));
    pixmap.fill(note()->backgroundColor().darker(FEEDBACK_DARKING));
    QPainter painter(&pixmap);
    painter.setPen(note()->textColor());
    painter.translate(0, 0);
    richText.drawContents(&painter, QRectF(0, 0, pixmap.width(), pixmap.height()));
    painter.end();

    return pixmap;
}

QPixmap ImageContent::feedbackPixmap(qreal width, qreal height)
{
    if (width >= m_pixmapItem.pixmap().width() && height >= m_pixmapItem.pixmap().height()) { // Full size
        if (m_pixmapItem.pixmap().hasAlpha()) {
            QPixmap opaque(m_pixmapItem.pixmap().width(), m_pixmapItem.pixmap().height());
            opaque.fill(note()->backgroundColor().darker(FEEDBACK_DARKING));
            QPainter painter(&opaque);
            painter.drawPixmap(0, 0, m_pixmapItem.pixmap());
            painter.end();
            return opaque;
        } else {
            return m_pixmapItem.pixmap();
        }
    } else { // Scaled down
        QImage imageToScale = m_pixmapItem.pixmap().toImage();
        QPixmap pmScaled;
        pmScaled = QPixmap::fromImage(imageToScale.scaled(width, height, Qt::KeepAspectRatio));
        if (pmScaled.hasAlpha()) {
            QPixmap opaque(pmScaled.width(), pmScaled.height());
            opaque.fill(note()->backgroundColor().darker(FEEDBACK_DARKING));
            QPainter painter(&opaque);
            painter.drawPixmap(0, 0, pmScaled);
            painter.end();
            return opaque;
        } else {
            return pmScaled;
        }
    }
}

QPixmap AnimationContent::feedbackPixmap(qreal width, qreal height)
{
    QPixmap pixmap = m_movie->currentPixmap();
    if (width >= pixmap.width() && height >= pixmap.height()) // Full size
        return pixmap;
    else { // Scaled down
        QImage imageToScale = pixmap.toImage();
        QPixmap pmScaled;
        pmScaled = QPixmap::fromImage(imageToScale.scaled(width, height, Qt::KeepAspectRatio));
        return pmScaled;
    }
}

QPixmap LinkContent::feedbackPixmap(qreal width, qreal height)
{
    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));
    return m_linkDisplayItem.linkDisplay().feedbackPixmap(width, height, palette, /*isDefaultColor=*/note()->textColor() == basket()->textColor());
}

QPixmap CrossReferenceContent::feedbackPixmap(qreal width, qreal height)
{
    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));
    return m_linkDisplayItem.linkDisplay().feedbackPixmap(width, height, palette, /*isDefaultColor=*/note()->textColor() == basket()->textColor());
}

QPixmap ColorContent::feedbackPixmap(qreal width, qreal height)
{
    // TODO: Duplicate code: make a rect() method!
    QRectF boundingRect = m_colorItem.boundingRect();

    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));

    QPixmap pixmap(qMin(width, boundingRect.width()), qMin(height, boundingRect.height()));
    pixmap.fill(note()->backgroundColor().darker(FEEDBACK_DARKING));
    QPainter painter(&pixmap);
    m_colorItem.paint(&painter, nullptr, nullptr); //, pixmap.width(), pixmap.height(), palette, false, false, false); // We don't care of the three last boolean parameters.
    painter.end();

    return pixmap;
}

QPixmap FileContent::feedbackPixmap(qreal width, qreal height)
{
    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));
    return m_linkDisplayItem.linkDisplay().feedbackPixmap(width, height, palette, /*isDefaultColor=*/note()->textColor() == basket()->textColor());
}

QPixmap LauncherContent::feedbackPixmap(qreal width, qreal height)
{
    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));
    return m_linkDisplayItem.linkDisplay().feedbackPixmap(width, height, palette, /*isDefaultColor=*/note()->textColor() == basket()->textColor());
}

QPixmap UnknownContent::feedbackPixmap(qreal width, qreal height)
{
    QRectF boundingRect = m_unknownItem.boundingRect();

    QPalette palette;
    palette = basket()->palette();
    palette.setColor(QPalette::WindowText, note()->textColor());
    palette.setColor(QPalette::Background, note()->backgroundColor().darker(FEEDBACK_DARKING));

    QPixmap pixmap(qMin(width, boundingRect.width()), qMin(height, boundingRect.height()));
    QPainter painter(&pixmap);
    m_unknownItem.paint(&painter, nullptr, nullptr); //, pixmap.width() + 1, pixmap.height(), palette, false, false, false); // We don't care of the three last boolean parameters.
    painter.setPen(note()->backgroundColor().darker(FEEDBACK_DARKING));
    painter.drawPoint(0, 0);
    painter.drawPoint(pixmap.width() - 1, 0);
    painter.drawPoint(0, pixmap.height() - 1);
    painter.drawPoint(pixmap.width() - 1, pixmap.height() - 1);
    painter.end();

    return pixmap;
}

/** class TextContent:
 */

TextContent::TextContent(Note *parent, const QString &fileName, bool lazyLoad)
    : NoteContent(parent, NoteType::Text, fileName)
    , m_graphicsTextItem(parent)
{
    if (parent) {
        parent->addToGroup(&m_graphicsTextItem);
        m_graphicsTextItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }

    basket()->addWatchedFile(fullPath());
    loadFromFile(lazyLoad);
}

TextContent::~TextContent()
{
    if (note())
        note()->removeFromGroup(&m_graphicsTextItem);
}

qreal TextContent::setWidthAndGetHeight(qreal /*width*/)
{
    return m_graphicsTextItem.boundingRect().height();
}

bool TextContent::loadFromFile(bool lazyLoad)
{
    DEBUG_WIN << "Loading TextContent From " + basket()->folderName() + fileName();

    QString content;
    bool success = FileStorage::loadFromFile(fullPath(), &content);

    if (success)
        setText(content, lazyLoad);
    else {
        qDebug() << "FAILED TO LOAD TextContent: " << fullPath();
        setText(QString(), lazyLoad);
        if (!QFile::exists(fullPath()))
            saveToFile(); // Reserve the fileName so no new note will have the same name!
    }
    return success;
}

bool TextContent::finishLazyLoad()
{
    m_graphicsTextItem.setFont(note()->font());
    contentChanged(m_graphicsTextItem.boundingRect().width() + 1);
    return true;
}

bool TextContent::saveToFile()
{
    return FileStorage::saveToFile(fullPath(), text());
}

QString TextContent::linkAt(const QPointF & /*pos*/)
{
    return QString();
    /*    if (m_simpleRichText)
            return m_simpleRichText->documentLayout()->anchorAt(pos);
        else
            return QString(); // Lazy loaded*/
}

QString TextContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening plain text...");
    case OpenSeveral:
        return i18n("Opening plain texts...");
    case OpenOneWith:
        return i18n("Opening plain text with...");
    case OpenSeveralWith:
        return i18n("Opening plain texts with...");
    case OpenOneWithDialog:
        return i18n("Open plain text with:");
    case OpenSeveralWithDialog:
        return i18n("Open plain texts with:");
    default:
        return QString();
    }
}

void TextContent::setText(const QString &text, bool lazyLoad)
{
    m_graphicsTextItem.setText(text);
    if (!lazyLoad)
        finishLazyLoad();
    else
        contentChanged(m_graphicsTextItem.boundingRect().width());
}

void TextContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString spaces;
    QString html = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><meta name=\"qrichtext\" content=\"1\" /></head><body>" +
        Tools::tagCrossReferences(Tools::tagURLs(Tools::textToHTMLWithoutP(text().replace(QChar('\t'), "                "))), false, exporter); // Don't collapse multiple spaces!
    exporter->stream << html.replace("  ", " &nbsp;").replace(QChar('\n'), '\n' + spaces.fill(' ', indent + 1));
}

/** class HtmlContent:
 */

HtmlContent::HtmlContent(Note *parent, const QString &fileName, bool lazyLoad)
    : NoteContent(parent, NoteType::Html, fileName)
    , m_simpleRichText(nullptr)
    , m_graphicsTextItem(parent)
{
    if (parent) {
        parent->addToGroup(&m_graphicsTextItem);
        m_graphicsTextItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }
    basket()->addWatchedFile(fullPath());
    loadFromFile(lazyLoad);
}

HtmlContent::~HtmlContent()
{
    if (note())
        note()->removeFromGroup(&m_graphicsTextItem);

    delete m_simpleRichText;
}

qreal HtmlContent::setWidthAndGetHeight(qreal width)
{
    width -= 1;
    m_graphicsTextItem.setTextWidth(width);
    return m_graphicsTextItem.boundingRect().height();
}

bool HtmlContent::loadFromFile(bool lazyLoad)
{
    DEBUG_WIN << "Loading HtmlContent From " + basket()->folderName() + fileName();

    QString content;
    bool success = FileStorage::loadFromFile(fullPath(), &content);

    if (success)
        setHtml(content, lazyLoad);
    else {
        setHtml(QString(), lazyLoad);
        if (!QFile::exists(fullPath()))
            saveToFile(); // Reserve the fileName so no new note will have the same name!
    }
    return success;
}

bool HtmlContent::finishLazyLoad()
{
    qreal width = m_graphicsTextItem.document()->idealWidth();

    m_graphicsTextItem.setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    m_graphicsTextItem.setTextInteractionFlags(Qt::TextEditorInteraction);

    /*QString css = ".cross_reference { display: block; width: 100%; text-decoration: none; color: #336600; }"
       "a:hover.cross_reference { text-decoration: underline; color: #ff8000; }";
    m_graphicsTextItem.document()->setDefaultStyleSheet(css);*/
    QString convert = Tools::tagURLs(m_html);
    if (note()->allowCrossReferences())
        convert = Tools::tagCrossReferences(convert);
    m_graphicsTextItem.setHtml(convert);
    m_graphicsTextItem.setDefaultTextColor(note()->textColor());
    m_graphicsTextItem.setFont(note()->font());
    m_graphicsTextItem.setTextWidth(1); // We put a width of 1 pixel, so usedWidth() is equal to the minimum width
    int minWidth = m_graphicsTextItem.document()->idealWidth();
    m_graphicsTextItem.setTextWidth(width);
    contentChanged(minWidth + 1);

    return true;
}

bool HtmlContent::saveToFile()
{
    return FileStorage::saveToFile(fullPath(), html());
}

QString HtmlContent::linkAt(const QPointF &pos)
{
    return m_graphicsTextItem.document()->documentLayout()->anchorAt(pos);
}

QString HtmlContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening text...");
    case OpenSeveral:
        return i18n("Opening texts...");
    case OpenOneWith:
        return i18n("Opening text with...");
    case OpenSeveralWith:
        return i18n("Opening texts with...");
    case OpenOneWithDialog:
        return i18n("Open text with:");
    case OpenSeveralWithDialog:
        return i18n("Open texts with:");
    default:
        return QString();
    }
}

void HtmlContent::setHtml(const QString &html, bool lazyLoad)
{
    m_html = html;
    /* The code was commented, so now non-Latin text is stored directly in Unicode.
     * If testing doesn't show any bugs, this block should be deleted
    QRegExp rx("([^\\x00-\\x7f])");
    while (m_html.contains(rx)) {
        m_html.replace( rx.cap().unicode()[0], QString("&#%1;").arg(rx.cap().unicode()[0].unicode()) );
    }*/
    m_textEquivalent = toText(QString()); // OPTIM_FILTER
    if (!lazyLoad)
        finishLazyLoad();
    else
        contentChanged(10);
}

void HtmlContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString spaces;
    QString convert = Tools::tagURLs(html().replace("\t", "                "));
    if (note()->allowCrossReferences())
        convert = Tools::tagCrossReferences(convert, false, exporter);

    exporter->stream << Tools::htmlToParagraph(convert).replace("  ", " &nbsp;").replace("\n", '\n' + spaces.fill(' ', indent + 1));
}

/** class ImageContent:
 */

ImageContent::ImageContent(Note *parent, const QString &fileName, bool lazyLoad)
    : NoteContent(parent, NoteType::Image, fileName)
    , m_pixmapItem(parent)
    , m_format()
{
    if (parent) {
        parent->addToGroup(&m_pixmapItem);
        m_pixmapItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }

    basket()->addWatchedFile(fullPath());
    loadFromFile(lazyLoad);
}

ImageContent::~ImageContent()
{
    if (note())
        note()->removeFromGroup(&m_pixmapItem);
}

qreal ImageContent::setWidthAndGetHeight(qreal width)
{
    width -= 1;
    // Don't store width: we will get it on paint!
    if (width >= m_pixmapItem.pixmap().width()) // Full size
    {
        m_pixmapItem.setScale(1.0);
        return m_pixmapItem.boundingRect().height();
    } else { // Scaled down
        qreal scaleFactor = width / m_pixmapItem.pixmap().width();
        m_pixmapItem.setScale(scaleFactor);
        return m_pixmapItem.boundingRect().height() * scaleFactor;
    }
}

bool ImageContent::loadFromFile(bool lazyLoad)
{
    if (lazyLoad)
        return true;
    else
        return finishLazyLoad();
}

bool ImageContent::finishLazyLoad()
{
    DEBUG_WIN << "Loading ImageContent From " + basket()->folderName() + fileName();

    QByteArray content;
    QPixmap pixmap;

    if (FileStorage::loadFromFile(fullPath(), &content)) {
        QBuffer buffer(&content);

        buffer.open(QIODevice::ReadOnly);
        m_format = QImageReader::imageFormat(&buffer); // See QImageIO to know what formats can be supported.
        buffer.close();
        if (!m_format.isNull()) {
            pixmap.loadFromData(content);
            setPixmap(pixmap);
            return true;
        }
    }

    qDebug() << "FAILED TO LOAD ImageContent: " << fullPath();
    m_format = "PNG";       // If the image is set later, it should be saved without destruction, so we use PNG by default.
    pixmap = QPixmap(1, 1); // Create a 1x1 pixels image instead of an undefined one.
    pixmap.fill();
    pixmap.setMask(pixmap.createHeuristicMask());
    setPixmap(pixmap);
    if (!QFile::exists(fullPath()))
        saveToFile(); // Reserve the fileName so no new note will have the same name!
    return false;
}

bool ImageContent::saveToFile()
{
    QByteArray ba;
    QBuffer buffer(&ba);

    buffer.open(QIODevice::WriteOnly);
    m_pixmapItem.pixmap().save(&buffer, m_format);
    return FileStorage::saveToFile(fullPath(), ba);
}

QMap<QString, QString> ImageContent::toolTipInfos()
{
    return {
        {i18n("Size"), i18n("%1 by %2 pixels", QString::number(m_pixmapItem.pixmap().width()),
                                               QString::number(m_pixmapItem.pixmap().height()))}
    };
}

QString ImageContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening image...");
    case OpenSeveral:
        return i18n("Opening images...");
    case OpenOneWith:
        return i18n("Opening image with...");
    case OpenSeveralWith:
        return i18n("Opening images with...");
    case OpenOneWithDialog:
        return i18n("Open image with:");
    case OpenSeveralWithDialog:
        return i18n("Open images with:");
    default:
        return QString();
    }
}

void ImageContent::setPixmap(const QPixmap &pixmap)
{
    m_pixmapItem.setPixmap(pixmap);
    // Since it's scaled, the height is always greater or equal to the size of the tag emblems (16)
    contentChanged(16 + 1); // TODO: always good? I don't think...
}

void ImageContent::exportToHTML(HTMLExporter *exporter, int /*indent*/)
{
    qreal width = m_pixmapItem.pixmap().width();
    qreal height = m_pixmapItem.pixmap().height();
    qreal contentWidth = note()->width() - note()->contentX() - 1 - Note::NOTE_MARGIN;

    QString imageName = exporter->copyFile(fullPath(), /*createIt=*/true);

    if (contentWidth <= m_pixmapItem.pixmap().width()) { // Scaled down
        qreal scale = contentWidth / m_pixmapItem.pixmap().width();
        width = m_pixmapItem.pixmap().width() * scale;
        height = m_pixmapItem.pixmap().height() * scale;
        exporter->stream << "<a href=\"" << exporter->dataFolderName << imageName << "\" title=\"" << i18n("Click for full size view") << "\">";
    }

    exporter->stream << "<img src=\"" << exporter->dataFolderName << imageName << "\" width=\"" << width << "\" height=\"" << height << "\" alt=\"\">";

    if (contentWidth <= m_pixmapItem.pixmap().width()) // Scaled down
        exporter->stream << "</a>";
}

/** class AnimationContent:
 */

AnimationContent::AnimationContent(Note *parent, const QString &fileName, bool lazyLoad)
    : NoteContent(parent, NoteType::Animation, fileName)
    , m_buffer(new QBuffer(this))
    , m_movie(new QMovie(this))
    , m_currentWidth(0)
    , m_graphicsPixmap(parent)
{
    if (parent) {
        parent->addToGroup(&m_graphicsPixmap);
        m_graphicsPixmap.setPos(parent->contentX(), Note::NOTE_MARGIN);
        connect(parent->basket(), SIGNAL(activated()), m_movie, SLOT(start()));
        connect(parent->basket(), SIGNAL(closed()), m_movie, SLOT(stop()));
    }

    basket()->addWatchedFile(fullPath());
    connect(m_movie, SIGNAL(resized(QSize)), this, SLOT(movieResized()));
    connect(m_movie, SIGNAL(frameChanged(int)), this, SLOT(movieFrameChanged()));

    loadFromFile(lazyLoad);
}

AnimationContent::~AnimationContent()
{
    note()->removeFromGroup(&m_graphicsPixmap);
}

qreal AnimationContent::setWidthAndGetHeight(qreal width)
{
    m_currentWidth = width;
    QPixmap pixmap = m_graphicsPixmap.pixmap();
    if (pixmap.width() > m_currentWidth) {
        qreal scaleFactor = m_currentWidth / pixmap.width();
        m_graphicsPixmap.setScale(scaleFactor);
        return pixmap.height() * scaleFactor;
    } else {
        m_graphicsPixmap.setScale(1.0);
        return pixmap.height();
    }

    return 0;
}

bool AnimationContent::loadFromFile(bool lazyLoad)
{
    if (lazyLoad)
        return true;
    else
        return finishLazyLoad();
}

bool AnimationContent::finishLazyLoad()
{
    QByteArray content;
    if (FileStorage::loadFromFile(fullPath(), &content)) {
        m_buffer->setData(content);
        startMovie();
        contentChanged(16);
        return true;
    }
    m_buffer->setData(nullptr);
    return false;
}

bool AnimationContent::saveToFile()
{
    // Impossible!
    return false;
}

QString AnimationContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening animation...");
    case OpenSeveral:
        return i18n("Opening animations...");
    case OpenOneWith:
        return i18n("Opening animation with...");
    case OpenSeveralWith:
        return i18n("Opening animations with...");
    case OpenOneWithDialog:
        return i18n("Open animation with:");
    case OpenSeveralWithDialog:
        return i18n("Open animations with:");
    default:
        return QString();
    }
}

bool AnimationContent::startMovie()
{
    if (m_buffer->data().isEmpty())
        return false;
    m_movie->setDevice(m_buffer);
    m_movie->start();
    return true;
}

void AnimationContent::movieUpdated()
{
    m_graphicsPixmap.setPixmap(m_movie->currentPixmap());
}

void AnimationContent::movieResized()
{
    m_graphicsPixmap.setPixmap(m_movie->currentPixmap());
}

void AnimationContent::movieFrameChanged()
{
    m_graphicsPixmap.setPixmap(m_movie->currentPixmap());
}

void AnimationContent::exportToHTML(HTMLExporter *exporter, int /*indent*/)
{
    exporter->stream << QString("<img src=\"%1\" width=\"%2\" height=\"%3\" alt=\"\">")
                            .arg(exporter->dataFolderName + exporter->copyFile(fullPath(), /*createIt=*/true), QString::number(m_movie->currentPixmap().size().width()), QString::number(m_movie->currentPixmap().size().height()));
}

/** class FileContent:
 */

FileContent::FileContent(Note *parent, const QString &fileName)
    : NoteContent(parent, NoteType::File, fileName)
    , m_linkDisplayItem(parent)
    , m_previewJob(nullptr)
{
    basket()->addWatchedFile(fullPath());
    setFileName(fileName); // FIXME: TO THAT HERE BECAUSE NoteContent() constructor seems to don't be able to call virtual methods???
    if (parent) {
        parent->addToGroup(&m_linkDisplayItem);
        m_linkDisplayItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }
}

FileContent::~FileContent()
{
    if (note())
        note()->removeFromGroup(&m_linkDisplayItem);
}

qreal FileContent::setWidthAndGetHeight(qreal width)
{
    m_linkDisplayItem.linkDisplay().setWidth(width);
    return m_linkDisplayItem.linkDisplay().height();
}

bool FileContent::loadFromFile(bool /*lazyLoad*/)
{
    setFileName(fileName()); // File changed: get new file preview!
    return true;
}

QMap<QString, QString> FileContent::toolTipInfos()
{
    QMap<QString, QString> toolTip;

    // Get the size of the file:
    uint size = QFileInfo(fullPath()).size();
    QString humanFileSize = KIO::convertSize((KIO::filesize_t)size);
    toolTip.insert(i18n("Size"), humanFileSize);

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForUrl(QUrl::fromLocalFile(fullPath()));
    if (mime.isValid()) {
        toolTip.insert(i18n("Type"), mime.comment());
    }

    MetaDataExtractionResult result(fullPath(), mime.name());

    KFileMetaData::ExtractorCollection extractorCollection;
    const QList<KFileMetaData::Extractor*> exList = extractorCollection.fetchExtractors(mime.name());
    for (KFileMetaData::Extractor *ex : exList) {
        ex->extract(&result);
        const auto groups = result.preferredGroups();
        DEBUG_WIN << "Metadata Extractor result has " << QString::number(groups.count()) << " groups";

        for (const auto &group : groups) {
            if (!group.second.isEmpty()) {
                toolTip.insert(group.first, group.second);
            }
        }
    }

    return toolTip;
}

int FileContent::zoneAt(const QPointF &pos)
{
    return (m_linkDisplayItem.linkDisplay().iconButtonAt(pos) ? 0 : Note::Custom0);
}

QRectF FileContent::zoneRect(int zone, const QPointF & /*pos*/)
{
    QRectF linkRect = m_linkDisplayItem.linkDisplay().iconButtonRect();

    if (zone == Note::Custom0)
        return QRectF(linkRect.width(), 0, note()->width(), note()->height()); // Too wide and height, but it will be clipped by Note::zoneRect()
    else if (zone == Note::Content)
        return linkRect;
    else
        return QRectF();
}

QString FileContent::zoneTip(int zone)
{
    return (zone == Note::Custom0 ? i18n("Open this file") : QString());
}

Qt::CursorShape FileContent::cursorFromZone(int zone) const
{
    if (zone == Note::Custom0)
        return Qt::PointingHandCursor;
    return Qt::ArrowCursor;
}

int FileContent::xEditorIndent()
{
    return m_linkDisplayItem.linkDisplay().iconButtonRect().width() + 2;
}

QString FileContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening file...");
    case OpenSeveral:
        return i18n("Opening files...");
    case OpenOneWith:
        return i18n("Opening file with...");
    case OpenSeveralWith:
        return i18n("Opening files with...");
    case OpenOneWithDialog:
        return i18n("Open file with:");
    case OpenSeveralWithDialog:
        return i18n("Open files with:");
    default:
        return QString();
    }
}

void FileContent::setFileName(const QString &fileName)
{
    NoteContent::setFileName(fileName);
    QUrl url = QUrl::fromLocalFile(fullPath());
    if (linkLook()->previewEnabled())
        m_linkDisplayItem.linkDisplay().setLink(fileName, NoteFactory::iconForURL(url), linkLook(), note()->font()); // FIXME: move iconForURL outside of NoteFactory !!!!!
    else
        m_linkDisplayItem.linkDisplay().setLink(fileName, NoteFactory::iconForURL(url), QPixmap(), linkLook(), note()->font());
    startFetchingUrlPreview();
    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void FileContent::linkLookChanged()
{
    fontChanged();
    // setFileName(fileName());
    // startFetchingUrlPreview();
}

void FileContent::newPreview(const KFileItem &, const QPixmap &preview)
{
    LinkLook *linkLook = this->linkLook();
    m_linkDisplayItem.linkDisplay().setLink(fileName(), NoteFactory::iconForURL(QUrl::fromLocalFile(fullPath())), (linkLook->previewEnabled() ? preview : QPixmap()), linkLook, note()->font());
    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void FileContent::removePreview(const KFileItem &ki)
{
    newPreview(ki, QPixmap());
}

void FileContent::startFetchingUrlPreview()
{
    /*
    KUrl url(fullPath());
    LinkLook *linkLook = this->linkLook();

//  delete m_previewJob;
    if (!url.isEmpty() && linkLook->previewSize() > 0) {
        QUrl filteredUrl = NoteFactory::filteredURL(url);//KURIFilter::self()->filteredURI(url);
        KUrl::List urlList;
        urlList.append(filteredUrl);
        m_previewJob = KIO::filePreview(urlList, linkLook->previewSize(), linkLook->previewSize(), linkLook->iconSize());
        connect(m_previewJob, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)), this, SLOT(newPreview(const KFileItem&, const QPixmap&)));
        connect(m_previewJob, SIGNAL(failed(const KFileItem&)),                     this, SLOT(removePreview(const KFileItem&)));
    }
    */
}

void FileContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString spaces;
    QString fileName = exporter->copyFile(fullPath(), true);
    exporter->stream << m_linkDisplayItem.linkDisplay().toHtml(exporter, QUrl::fromLocalFile(exporter->dataFolderName + fileName), QString()).replace("\n", '\n' + spaces.fill(' ', indent + 1));
}

/** class SoundContent:
 */

SoundContent::SoundContent(Note *parent, const QString &fileName)
    : FileContent(parent, fileName)
{
    setFileName(fileName);
    music = new Phonon::MediaObject(this);
    music->setCurrentSource(Phonon::MediaSource(fullPath()));
    Phonon::AudioOutput *audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
    Phonon::Path path = Phonon::createPath(music, audioOutput);
    connect(music, SIGNAL(stateChanged(Phonon::State, Phonon::State)), this, SLOT(stateChanged(Phonon::State, Phonon::State)));
}

void SoundContent::stateChanged(Phonon::State newState, Phonon::State oldState)
{
    qDebug() << "stateChanged " << oldState << " to " << newState;
}

QString SoundContent::zoneTip(int zone)
{
    return (zone == Note::Custom0 ? i18n("Open this sound") : QString());
}

void SoundContent::setHoveredZone(int oldZone, int newZone)
{
    if (newZone == Note::Custom0 || newZone == Note::Content) {
        // Start the sound preview:
        if (oldZone != Note::Custom0 && oldZone != Note::Content) { // Don't restart if it was already in one of those zones
            if (music->state() == 1) {
                music->play();
            }
        }
    } else {
        //       Stop the sound preview, if it was started:
        if (music->state() != 1) {
            music->stop();
            //          delete music;//TODO implement this in slot connected with music alted signal
            //          music = 0;
        }
    }
}

QString SoundContent::messageWhenOpening(OpenMessage where)
{
    switch (where) {
    case OpenOne:
        return i18n("Opening sound...");
    case OpenSeveral:
        return i18n("Opening sounds...");
    case OpenOneWith:
        return i18n("Opening sound with...");
    case OpenSeveralWith:
        return i18n("Opening sounds with...");
    case OpenOneWithDialog:
        return i18n("Open sound with:");
    case OpenSeveralWithDialog:
        return i18n("Open sounds with:");
    default:
        return QString();
    }
}

/** class LinkContent:
 */

LinkContent::LinkContent(Note *parent, const QUrl &url, const QString &title, const QString &icon, bool autoTitle, bool autoIcon)
    : NoteContent(parent, NoteType::Link)
    , m_linkDisplayItem(parent)
    , m_access_manager(nullptr)
    , m_acceptingData(false)
    , m_previewJob(nullptr)
{
    setLink(url, title, icon, autoTitle, autoIcon);
    if (parent) {
        parent->addToGroup(&m_linkDisplayItem);
        m_linkDisplayItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }
}
LinkContent::~LinkContent()
{
    if (note())
        note()->removeFromGroup(&m_linkDisplayItem);
    delete m_access_manager;
}

qreal LinkContent::setWidthAndGetHeight(qreal width)
{
    m_linkDisplayItem.linkDisplay().setWidth(width);
    return m_linkDisplayItem.linkDisplay().height();
}

void LinkContent::saveToNode(QXmlStreamWriter &stream)
{
    stream.writeStartElement("content");
    stream.writeAttribute("title", title());
    stream.writeAttribute("icon", icon());
    stream.writeAttribute("autoIcon", (autoIcon() ? "true" : "false"));
    stream.writeAttribute("autoTitle", (autoTitle() ? "true" : "false"));
    stream.writeCharacters(url().toDisplayString());
    stream.writeEndElement();
}

QMap<QString, QString> LinkContent::toolTipInfos()
{
    return {
        { i18n("Target"), m_url.toDisplayString() }
    };
}

int LinkContent::zoneAt(const QPointF &pos)
{
    return (m_linkDisplayItem.linkDisplay().iconButtonAt(pos) ? 0 : Note::Custom0);
}

QRectF LinkContent::zoneRect(int zone, const QPointF & /*pos*/)
{
    QRectF linkRect = m_linkDisplayItem.linkDisplay().iconButtonRect();

    if (zone == Note::Custom0)
        return QRectF(linkRect.width(), 0, note()->width(), note()->height()); // Too wide and height, but it will be clipped by Note::zoneRect()
    else if (zone == Note::Content)
        return linkRect;
    else
        return QRectF();
}

QString LinkContent::zoneTip(int zone)
{
    return (zone == Note::Custom0 ? i18n("Open this link") : QString());
}

Qt::CursorShape LinkContent::cursorFromZone(int zone) const
{
    if (zone == Note::Custom0)
        return Qt::PointingHandCursor;
    return Qt::ArrowCursor;
}

QString LinkContent::statusBarMessage(int zone)
{
    if (zone == Note::Custom0 || zone == Note::Content)
        return m_url.toDisplayString();
    else
        return QString();
}

QUrl LinkContent::urlToOpen(bool /*with*/)
{
    return NoteFactory::filteredURL(url()); // KURIFilter::self()->filteredURI(url());
}

QString LinkContent::messageWhenOpening(OpenMessage where)
{
    if (url().isEmpty())
        return i18n("Link have no URL to open.");

    switch (where) {
    case OpenOne:
        return i18n("Opening link target...");
    case OpenSeveral:
        return i18n("Opening link targets...");
    case OpenOneWith:
        return i18n("Opening link target with...");
    case OpenSeveralWith:
        return i18n("Opening link targets with...");
    case OpenOneWithDialog:
        return i18n("Open link target with:");
    case OpenSeveralWithDialog:
        return i18n("Open link targets with:");
    default:
        return QString();
    }
}

void LinkContent::setLink(const QUrl &url, const QString &title, const QString &icon, bool autoTitle, bool autoIcon)
{
    m_autoTitle = autoTitle;
    m_autoIcon = autoIcon;
    m_url = NoteFactory::filteredURL(url); // KURIFilter::self()->filteredURI(url);
    m_title = (autoTitle ? NoteFactory::titleForURL(m_url) : title);
    m_icon = (autoIcon ? NoteFactory::iconForURL(m_url) : icon);

    LinkLook *look = LinkLook::lookForURL(m_url);
    if (look->previewEnabled())
        m_linkDisplayItem.linkDisplay().setLink(m_title, m_icon, look, note()->font());
    else
        m_linkDisplayItem.linkDisplay().setLink(m_title, m_icon, QPixmap(), look, note()->font());
    startFetchingUrlPreview();
    if (autoTitle)
        startFetchingLinkTitle();
    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void LinkContent::linkLookChanged()
{
    fontChanged();
}

void LinkContent::newPreview(const KFileItem &, const QPixmap &preview)
{
    LinkLook *linkLook = LinkLook::lookForURL(url());
    m_linkDisplayItem.linkDisplay().setLink(title(), icon(), (linkLook->previewEnabled() ? preview : QPixmap()), linkLook, note()->font());
    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void LinkContent::removePreview(const KFileItem &ki)
{
    newPreview(ki, QPixmap());
}

// QHttp slots for getting link title
void LinkContent::httpReadyRead()
{
    if (!m_acceptingData)
        return;

    // Check for availability
    qint64 bytesAvailable = m_reply->bytesAvailable();
    if (bytesAvailable <= 0)
        return;

    QByteArray buf = m_reply->read(bytesAvailable);
    m_httpBuff.append(buf);

    // Stop at 10k bytes
    if (m_httpBuff.length() > 10000) {
        m_acceptingData = false;
        m_reply->abort();
        endFetchingLinkTitle();
    }
}

void LinkContent::httpDone(QNetworkReply *reply)
{
    if (m_acceptingData) {
        m_acceptingData = false;
        endFetchingLinkTitle();
    }

    // If all done, close and delete the reply.
    reply->deleteLater();
}

void LinkContent::startFetchingLinkTitle()
{
    QUrl newUrl = this->url();

    // If this is not an HTTP request, just ignore it.
    if (newUrl.scheme() == "http") {
        // If we have no access_manager, create one.
        if (m_access_manager == nullptr) {
            m_access_manager = new KIO::Integration::AccessManager(this);
            connect(m_access_manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(httpDone(QNetworkReply *)));
        }

        // If no explicit port, default to port 80.
        if (newUrl.port() == 0)
            newUrl.setPort(80);

        // If no path or query part, default to /
        if ((newUrl.path() + newUrl.query()).isEmpty())
            newUrl = QUrl::fromLocalFile("/");

        // Issue request
        m_reply = m_access_manager->get(QNetworkRequest(newUrl));
        m_acceptingData = true;
        connect(m_reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
    }
}

// Code duplicated from FileContent::startFetchingUrlPreview()
void LinkContent::startFetchingUrlPreview()
{
    QUrl url = this->url();
    LinkLook *linkLook = LinkLook::lookForURL(this->url());

    //  delete m_previewJob;
    if (!url.isEmpty() && linkLook->previewSize() > 0) {
        QUrl filteredUrl = NoteFactory::filteredURL(url); // KURIFilter::self()->filteredURI(url);
        QList<QUrl> urlList;
        urlList.append(filteredUrl);
        m_previewJob = KIO::filePreview(urlList, linkLook->previewSize(), linkLook->previewSize(), linkLook->iconSize());
        connect(m_previewJob, SIGNAL(gotPreview(const KFileItem &, const QPixmap &)), this, SLOT(newPreview(const KFileItem &, const QPixmap &)));
        connect(m_previewJob, SIGNAL(failed(const KFileItem &)), this, SLOT(removePreview(const KFileItem &)));
    }
}

void LinkContent::endFetchingLinkTitle()
{
    if (m_httpBuff.length() > 0) {
        decodeHtmlTitle();
        m_httpBuff.clear();
    } else
        DEBUG_WIN << "LinkContent: empty buffer on endFetchingLinkTitle for " + m_url.toString();
}

void LinkContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString linkTitle = title();

    // TODO:
    //  // Append address (useful for print version of the page/basket):
    //  if (exportData.formatForImpression && (!autoTitle() && title() != NoteFactory::titleForURL(url().toDisplayString()))) {
    //      // The address is on a new line, unless title is empty (empty lines was replaced by &nbsp;):
    //      if (linkTitle == " "/*"&nbsp;"*/)
    //          linkTitle = url().toDisplayString()/*QString()*/;
    //      else
    //          linkTitle = linkTitle + " <" + url().toDisplayString() + ">"/*+ "<br>"*/;
    //      //linkTitle += "<i>" + url().toDisplayString() + "</i>";
    //  }

    QUrl linkURL;
    /*
        QFileInfo fInfo(url().path());
    //  DEBUG_WIN << url().path()
    //            << "IsFile:" + QString::number(fInfo.isFile())
    //            << "IsDir:"  + QString::number(fInfo.isDir());
        if (exportData.embedLinkedFiles && fInfo.isFile()) {
    //      DEBUG_WIN << "Embed file";
            linkURL = exportData.dataFolderName + BasketScene::copyFile(url().path(), exportData.dataFolderPath, true);
        } else if (exportData.embedLinkedFolders && fInfo.isDir()) {
    //      DEBUG_WIN << "Embed folder";
            linkURL = exportData.dataFolderName + BasketScene::copyFile(url().path(), exportData.dataFolderPath, true);
        } else {
    //      DEBUG_WIN << "Embed LINK";
    */
    linkURL = url();
    /*
        }
    */

    QString spaces;
    exporter->stream << m_linkDisplayItem.linkDisplay().toHtml(exporter, linkURL, linkTitle).replace("\n", '\n' + spaces.fill(' ', indent + 1));
}

/** class CrossReferenceContent:
 */

CrossReferenceContent::CrossReferenceContent(Note *parent, const QUrl &url, const QString &title, const QString &icon)
    : NoteContent(parent, NoteType::CrossReference)
    , m_linkDisplayItem(parent)
{
    this->setCrossReference(url, title, icon);
    if (parent)
        parent->addToGroup(&m_linkDisplayItem);
}

CrossReferenceContent::~CrossReferenceContent()
{
    if (note())
        note()->removeFromGroup(&m_linkDisplayItem);
}

qreal CrossReferenceContent::setWidthAndGetHeight(qreal width)
{
    m_linkDisplayItem.linkDisplay().setWidth(width);
    return m_linkDisplayItem.linkDisplay().height();
}

void CrossReferenceContent::saveToNode(QXmlStreamWriter &stream)
{
    stream.writeStartElement("content");
    stream.writeAttribute("title", title());
    stream.writeAttribute("icon", icon());
    stream.writeCharacters(url().toDisplayString());
    stream.writeEndElement();
}

QMap<QString, QString> CrossReferenceContent::toolTipInfos()
{
    return {
        { i18n("Target"), m_url.toDisplayString() }
    };
}

int CrossReferenceContent::zoneAt(const QPointF &pos)
{
    return (m_linkDisplayItem.linkDisplay().iconButtonAt(pos) ? 0 : Note::Custom0);
}

QRectF CrossReferenceContent::zoneRect(int zone, const QPointF & /*pos*/)
{
    QRectF linkRect = m_linkDisplayItem.linkDisplay().iconButtonRect();

    if (zone == Note::Custom0)
        return QRectF(linkRect.width(), 0, note()->width(), note()->height()); // Too wide and height, but it will be clipped by Note::zoneRect()
    else if (zone == Note::Content)
        return linkRect;
    else
        return QRectF();
}

QString CrossReferenceContent::zoneTip(int zone)
{
    return (zone == Note::Custom0 ? i18n("Open this link") : QString());
}

Qt::CursorShape CrossReferenceContent::cursorFromZone(int zone) const
{
    if (zone == Note::Custom0)
        return Qt::PointingHandCursor;
    return Qt::ArrowCursor;
}

QString CrossReferenceContent::statusBarMessage(int zone)
{
    if (zone == Note::Custom0 || zone == Note::Content)
        return i18n("Link to %1", this->title());
    else
        return QString();
}

QUrl CrossReferenceContent::urlToOpen(bool /*with*/)
{
    return m_url;
}

QString CrossReferenceContent::messageWhenOpening(OpenMessage where)
{
    if (url().isEmpty())
        return i18n("Link has no basket to open.");

    switch (where) {
    case OpenOne:
        return i18n("Opening basket...");
    default:
        return QString();
    }
}

void CrossReferenceContent::setLink(const QUrl &url, const QString &title, const QString &icon)
{
    this->setCrossReference(url, title, icon);
}

void CrossReferenceContent::setCrossReference(const QUrl &url, const QString &title, const QString &icon)
{
    m_url = url;
    m_title = (title.isEmpty() ? url.url() : title);
    m_icon = icon;

    LinkLook *look = LinkLook::crossReferenceLook;
    m_linkDisplayItem.linkDisplay().setLink(m_title, m_icon, look, note()->font());

    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void CrossReferenceContent::linkLookChanged()
{
    fontChanged();
}

void CrossReferenceContent::exportToHTML(HTMLExporter *exporter, int /*indent*/)
{
    QString url = m_url.url();
    QString title;

    if (url.startsWith(QLatin1String("basket://")))
        url = url.mid(9, url.length() - 9);
    if (url.endsWith('/'))
        url = url.left(url.length() - 1);

    BasketScene *basket = Global::bnpView->basketForFolderName(url);

    if (!basket)
        title = "unknown basket";
    else
        title = basket->basketName();

    // if the basket we're trying to link to is the basket that was exported then
    // we have to use a special way to refer to it for the links.
    if (basket == exporter->exportedBasket)
        url = "../../" + exporter->fileName;
    else {
        // if we're in the exported basket then the links have to include
        // the sub directories.
        if (exporter->currentBasket == exporter->exportedBasket)
            url.prepend(exporter->basketsFolderName);
        url.append(".html");
    }

    QString linkIcon = exporter->iconsFolderName + exporter->copyIcon(m_icon, LinkLook::crossReferenceLook->iconSize());
    linkIcon = QString("<img src=\"%1\" alt=\"\">").arg(linkIcon);

    exporter->stream << QString("<a href=\"%1\">%2 %3</a>").arg(url, linkIcon, title);
}

/** class LauncherContent:
 */

LauncherContent::LauncherContent(Note *parent, const QString &fileName)
    : NoteContent(parent, NoteType::Launcher, fileName)
    , m_linkDisplayItem(parent)
{
    basket()->addWatchedFile(fullPath());
    loadFromFile(/*lazyLoad=*/false);
    if (parent) {
        parent->addToGroup(&m_linkDisplayItem);
        m_linkDisplayItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }
}

LauncherContent::~LauncherContent()
{
    if (note())
        note()->removeFromGroup(&m_linkDisplayItem);
}

qreal LauncherContent::setWidthAndGetHeight(qreal width)
{
    m_linkDisplayItem.linkDisplay().setWidth(width);
    return m_linkDisplayItem.linkDisplay().height();
}

bool LauncherContent::loadFromFile(bool /*lazyLoad*/) // TODO: saveToFile() ?? Is it possible?
{
    DEBUG_WIN << "Loading LauncherContent From " + basket()->folderName() + fileName();
    KService service(fullPath());
    setLauncher(service.name(), service.icon(), service.exec());
    return true;
}

QMap<QString, QString> LauncherContent::toolTipInfos()
{
    QMap<QString, QString> toolTip;
    KService service(fullPath());

    QString exec = service.exec();
    if (service.terminal())
        exec = i18n("%1 <i>(run in terminal)</i>", exec);

    if (!service.comment().isEmpty() && service.comment() != service.name()) {
        toolTip.insert(i18n("Comment"), service.comment());
    }
    toolTip.insert(i18n("Command"), exec);

    return toolTip;
}

int LauncherContent::zoneAt(const QPointF &pos)
{
    return (m_linkDisplayItem.linkDisplay().iconButtonAt(pos) ? 0 : Note::Custom0);
}

QRectF LauncherContent::zoneRect(int zone, const QPointF & /*pos*/)
{
    QRectF linkRect = m_linkDisplayItem.linkDisplay().iconButtonRect();

    if (zone == Note::Custom0)
        return QRectF(linkRect.width(), 0, note()->width(), note()->height()); // Too wide and height, but it will be clipped by Note::zoneRect()
    else if (zone == Note::Content)
        return linkRect;
    else
        return QRectF();
}

QString LauncherContent::zoneTip(int zone)
{
    return (zone == Note::Custom0 ? i18n("Launch this application") : QString());
}

Qt::CursorShape LauncherContent::cursorFromZone(int zone) const
{
    if (zone == Note::Custom0)
        return Qt::PointingHandCursor;
    return Qt::ArrowCursor;
}

QUrl LauncherContent::urlToOpen(bool with)
{
    if (KService(fullPath()).exec().isEmpty())
        return QUrl();

    return (with ? QUrl() : QUrl::fromLocalFile(fullPath())); // Can open the application, but not with another application :-)
}

QString LauncherContent::messageWhenOpening(OpenMessage where)
{
    if (KService(fullPath()).exec().isEmpty())
        return i18n("The launcher have no command to run.");

    switch (where) {
    case OpenOne:
        return i18n("Launching application...");
    case OpenSeveral:
        return i18n("Launching applications...");
    case OpenOneWith:
    case OpenSeveralWith:
    case OpenOneWithDialog:
    case OpenSeveralWithDialog: // TODO: "Open this application with this file as parameter"?
    default:
        return QString();
    }
}

void LauncherContent::setLauncher(const QString &name, const QString &icon, const QString &exec)
{
    m_name = name;
    m_icon = icon;
    m_exec = exec;

    m_linkDisplayItem.linkDisplay().setLink(name, icon, LinkLook::launcherLook, note()->font());
    contentChanged(m_linkDisplayItem.linkDisplay().minWidth());
}

void LauncherContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString spaces;
    QString fileName = exporter->copyFile(fullPath(), /*createIt=*/true);
    exporter->stream << m_linkDisplayItem.linkDisplay().toHtml(exporter, QUrl::fromLocalFile(exporter->dataFolderName + fileName), QString()).replace("\n", '\n' + spaces.fill(' ', indent + 1));
}

/** class ColorItem:
 */
const int ColorItem::RECT_MARGIN = 2;

ColorItem::ColorItem(Note *parent, const QColor &color)
    : QGraphicsItem(parent)
    , m_note(parent)
{
    setColor(color);
}

void ColorItem::setColor(const QColor &color)
{
    m_color = color;
    m_textRect = QFontMetrics(m_note->font()).boundingRect(m_color.name());
}

QRectF ColorItem::boundingRect() const
{
    qreal rectHeight = (m_textRect.height() + 2) * 3 / 2;
    qreal rectWidth = rectHeight * 14 / 10; // 1.4 times the height, like A4 papers.
    return QRectF(0, 0, rectWidth + RECT_MARGIN + m_textRect.width() + RECT_MARGIN, rectHeight);
}

void ColorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF boundingRect = this->boundingRect();
    qreal rectHeight = (m_textRect.height() + 2) * 3 / 2;
    qreal rectWidth = rectHeight * 14 / 10; // 1.4 times the height, like A4 papers.

    // FIXME: Duplicate from CommonColorSelector::drawColorRect:
    // Fill:
    painter->fillRect(1, 1, rectWidth - 2, rectHeight - 2, color());
    // Stroke:
    QColor stroke = color().darker(125);
    painter->setPen(stroke);
    painter->drawLine(1, 0, rectWidth - 2, 0);
    painter->drawLine(0, 1, 0, rectHeight - 2);
    painter->drawLine(1, rectHeight - 1, rectWidth - 2, rectHeight - 1);
    painter->drawLine(rectWidth - 1, 1, rectWidth - 1, rectHeight - 2);
    // Round corners:
    painter->setPen(Tools::mixColor(color(), stroke));
    painter->drawPoint(1, 1);
    painter->drawPoint(1, rectHeight - 2);
    painter->drawPoint(rectWidth - 2, rectHeight - 2);
    painter->drawPoint(rectWidth - 2, 1);

    // Draw the text:
    painter->setFont(m_note->font());
    painter->setPen(m_note->palette().color(QPalette::Active, QPalette::WindowText));
    painter->drawText(rectWidth + RECT_MARGIN, 0, m_textRect.width(), boundingRect.height(), Qt::AlignLeft | Qt::AlignVCenter, color().name());
}

/** class ColorContent:
 */

ColorContent::ColorContent(Note *parent, const QColor &color)
    : NoteContent(parent, NoteType::Color)
    , m_colorItem(parent, color)
{
    if (parent) {
        parent->addToGroup(&m_colorItem);
        m_colorItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }
}

ColorContent::~ColorContent()
{
    if (note())
        note()->removeFromGroup(&m_colorItem);
}

qreal ColorContent::setWidthAndGetHeight(qreal /*width*/) // We do not need width because we can't word-break, and width is always >= minWidth()
{
    return m_colorItem.boundingRect().height();
}

void ColorContent::saveToNode(QXmlStreamWriter &stream)
{
    stream.writeStartElement("content");
    stream.writeCharacters(color().name());
    stream.writeEndElement();
}

QMap<QString, QString> ColorContent::toolTipInfos()
{
    QMap<QString, QString> toolTip;

    int hue, saturation, value;
    color().getHsv(&hue, &saturation, &value);

    toolTip.insert(i18nc("RGB Colorspace: Red/Green/Blue", "RGB"),
                   i18n("<i>Red</i>: %1, <i>Green</i>: %2, <i>Blue</i>: %3,", QString::number(color().red()), QString::number(color().green()), QString::number(color().blue())));
    toolTip.insert(i18nc("HSV Colorspace: Hue/Saturation/Value", "HSV"),
                   i18n("<i>Hue</i>: %1, <i>Saturation</i>: %2, <i>Value</i>: %3,", QString::number(hue), QString::number(saturation), QString::number(value)));

    const QString colorName = Tools::cssColorName(color().name());
    if (!colorName.isEmpty()) {
        toolTip.insert(i18n("CSS Color Name"), colorName);
    }

    toolTip.insert(i18n("Is Web Color"), Tools::isWebColor(color()) ? i18n("Yes") : i18n("No"));

    return toolTip;
}

void ColorContent::setColor(const QColor &color)
{
    m_colorItem.setColor(color);
    contentChanged(m_colorItem.boundingRect().width());
}

void ColorContent::addAlternateDragObjects(QMimeData *dragObject)
{
    dragObject->setColorData(color());
}

void ColorContent::exportToHTML(HTMLExporter *exporter, int /*indent*/)
{
    // FIXME: Duplicate from setColor(): TODO: rectSize()
    QRectF textRect = QFontMetrics(note()->font()).boundingRect(color().name());
    int rectHeight = (textRect.height() + 2) * 3 / 2;
    int rectWidth = rectHeight * 14 / 10; // 1.4 times the height, like A4 papers.

    QString fileName = /*Tools::fileNameForNewFile(*/ QString("color_%1.png").arg(color().name().toLower().mid(1)) /*, exportData.iconsFolderPath)*/;
    QString fullPath = exporter->iconsFolderPath + fileName;
    QPixmap colorIcon(rectWidth, rectHeight);
    QPainter painter(&colorIcon);
    painter.setBrush(color());
    painter.drawRoundedRect(0, 0, rectWidth, rectHeight, 2, 2);
    colorIcon.save(fullPath, "PNG");
    QString iconHtml = QString("<img src=\"%1\" width=\"%2\" height=\"%3\" alt=\"\">").arg(exporter->iconsFolderName + fileName, QString::number(colorIcon.width()), QString::number(colorIcon.height()));

    exporter->stream << iconHtml + ' ' + color().name();
}

/** class UnknownItem:
 */

const qreal UnknownItem::DECORATION_MARGIN = 2;

UnknownItem::UnknownItem(Note *parent)
    : QGraphicsItem(parent)
    , m_note(parent)
{
}

QRectF UnknownItem::boundingRect() const
{
    return QRectF(0, 0, m_textRect.width() + 2 * DECORATION_MARGIN, m_textRect.height() + 2 * DECORATION_MARGIN);
}

void UnknownItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QPalette palette = m_note->basket()->palette();
    qreal width = boundingRect().width();
    qreal height = boundingRect().height();
    painter->setPen(palette.color(QPalette::Active, QPalette::WindowText));

    // Stroke:
    QColor stroke = Tools::mixColor(palette.color(QPalette::Active, QPalette::Background), palette.color(QPalette::Active, QPalette::WindowText));
    painter->setPen(stroke);
    painter->drawLine(1, 0, width - 2, 0);
    painter->drawLine(0, 1, 0, height - 2);
    painter->drawLine(1, height - 1, width - 2, height - 1);
    painter->drawLine(width - 1, 1, width - 1, height - 2);
    // Round corners:
    painter->setPen(Tools::mixColor(palette.color(QPalette::Active, QPalette::Background), stroke));
    painter->drawPoint(1, 1);
    painter->drawPoint(1, height - 2);
    painter->drawPoint(width - 2, height - 2);
    painter->drawPoint(width - 2, 1);

    painter->setPen(palette.color(QPalette::Active, QPalette::WindowText));
    painter->drawText(DECORATION_MARGIN, DECORATION_MARGIN, width - 2 * DECORATION_MARGIN, height - 2 * DECORATION_MARGIN, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_mimeTypes);
}

void UnknownItem::setMimeTypes(QString mimeTypes)
{
    m_mimeTypes = mimeTypes;
    m_textRect = QFontMetrics(m_note->font()).boundingRect(0, 0, 1, 500000, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_mimeTypes);
}

void UnknownItem::setWidth(qreal width)
{
    prepareGeometryChange();
    m_textRect = QFontMetrics(m_note->font()).boundingRect(0, 0, width, 500000, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_mimeTypes);
}

/** class UnknownContent:
 */

UnknownContent::UnknownContent(Note *parent, const QString &fileName)
    : NoteContent(parent, NoteType::Unknown, fileName)
    , m_unknownItem(parent)
{
    if (parent) {
        parent->addToGroup(&m_unknownItem);
        m_unknownItem.setPos(parent->contentX(), Note::NOTE_MARGIN);
    }

    basket()->addWatchedFile(fullPath());
    loadFromFile(/*lazyLoad=*/false);
}

UnknownContent::~UnknownContent()
{
    if (note())
        note()->removeFromGroup(&m_unknownItem);
}

qreal UnknownContent::setWidthAndGetHeight(qreal width)
{
    m_unknownItem.setWidth(width);
    return m_unknownItem.boundingRect().height();
}

bool UnknownContent::loadFromFile(bool /*lazyLoad*/)
{
    DEBUG_WIN << "Loading UnknownContent From " + basket()->folderName() + fileName();
    QString mimeTypes;
    QFile file(fullPath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString line;
        // Get the MIME-types names:
        do {
            if (!stream.atEnd()) {
                line = stream.readLine();
                if (!line.isEmpty()) {
                    if (mimeTypes.isEmpty())
                        mimeTypes += line;
                    else
                        mimeTypes += QString("\n") + line;
                }
            }
        } while (!line.isEmpty() && !stream.atEnd());
        file.close();
    }

    m_unknownItem.setMimeTypes(mimeTypes);
    contentChanged(m_unknownItem.boundingRect().width() + 1);

    return true;
}

void UnknownContent::addAlternateDragObjects(QMimeData *dragObject)
{
    QFile file(fullPath());
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream stream(&file);
        // Get the MIME types names:
        QStringList mimes;
        QString line;
        do {
            if (!stream.atEnd()) {
                stream >> line;
                if (!line.isEmpty())
                    mimes.append(line);
            }
        } while (!line.isEmpty() && !stream.atEnd());
        // Add the streams:
        quint64 size; // TODO: It was quint32 in version 0.5.0 !
        QByteArray *array;
        for (int i = 0; i < mimes.count(); ++i) {
            // Get the size:
            stream >> size;
            // Allocate memory to retrieve size bytes and store them:
            array = new QByteArray;
            array->resize(size);
            stream.readRawData(array->data(), size);
            // Creata and add the QDragObject:
            dragObject->setData(mimes.at(i).toLatin1(), *array);
            delete array; // FIXME: Should we?
        }
        file.close();
    }
}

void UnknownContent::exportToHTML(HTMLExporter *exporter, int indent)
{
    QString spaces;
    exporter->stream << "<div class=\"unknown\">" << mimeTypes().replace("\n", '\n' + spaces.fill(' ', indent + 1 + 1)) << "</div>";
}

void LinkContent::decodeHtmlTitle()
{
    KEncodingProber prober;
    prober.feed(m_httpBuff);

    // Fallback scheme: KEncodingProber - QTextCodec::codecForHtml - UTF-8
    QTextCodec *textCodec;
    if (prober.confidence() > 0.5)
        textCodec = QTextCodec::codecForName(prober.encoding());
    else
        textCodec = QTextCodec::codecForHtml(m_httpBuff, QTextCodec::codecForName("utf-8"));

    QString httpBuff = textCodec->toUnicode(m_httpBuff.data(), m_httpBuff.size());

    // todo: this should probably strip odd html tags like &nbsp; etc
    QRegExp reg("<title>[\\s]*(&nbsp;)?([^<]+)[\\s]*</title>", Qt::CaseInsensitive);
    reg.setMinimal(true);
    // qDebug() << *m_httpBuff << " bytes: " << bytes_read;

    if (reg.indexIn(httpBuff) >= 0) {
        m_title = reg.cap(2);
        m_autoTitle = false;
        setEdited();

        // refresh the title
        setLink(url(), title(), icon(), autoTitle(), autoIcon());
    }
}
