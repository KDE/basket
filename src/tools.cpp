/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tools.h"
#include "tag.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontInfo>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QMimeData>
#include <QObject>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
#include <QTime>

#include <QTextBlock>
#include <QTextDocument>

#include <KIO/CopyJob> //For KIO::trash
#include <KLocalizedString>

#include "config.h"
#include "debugwindow.h"

// cross reference
#include "bnpview.h"
#include "global.h"
#include "htmlexporter.h"
#include "linklabel.h"

#include <langinfo.h>

QVector<QTime> StopWatch::starts;
QVector<double> StopWatch::totals;
QVector<uint> StopWatch::counts;

void StopWatch::start(int id)
{
    if (id >= starts.size()) {
        totals.resize(id + 1);
        counts.resize(id + 1);
        for (int i = starts.size(); i <= id; i++) {
            totals[i] = 0;
            counts[i] = 0;
        }
        starts.resize(id + 1);
    }
    starts[id] = QTime::currentTime();
}

void StopWatch::check(int id)
{
    if (id >= starts.size())
        return;
    double time = starts[id].msecsTo(QTime::currentTime()) / 1000.0;
    totals[id] += time;
    counts[id]++;
    qDebug() << Q_FUNC_INFO << "Timer_" << id << ": " << time << " s    [" << counts[id] << " times, total: " << totals[id]
             << " s, average: " << totals[id] / counts[id] << " s]" << Qt::endl;
}

QString Tools::textToHTML(const QString &text)
{
    if (text.isEmpty())
        return QStringLiteral("<p></p>");
    if (/*text.isEmpty() ||*/ text == QStringLiteral(" ") || text == QStringLiteral("&nbsp;"))
        return QStringLiteral("<p>&nbsp;</p>");

    // convertFromPlainText() replace "\n\n" by "</p>\n<p>": we don't want that
    QString htmlString = Qt::convertFromPlainText(text, Qt::WhiteSpaceNormal);
    return htmlString.replace(QStringLiteral("</p>\n"), QStringLiteral("<br>\n<br>\n"))
        .replace(QStringLiteral("\n<p>"), QStringLiteral("\n")); // Don't replace first and last tags
}

QString Tools::textToHTMLWithoutP(const QString &text)
{
    // textToHTML(text) return "<p>HTMLizedText</p>". We remove the strating "<p>" and ending </p>"
    QString HTMLizedText = textToHTML(text);
    return HTMLizedText.mid(3, HTMLizedText.length() - 3 - 4);
}

QString Tools::htmlToParagraph(const QString &html)
{
    QString result = html;

    // Remove the <html> start tag, all the <head> and the <body> start
    QRegularExpression patternBodyTag(QStringLiteral("<body.*?>"));
    QRegularExpressionMatch bodyTag = patternBodyTag.match(result);

    if (bodyTag.hasMatch()) {
        result = result.mid(bodyTag.capturedEnd(0));
    }

    // Remove the ending "</p>\n</body></html>", each tag can be separated by space characters (%s)
    // "</p>" can be omitted (eg. if the HTML doesn't contain paragraph but tables), as well as "</body>" (optional)
    QRegularExpression re(QStringLiteral("(?:(?:</p>[\\s\\n\\r\\t]*)*</body>[\\s\\n\\r\\t]*)*</html>"), QRegularExpression::CaseInsensitiveOption);
    int pos = result.indexOf(re);
    if (pos != -1)
        result = result.left(pos);

    return result;
}

// The following is adapted from KStringHanlder::tagURLs
// The adaptation lies in the change to urlEx
// Thanks to Richard Heck
QString Tools::detectURLs(const QString &text)
{
    QRegularExpression urlEx(QStringLiteral("<!DOCTYPE[^\"]+\"([^\"]+)\"[^\"]+\"([^\"]+)/([^/]+)\\.dtd\">"));
    QString richText(text);
    int urlPos = 0;
    int urlLen;
    if ((urlPos = richText.indexOf(urlEx, urlPos)) >= 0) {
        QRegularExpressionMatch m = urlEx.match(richText);
        urlPos += m.capturedLength();
    } else
        urlPos = 0;
    urlEx.setPattern(QStringLiteral("(www\\.(?!\\.)|(fish|(f|ht)tp(|s))://)[\\d\\w\\./,:_~\\?=&;#@\\-\\+\\%\\$]+[\\d\\w/]"));
    while ((urlPos = richText.indexOf(urlEx, urlPos)) >= 0) {
        QRegularExpressionMatch m = urlEx.match(richText);
        urlLen = m.capturedLength();

        // if this match is already a link don't convert it.
        if (richText.mid(urlPos - 6, 6) == QStringLiteral("href=\"")) {
            urlPos += urlLen;
            continue;
        }

        QString href = richText.mid(urlPos, urlLen);
        // we handle basket links separately...
        if (href.contains(QStringLiteral("basket://"))) {
            urlPos += urlLen;
            continue;
        }
        // Qt doesn't support (?<=pattern) so we do it here
        if ((urlPos > 0) && richText[urlPos - 1].isLetterOrNumber()) {
            urlPos++;
            continue;
        }
        // Don't use QString::arg since %01, %20, etc could be in the string
        QString anchor = QStringLiteral("<a href=\"") + href + QStringLiteral("\">") + href + QStringLiteral("</a>");
        richText.replace(urlPos, urlLen, anchor);
        urlPos += anchor.length();
    }
    return richText;
}

QString Tools::detectCrossReferences(const QString &text, bool userLink, HTMLExporter *exporter)
{
    QString richText(text);

    int urlPos = 0;
    int urlLen;

    QRegularExpression urlEx(QStringLiteral("\\[\\[(.+)\\]\\]"));

    while ((urlPos = richText.indexOf(urlEx, urlPos)) >= 0) {
        QRegularExpressionMatch m = urlEx.match(richText);
        urlLen = m.capturedLength();
        QString href = m.captured(1);

        QStringList hrefParts = href.split(QLatin1Char('|'));
        QString anchor;

        if (exporter) // if we're exporting this basket to html.
            anchor = crossReferenceForHtml(hrefParts, exporter);
        else if (userLink) // the link is manually created (ie [[/top level/sub]] )
            anchor = crossReferenceForConversion(hrefParts);
        else // otherwise it's a standard link (ie. [[basket://basket107]] )
            anchor = crossReferenceForBasket(hrefParts);

        if (!anchor.isEmpty()) {
            richText.replace(urlPos, urlLen, anchor);
            urlPos += anchor.length();

        } else {
            urlPos += urlLen;
        }
    }
    return richText;
}

QList<State *> Tools::detectTags(const QString &text, int &prefixLength)
{
    QList<State *> tagsDetected;
    prefixLength = 0;
    QRegularExpressionMatchIterator matchIt = Tag::regexpDetectTagsInPlainText().globalMatch(text);
    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        prefixLength = match.capturedEnd(0);
        const QString &stateRepr = match.captured(1);
        State *state = Tag::stateByTextEquiv(stateRepr);
        if (state)
            tagsDetected.append(state);
    }

    // if tags are found, eat possible trailing whitespaces as well
    if (prefixLength) {
        prefixLength = text.indexOf(QRegularExpression(QStringLiteral("[^\\s]")), prefixLength);
        if (prefixLength == -1)
            prefixLength = text.length();
    }
    return tagsDetected;
}

QString Tools::crossReferenceForBasket(const QStringList &linkParts)
{
    QString basketLink = linkParts.first();
    if (!basketLink.startsWith(QStringLiteral("basket://")))
        return QString();

    QString url = basketLink.mid(9, basketLink.length() - 9);
    if (url.isEmpty())
        return QString();

    QString title = linkParts.last().trimmed();
    QString css = LinkLook::crossReferenceLook->toCSS(QStringLiteral("cross_reference"), QColor());
    QString classes = QStringLiteral("cross_reference");

    QString anchor = QStringLiteral("<style>%1</style><a href=\"%2\" class=\"%3\">%4</a>")
                         .arg(css)
                         .arg(basketLink)
                         .arg(classes)
                         .arg(QUrl::fromPercentEncoding(title.toUtf8()));

    return anchor;
}

QString Tools::crossReferenceForHtml(const QStringList &linkParts, HTMLExporter *exporter)
{
    QString basketLink = linkParts.first();
    QString title = linkParts.last().trimmed();
    if (!basketLink.startsWith(QStringLiteral("basket://")))
        return QString();

    QString url = basketLink.mid(9, basketLink.length() - 9);
    if (url.isEmpty())
        return QString();

    BasketScene *basket = Global::bnpView->basketForFolderName(url);

    // remove the trailing slash.
    url = url.left(url.length() - 1);

    // if the basket we're trying to link to is the basket that was exported then
    // we have to use a special way to refer to it for the links.
    if (basket == exporter->exportedBasket)
        url = QStringLiteral("../../") + exporter->fileName;
    else {
        // if we're in the exported basket then the links have to include
        // the sub directories.
        if (exporter->currentBasket == exporter->exportedBasket)
            url.prepend(exporter->basketsFolderName);
        if (!url.isEmpty())
            url.append(QStringLiteral(".html"));
    }

    QString classes = QStringLiteral("cross_reference");
    QString anchor = QStringLiteral("<a href=\"") + url + QStringLiteral("\" class=\"") + classes + QStringLiteral("\">")
        + QUrl::fromPercentEncoding(title.toUtf8()) + QStringLiteral("</a>");
    return anchor;
}

QString Tools::crossReferenceForConversion(const QStringList &linkParts)
{
    QString basketLink = linkParts.first();
    QString title;

    if (basketLink.startsWith(QStringLiteral("basket://")))
        return QStringLiteral("[[%1|%2]]").arg(basketLink, linkParts.last());

    if (basketLink.endsWith(QLatin1Char('/')))
        basketLink = basketLink.left(basketLink.length() - 1);

    QStringList pages = basketLink.split(QLatin1Char('/'));

    if (linkParts.count() <= 1)
        title = pages.last();
    else
        title = linkParts.last().trimmed();

    QString url = Global::bnpView->folderFromBasketNameLink(pages);
    if (url.isEmpty())
        return QString();

    return QStringLiteral("[[basket://%1|%2]]").arg(url, title);
}

QString Tools::htmlToText(const QString &html)
{
    QString text = htmlToParagraph(html);
    text.remove(QLatin1Char('\n'));
    text.replace(QStringLiteral("</h1>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</h2>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</h3>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</h4>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</h5>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</h6>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</li>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</dt>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</dd>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("<dd>"), QStringLiteral("   "));
    text.replace(QStringLiteral("</div>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</blockquote>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</caption>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</tr>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</th>"), QStringLiteral("  "));
    text.replace(QStringLiteral("</td>"), QStringLiteral("  "));
    text.replace(QStringLiteral("<br>"), QStringLiteral("\n"));
    text.replace(QStringLiteral("<br />"), QStringLiteral("\n"));
    text.replace(QStringLiteral("</p>"), QStringLiteral("\n"));
    // FIXME: Format <table> tags better, if possible
    // TODO: Replace &eacute; and co. by their equivalent!

    // To manage tags:
    int pos = 0;
    int pos2;
    QString tag, tag3;
    // To manage lists:
    int deep = 0; // The deep of the current line in imbriqued lists
    QList<bool> ul; // true if current list is a <ul> one, false if it's an <ol> one
    QList<int> lines; // The line number if it is an <ol> list
    // We're removing every other tags, or replace them in the case of li:
    while ((pos = text.indexOf(QStringLiteral("<")), pos) != -1) {
        // What is the current tag?
        tag = text.mid(pos + 1, 2);
        tag3 = text.mid(pos + 1, 3);
        // Lists work:
        if (tag == QStringLiteral("ul")) {
            deep++;
            ul.push_back(true);
            lines.push_back(-1);
        } else if (tag == QStringLiteral("ol")) {
            deep++;
            ul.push_back(false);
            lines.push_back(0);
        } else if (tag3 == QStringLiteral("/ul") || tag3 == QStringLiteral("/ol")) {
            deep--;
            ul.pop_back();
            lines.pop_back();
        }
        // Where the tag closes?
        pos2 = text.indexOf(QStringLiteral(">"));
        if (pos2 != -1) {
            // Remove the tag:
            text.remove(pos, pos2 - pos + 1);
            // And replace li with "* ", "x. "... without forbidding to indent that:
            if (tag == QStringLiteral("li")) {
                // How many spaces before the line (indentation):
                QString spaces;
                for (int i = 1; i < deep; i++)
                    spaces += QStringLiteral("  ");
                // The bullet or number of the line:
                QString bullet = QStringLiteral("* ");
                if (ul.back() == false) {
                    lines.push_back(lines.back() + 1);
                    lines.pop_back();
                    bullet = QString::number(lines.back()) + QStringLiteral(". ");
                }
                // Insertion:
                text.insert(pos, spaces + bullet);
            }
            if ((tag3 == QStringLiteral("/ul") || tag3 == QStringLiteral("/ol")) && deep == 0)
                text.insert(pos, QStringLiteral("\n")); // Empty line before and after a set of lists
        }
        ++pos;
    }

    text.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    text.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    text.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    text.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
    text.replace(QStringLiteral("&amp;"), QStringLiteral("&")); // CONVERT IN LAST!!

    // HtmlContent produces "\n" for empty note
    if (text == QStringLiteral("\n"))
        text = QString();

    return text;
}

QString Tools::textDocumentToMinimalHTML(QTextDocument *document)
{
    QFont originalFont = document->defaultFont();
    document->setDefaultFont(QFont());
    QString docContent = document->toHtml();
    document->setDefaultFont(originalFont);

    // Tag styles appear in html output as body styles. Remove them to preserve internal formatting.
    QRegularExpression patternBodyTag(QStringLiteral("<body.*?>"));
    QRegularExpressionMatch bodyTag = patternBodyTag.match(docContent);

    if (!bodyTag.hasMatch())
        return docContent;

    return docContent.replace(bodyTag.capturedStart(0), bodyTag.capturedLength(0), QStringLiteral("<body>"));
}

QString Tools::cssFontDefinition(const QFont &font, bool onlyFontFamily)
{
    // The font definition:
    QString definition = font.key() + (font.italic() ? QStringLiteral("italic ") : QString()) + (font.bold() ? QStringLiteral("bold ") : QString())
        + QString::number(QFontInfo(font).pixelSize()) + QStringLiteral("px ");

    // Then, try to match the font name with a standard CSS font family:
    QString genericFont;
    if (definition.contains(QStringLiteral("serif"), Qt::CaseInsensitive) || definition.contains(QStringLiteral("roman"), Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("serif");
    }
    // No "else if" because "sans serif" must be counted as "sans". So, the order between "serif" and "sans" is important
    if (definition.contains(QStringLiteral("sans"), Qt::CaseInsensitive) || definition.contains(QStringLiteral("arial"), Qt::CaseInsensitive)
        || definition.contains(QStringLiteral("helvetica"), Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("sans-serif");
    }
    if (definition.contains(QStringLiteral("mono"), Qt::CaseInsensitive) || definition.contains(QStringLiteral("courier"), Qt::CaseInsensitive)
        || definition.contains(QStringLiteral("typewriter"), Qt::CaseInsensitive) || definition.contains(QStringLiteral("console"), Qt::CaseInsensitive)
        || definition.contains(QStringLiteral("terminal"), Qt::CaseInsensitive) || definition.contains(QStringLiteral("news"), Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("monospace");
    }

    // Eventually add the generic font family to the definition:
    QString fontDefinition = QStringLiteral("\"%1\"").arg(font.family());
    if (!genericFont.isEmpty())
        fontDefinition += QStringLiteral(", ") + genericFont;

    if (onlyFontFamily)
        return fontDefinition;

    return definition + fontDefinition;
}

QString Tools::stripEndWhiteSpaces(const QString &string)
{
    uint length = string.length();
    uint i;
    for (i = length; i > 0; --i)
        if (!string[i - 1].isSpace())
            break;
    if (i == 0)
        return QString();
    else
        return string.left(i);
}

QString Tools::cssColorName(const QString &colorHex)
{
    static const QMap<QString, QString> cssColors = {{QStringLiteral("#00ffff"), QStringLiteral("aqua")},
                                                     {QStringLiteral("#000000"), QStringLiteral("black")},
                                                     {QStringLiteral("#0000ff"), QStringLiteral("blue")},
                                                     {QStringLiteral("#ff00ff"), QStringLiteral("fuchsia")},
                                                     {QStringLiteral("#808080"), QStringLiteral("gray")},
                                                     {QStringLiteral("#008000"), QStringLiteral("green")},
                                                     {QStringLiteral("#00ff00"), QStringLiteral("lime")},
                                                     {QStringLiteral("#800000"), QStringLiteral("maroon")},
                                                     {QStringLiteral("#000080"), QStringLiteral("navy")},
                                                     {QStringLiteral("#808000"), QStringLiteral("olive")},
                                                     {QStringLiteral("#800080"), QStringLiteral("purple")},
                                                     {QStringLiteral("#ff0000"), QStringLiteral("red")},
                                                     {QStringLiteral("#c0c0c0"), QStringLiteral("silver")},
                                                     {QStringLiteral("#008080"), QStringLiteral("teal")},
                                                     {QStringLiteral("#ffffff"), QStringLiteral("white")},
                                                     {QStringLiteral("#ffff00"), QStringLiteral("yellow")},
                                                     // CSS extended colors
                                                     {QStringLiteral("#f0f8ff"), QStringLiteral("aliceblue")},
                                                     {QStringLiteral("#faebd7"), QStringLiteral("antiquewhite")},
                                                     {QStringLiteral("#7fffd4"), QStringLiteral("aquamarine")},
                                                     {QStringLiteral("#f0ffff"), QStringLiteral("azure")},
                                                     {QStringLiteral("#f5f5dc"), QStringLiteral("beige")},
                                                     {QStringLiteral("#ffe4c4"), QStringLiteral("bisque")},
                                                     {QStringLiteral("#ffebcd"), QStringLiteral("blanchedalmond")},
                                                     {QStringLiteral("#8a2be2"), QStringLiteral("blueviolet")},
                                                     {QStringLiteral("#a52a2a"), QStringLiteral("brown")},
                                                     {QStringLiteral("#deb887"), QStringLiteral("burlywood")},
                                                     {QStringLiteral("#5f9ea0"), QStringLiteral("cadetblue")},
                                                     {QStringLiteral("#7fff00"), QStringLiteral("chartreuse")},
                                                     {QStringLiteral("#d2691e"), QStringLiteral("chocolate")},
                                                     {QStringLiteral("#ff7f50"), QStringLiteral("coral")},
                                                     {QStringLiteral("#6495ed"), QStringLiteral("cornflowerblue")},
                                                     {QStringLiteral("#fff8dc"), QStringLiteral("cornsilk")},
                                                     {QStringLiteral("#dc1436"), QStringLiteral("crimson")},
                                                     {QStringLiteral("#00ffff"), QStringLiteral("cyan")},
                                                     {QStringLiteral("#00008b"), QStringLiteral("darkblue")},
                                                     {QStringLiteral("#008b8b"), QStringLiteral("darkcyan")},
                                                     {QStringLiteral("#b8860b"), QStringLiteral("darkgoldenrod")},
                                                     {QStringLiteral("#a9a9a9"), QStringLiteral("darkgray")},
                                                     {QStringLiteral("#006400"), QStringLiteral("darkgreen")},
                                                     {QStringLiteral("#bdb76b"), QStringLiteral("darkkhaki")},
                                                     {QStringLiteral("#8b008b"), QStringLiteral("darkmagenta")},
                                                     {QStringLiteral("#556b2f"), QStringLiteral("darkolivegreen")},
                                                     {QStringLiteral("#ff8c00"), QStringLiteral("darkorange")},
                                                     {QStringLiteral("#9932cc"), QStringLiteral("darkorchid")},
                                                     {QStringLiteral("#8b0000"), QStringLiteral("darkred")},
                                                     {QStringLiteral("#e9967a"), QStringLiteral("darksalmon")},
                                                     {QStringLiteral("#8fbc8f"), QStringLiteral("darkseagreen")},
                                                     {QStringLiteral("#483d8b"), QStringLiteral("darkslateblue")},
                                                     {QStringLiteral("#2f4f4f"), QStringLiteral("darkslategray")},
                                                     {QStringLiteral("#00ced1"), QStringLiteral("darkturquoise")},
                                                     {QStringLiteral("#9400d3"), QStringLiteral("darkviolet")},
                                                     {QStringLiteral("#ff1493"), QStringLiteral("deeppink")},
                                                     {QStringLiteral("#00bfff"), QStringLiteral("deepskyblue")},
                                                     {QStringLiteral("#696969"), QStringLiteral("dimgray")},
                                                     {QStringLiteral("#1e90ff"), QStringLiteral("dodgerblue")},
                                                     {QStringLiteral("#b22222"), QStringLiteral("firebrick")},
                                                     {QStringLiteral("#fffaf0"), QStringLiteral("floralwhite")},
                                                     {QStringLiteral("#228b22"), QStringLiteral("forestgreen")},
                                                     {QStringLiteral("#dcdcdc"), QStringLiteral("gainsboro")},
                                                     {QStringLiteral("#f8f8ff"), QStringLiteral("ghostwhite")},
                                                     {QStringLiteral("#ffd700"), QStringLiteral("gold")},
                                                     {QStringLiteral("#daa520"), QStringLiteral("goldenrod")},
                                                     {QStringLiteral("#adff2f"), QStringLiteral("greenyellow")},
                                                     {QStringLiteral("#f0fff0"), QStringLiteral("honeydew")},
                                                     {QStringLiteral("#ff69b4"), QStringLiteral("hotpink")},
                                                     {QStringLiteral("#cd5c5c"), QStringLiteral("indianred")},
                                                     {QStringLiteral("#4b0082"), QStringLiteral("indigo")},
                                                     {QStringLiteral("#fffff0"), QStringLiteral("ivory")},
                                                     {QStringLiteral("#f0e68c"), QStringLiteral("khaki")},
                                                     {QStringLiteral("#e6e6fa"), QStringLiteral("lavender")},
                                                     {QStringLiteral("#fff0f5"), QStringLiteral("lavenderblush")},
                                                     {QStringLiteral("#7cfc00"), QStringLiteral("lawngreen")},
                                                     {QStringLiteral("#fffacd"), QStringLiteral("lemonchiffon")},
                                                     {QStringLiteral("#add8e6"), QStringLiteral("lightblue")},
                                                     {QStringLiteral("#f08080"), QStringLiteral("lightcoral")},
                                                     {QStringLiteral("#e0ffff"), QStringLiteral("lightcyan")},
                                                     {QStringLiteral("#fafad2"), QStringLiteral("lightgoldenrodyellow")},
                                                     {QStringLiteral("#90ee90"), QStringLiteral("lightgreen")},
                                                     {QStringLiteral("#d3d3d3"), QStringLiteral("lightgrey")},
                                                     {QStringLiteral("#ffb6c1"), QStringLiteral("lightpink")},
                                                     {QStringLiteral("#ffa07a"), QStringLiteral("lightsalmon")},
                                                     {QStringLiteral("#20b2aa"), QStringLiteral("lightseagreen")},
                                                     {QStringLiteral("#87cefa"), QStringLiteral("lightskyblue")},
                                                     {QStringLiteral("#778899"), QStringLiteral("lightslategray")},
                                                     {QStringLiteral("#b0c4de"), QStringLiteral("lightsteelblue")},
                                                     {QStringLiteral("#ffffe0"), QStringLiteral("lightyellow")},
                                                     {QStringLiteral("#32cd32"), QStringLiteral("limegreen")},
                                                     {QStringLiteral("#faf0e6"), QStringLiteral("linen")},
                                                     {QStringLiteral("#ff00ff"), QStringLiteral("magenta")},
                                                     {QStringLiteral("#66cdaa"), QStringLiteral("mediumaquamarine")},
                                                     {QStringLiteral("#0000cd"), QStringLiteral("mediumblue")},
                                                     {QStringLiteral("#ba55d3"), QStringLiteral("mediumorchid")},
                                                     {QStringLiteral("#9370db"), QStringLiteral("mediumpurple")},
                                                     {QStringLiteral("#3cb371"), QStringLiteral("mediumseagreen")},
                                                     {QStringLiteral("#7b68ee"), QStringLiteral("mediumslateblue")},
                                                     {QStringLiteral("#00fa9a"), QStringLiteral("mediumspringgreen")},
                                                     {QStringLiteral("#48d1cc"), QStringLiteral("mediumturquoise")},
                                                     {QStringLiteral("#c71585"), QStringLiteral("mediumvioletred")},
                                                     {QStringLiteral("#191970"), QStringLiteral("midnightblue")},
                                                     {QStringLiteral("#f5fffa"), QStringLiteral("mintcream")},
                                                     {QStringLiteral("#ffe4e1"), QStringLiteral("mistyrose")},
                                                     {QStringLiteral("#ffe4b5"), QStringLiteral("moccasin")},
                                                     {QStringLiteral("#ffdead"), QStringLiteral("navajowhite")},
                                                     {QStringLiteral("#fdf5e6"), QStringLiteral("oldlace")},
                                                     {QStringLiteral("#6b8e23"), QStringLiteral("olivedrab")},
                                                     {QStringLiteral("#ffa500"), QStringLiteral("orange")},
                                                     {QStringLiteral("#ff4500"), QStringLiteral("orangered")},
                                                     {QStringLiteral("#da70d6"), QStringLiteral("orchid")},
                                                     {QStringLiteral("#eee8aa"), QStringLiteral("palegoldenrod")},
                                                     {QStringLiteral("#98fb98"), QStringLiteral("palegreen")},
                                                     {QStringLiteral("#afeeee"), QStringLiteral("paleturquoise")},
                                                     {QStringLiteral("#db7093"), QStringLiteral("palevioletred")},
                                                     {QStringLiteral("#ffefd5"), QStringLiteral("papayawhip")},
                                                     {QStringLiteral("#ffdab9"), QStringLiteral("peachpuff")},
                                                     {QStringLiteral("#cd853f"), QStringLiteral("peru")},
                                                     {QStringLiteral("#ffc0cb"), QStringLiteral("pink")},
                                                     {QStringLiteral("#dda0dd"), QStringLiteral("plum")},
                                                     {QStringLiteral("#b0e0e6"), QStringLiteral("powderblue")},
                                                     {QStringLiteral("#bc8f8f"), QStringLiteral("rosybrown")},
                                                     {QStringLiteral("#4169e1"), QStringLiteral("royalblue")},
                                                     {QStringLiteral("#8b4513"), QStringLiteral("saddlebrown")},
                                                     {QStringLiteral("#fa8072"), QStringLiteral("salmon")},
                                                     {QStringLiteral("#f4a460"), QStringLiteral("sandybrown")},
                                                     {QStringLiteral("#2e8b57"), QStringLiteral("seagreen")},
                                                     {QStringLiteral("#fff5ee"), QStringLiteral("seashell")},
                                                     {QStringLiteral("#a0522d"), QStringLiteral("sienna")},
                                                     {QStringLiteral("#87ceeb"), QStringLiteral("skyblue")},
                                                     {QStringLiteral("#6a5acd"), QStringLiteral("slateblue")},
                                                     {QStringLiteral("#708090"), QStringLiteral("slategray")},
                                                     {QStringLiteral("#fffafa"), QStringLiteral("snow")},
                                                     {QStringLiteral("#00ff7f"), QStringLiteral("springgreen")},
                                                     {QStringLiteral("#4682b4"), QStringLiteral("steelblue")},
                                                     {QStringLiteral("#d2b48c"), QStringLiteral("tan")},
                                                     {QStringLiteral("#d8bfd8"), QStringLiteral("thistle")},
                                                     {QStringLiteral("#ff6347"), QStringLiteral("tomato")},
                                                     {QStringLiteral("#40e0d0"), QStringLiteral("turquoise")},
                                                     {QStringLiteral("#ee82ee"), QStringLiteral("violet")},
                                                     {QStringLiteral("#f5deb3"), QStringLiteral("wheat")},
                                                     {QStringLiteral("#f5f5f5"), QStringLiteral("whitesmoke")},
                                                     {QStringLiteral("#9acd32"), QStringLiteral("yellowgreen")}};

    return cssColors.value(colorHex, QString());
}

bool Tools::isWebColor(const QColor &color)
{
    int r = color.red(); // The 216 web colors are those colors whose RGB (Red, Green, Blue)
    int g = color.green(); //  values are all in the set (0, 51, 102, 153, 204, 255).
    int b = color.blue();

    return ((r == 0 || r == 51 || r == 102 || r == 153 || r == 204 || r == 255) && (g == 0 || g == 51 || g == 102 || g == 153 || g == 204 || g == 255)
            && (b == 0 || b == 51 || b == 102 || b == 153 || b == 204 || b == 255));
}

QColor Tools::mixColor(const QColor &color1, const QColor &color2, const float ratio)
{
    QColor mixedColor;
    mixedColor.setRgb((color1.red() * ratio + color2.red()) / (1 + ratio),
                      (color1.green() * ratio + color2.green()) / (1 + ratio),
                      (color1.blue() * ratio + color2.blue()) / (1 + ratio));
    return mixedColor;
}

bool Tools::tooDark(const QColor &color)
{
    return color.value() < 175;
}

// TODO: Use it for all indentPixmap()
QPixmap Tools::normalizePixmap(const QPixmap &pixmap, int width, int height)
{
    if (height <= 0)
        height = width;

    if (pixmap.isNull() || (pixmap.width() == width && pixmap.height() == height))
        return pixmap;

    return pixmap;
}

QPixmap Tools::indentPixmap(const QPixmap &source, int depth, int deltaX)
{
    // Verify if it is possible:
    if (depth <= 0 || source.isNull())
        return source;

    // Compute the number of pixels to indent:
    if (deltaX <= 0)
        deltaX = 2 * source.width() / 3;
    int indent = depth * deltaX;

    // Create the images:
    QImage resultImage(indent + source.width(), source.height(), QImage::Format_ARGB32);
    resultImage.setColorCount(32);

    QImage sourceImage = source.toImage();

    // Clear the indent part (the left part) by making it fully transparent:
    uint *p;
    for (int row = 0; row < resultImage.height(); ++row) {
        for (int column = 0; column < resultImage.width(); ++column) {
            p = (uint *)resultImage.scanLine(row) + column;
            *p = 0; // qRgba(0, 0, 0, 0)
        }
    }

    // Copy the source image byte per byte to the right part:
    uint *q;
    for (int row = 0; row < sourceImage.height(); ++row) {
        for (int column = 0; column < sourceImage.width(); ++column) {
            p = (uint *)resultImage.scanLine(row) + indent + column;
            q = (uint *)sourceImage.scanLine(row) + column;
            *p = *q;
        }
    }

    // And return the result:
    QPixmap result = QPixmap::fromImage(resultImage);
    return result;
}

void Tools::deleteRecursively(const QString &folderOrFile)
{
    if (folderOrFile.isEmpty())
        return;

    QFileInfo fileInfo(folderOrFile);
    if (fileInfo.isDir()) {
        // Delete the child files:
        QDir dir(folderOrFile, QString(), QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
        QStringList list = dir.entryList();
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
            if (*it != QStringLiteral(".") && *it != QStringLiteral(".."))
                deleteRecursively(folderOrFile + QLatin1Char('/') + *it);
        // And then delete the folder:
        dir.rmdir(folderOrFile);
    } else
        // Delete the file:
        QFile::remove(folderOrFile);
}

void Tools::deleteMetadataRecursively(const QString &folderOrFile)
{
    QFileInfo fileInfo(folderOrFile);
    if (fileInfo.isDir()) {
        // Delete Metadata of the child files:
        QDir dir(folderOrFile, QString(), QDir::Name | QDir::IgnoreCase, QDir::TypeMask | QDir::Hidden);
        QStringList list = dir.entryList();
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
            if (*it != QStringLiteral(".") && *it != QStringLiteral(".."))
                deleteMetadataRecursively(folderOrFile + QLatin1Char('/') + *it);
    }
}

void Tools::trashRecursively(const QString &folderOrFile)
{
    if (folderOrFile.isEmpty())
        return;

    KIO::trash(QUrl::fromLocalFile(folderOrFile), KIO::HideProgressInfo);
}

QString Tools::fileNameForNewFile(const QString &wantedName, const QString &destFolder)
{
    QString fileName = wantedName;
    QString fullName = destFolder + fileName;
    QString extension = QString();
    int number = 2;
    QDir dir;

    // First check if the file do not exists yet (simpler and more often case)
    dir = QDir(fullName);
    if (!dir.exists(fullName))
        return fileName;

    // Find the file extension, if it exists : Split fileName in fileName and extension
    // Example : fileName == "note5-3.txt" => fileName = "note5-3" and extension = ".txt"
    int extIndex = fileName.lastIndexOf(QLatin1Char('.'));
    if (extIndex != -1 && extIndex != int(fileName.length() - 1)) { // Extension found and fileName do not ends with '.' !
        extension = fileName.mid(extIndex);
        fileName.truncate(extIndex);
    } // else fileName = fileName and extension = QString()

    // Find the file number, if it exists : Split fileName in fileName and number
    // Example : fileName == "note5-3" => fileName = "note5" and number = 3
    int extNumber = fileName.lastIndexOf(QLatin1Char('-'));
    if (extNumber != -1 && extNumber != int(fileName.length() - 1)) { // Number found and fileName do not ends with '-' !
        bool isANumber;
        int theNumber = fileName.mid(extNumber + 1).toInt(&isANumber);
        if (isANumber) {
            number = theNumber;
            fileName.truncate(extNumber);
        } // else :
    } // else fileName = fileName and number = 2 (because if the file already exists, the generated name is at last the 2nd)

    QString finalName;
    for (/*int number = 2*/;; ++number) { // TODO: FIXME: If overflow ???
        finalName = fileName + QLatin1Char('-') + QString::number(number) + extension;
        fullName = destFolder + finalName;
        dir = QDir(fullName);
        if (!dir.exists(fullName))
            break;
    }

    return finalName;
}

qint64 Tools::computeSizeRecursively(const QString &path)
{
    qint64 result = 0;

    QFileInfo file(path);
    result += file.size();
    if (file.isDir()) {
        QFileInfoList children = QDir(path).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Hidden);
        for (const QFileInfo &child : children)
            result += computeSizeRecursively(child.absoluteFilePath());
    }
    return result;
}

// TODO: Move it from NoteFactory
/*QString NoteFactory::iconForURL(const QUrl &url)
{
    QString icon = KMimeType::iconNameForUrl(url.url());
    if ( url.scheme() == "mailto" )
        icon = "message";
    return icon;
}*/

bool Tools::isAFileCut(const QMimeData *source)
{
    if (source->hasFormat(QStringLiteral("application/x-kde-cutselection"))) {
        QByteArray array = source->data(QStringLiteral("application/x-kde-cutselection"));
        return !array.isEmpty() && QByteArray(array.data(), array.size() + 1).at(0) == '1';
    }

    return false;
}

void Tools::printChildren(QObject *parent)
{
    for (const auto &obj : parent->children()) {
        qDebug() << Q_FUNC_INFO << obj->metaObject()->className() << ": " << obj->objectName() << Qt::endl;
    }
}

QString Tools::makeStandardCaption(const QString &userCaption)
{
    QString caption = QGuiApplication::applicationDisplayName();

    if (!userCaption.isEmpty()) {
        return userCaption + i18nc("Document/application separator in titlebar", " – ") + caption;
    }

    return caption;
}

QByteArray Tools::systemCodeset()
{
    QByteArray codeset;
#if 0 // TODO verify
#if _LANGINFO_H
    // Qt since 4.2 always returns 'System' as codecForLocale and libraries like for example
    // KEncodingFileDialog expects real encoding name. So on systems that have langinfo.h use
    // nl_langinfo instead, just like Qt compiled without iconv does. Windows already has its own
    // workaround

    codeset = nl_langinfo(CODESET);

    if ((codeset == "ANSI_X3.4-1968") || (codeset == "US-ASCII")) {
        // means ascii, "C"; QTextCodec doesn't know, so avoid warning
        codeset = "ISO-8859-1";
    }
#endif
#endif
    return codeset;
}
