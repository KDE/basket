/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tools.h"

#include <QDebug>
#include <QGuiApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtGui/QFont>
#include <QtGui/QFontInfo>
#include <QtGui/QImage>
#include <QtGui/QPixmap>

#include <QTextBlock>
#include <QtGui/QTextDocument>

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
    qDebug() << Q_FUNC_INFO << "Timer_" << id << ": " << time << " s    [" << counts[id] << " times, total: " << totals[id] << " s, average: " << totals[id] / counts[id] << " s]" << endl;
}

/** @namespace HTM
 *  @brief HTML tags constants */
namespace HTM
{
static const char *BR = "<br/>";
static const char *PAR = "<p>";
static const char *_PAR = "</p>";

// Styles
static const char *FONT_FAMILY = "font-family: %1; ";
static const char *FONT_STYLE = "font-style: %1; ";
static const char *TEXT_DECORATION = "text-decoration: %1; ";

static const char *ITALIC = "italic";
static const char *UNDERLINE = "underline";
static const char *LINE_THROUGH = "line-through";

// static const char* FONT_WEIGHT = "font-weight: %1; ";
static const char *FONT_SIZE = "font-size: %1pt; ";
static const char *COLOR = "color: %1; ";
}

QString Tools::textToHTML(const QString &text)
{
    if (text.isEmpty())
        return "<p></p>";
    if (/*text.isEmpty() ||*/ text == " " || text == "&nbsp;")
        return "<p>&nbsp;</p>";

    // convertFromPlainText() replace "\n\n" by "</p>\n<p>": we don't want that
    QString htmlString = Qt::convertFromPlainText(text, Qt::WhiteSpaceNormal);
    return htmlString.replace("</p>\n", "<br>\n<br>\n").replace("\n<p>", "\n"); // Don't replace first and last tags
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
    bool startedBySpan = false;

    // Remove the <html> start tag, all the <head> and the <body> start
    // Because <body> can contain style="..." parameter, we transform it to <span>
    int pos = result.indexOf("<body");
    if (pos != -1) {
        result = "<span" + result.mid(pos + 5);
        startedBySpan = true;
    }

    // Remove the ending "</p>\n</body></html>", each tag can be separated by space characters (%s)
    // "</p>" can be omitted (eg. if the HTML doesn't contain paragraph but tables), as well as "</body>" (optional)
    pos = result.indexOf(QRegExp("(?:(?:</p>[\\s\\n\\r\\t]*)*</body>[\\s\\n\\r\\t]*)*</html>", Qt::CaseInsensitive));
    if (pos != -1)
        result = result.left(pos);

    if (startedBySpan)
        result += "</span>";

    return result;
}

// The following is adapted from KStringHanlder::tagURLs
// The adaptation lies in the change to urlEx
// Thanks to Richard Heck
QString Tools::tagURLs(const QString &text)
{
    QRegExp urlEx("<!DOCTYPE[^\"]+\"([^\"]+)\"[^\"]+\"([^\"]+)/([^/]+)\\.dtd\">");
    QString richText(text);
    int urlPos = 0;
    int urlLen;
    if ((urlPos = urlEx.indexIn(richText, urlPos)) >= 0)
        urlPos += urlEx.matchedLength();
    else
        urlPos = 0;
    urlEx.setPattern("(www\\.(?!\\.)|(fish|(f|ht)tp(|s))://)[\\d\\w\\./,:_~\\?=&;#@\\-\\+\\%\\$]+[\\d\\w/]");
    while ((urlPos = urlEx.indexIn(richText, urlPos)) >= 0) {
        urlLen = urlEx.matchedLength();

        // if this match is already a link don't convert it.
        if (richText.mid(urlPos - 6, 6) == "href=\"") {
            urlPos += urlLen;
            continue;
        }

        QString href = richText.mid(urlPos, urlLen);
        // we handle basket links separately...
        if (href.contains("basket://")) {
            urlPos += urlLen;
            continue;
        }
        // Qt doesn't support (?<=pattern) so we do it here
        if ((urlPos > 0) && richText[urlPos - 1].isLetterOrNumber()) {
            urlPos++;
            continue;
        }
        // Don't use QString::arg since %01, %20, etc could be in the string
        QString anchor = "<a href=\"" + href + "\">" + href + "</a>";
        richText.replace(urlPos, urlLen, anchor);
        urlPos += anchor.length();
    }
    return richText;
}

QString Tools::tagCrossReferences(const QString &text, bool userLink, HTMLExporter *exporter)
{
    QString richText(text);

    int urlPos = 0;
    int urlLen;

    QRegExp urlEx("\\[\\[(.+)\\]\\]");
    urlEx.setMinimal(true);
    while ((urlPos = urlEx.indexIn(richText, urlPos)) >= 0) {
        urlLen = urlEx.matchedLength();
        QString href = urlEx.cap(1);

        QStringList hrefParts = href.split('|');
        QString anchor;

        if (exporter) // if we're exporting this basket to html.
            anchor = crossReferenceForHtml(hrefParts, exporter);
        else if (userLink) // the link is manually created (ie [[/top level/sub]] )
            anchor = crossReferenceForConversion(hrefParts);
        else // otherwise it's a standard link (ie. [[basket://basket107]] )
            anchor = crossReferenceForBasket(hrefParts);

        richText.replace(urlPos, urlLen, anchor);
        urlPos += anchor.length();
    }
    return richText;
}

QString Tools::crossReferenceForBasket(QStringList linkParts)
{
    QString basketLink = linkParts.first();
    QString title;

    bool linkIsEmpty = false;

    if (basketLink == "basket://" || basketLink.isEmpty())
        linkIsEmpty = true;

    title = linkParts.last().trimmed();

    QString css = LinkLook::crossReferenceLook->toCSS("cross_reference", QColor());
    QString classes = "cross_reference";
    classes += (linkIsEmpty ? " xref_empty" : QString());

    css += (linkIsEmpty ? " a.xref_empty { display: block; width: 100%; text-decoration: underline; color: #CC2200; }"
                          " a:hover.xref_empty { color: #A55858; }"
                        : QString());

    QString anchor = "<style>" + css + "</style><a href=\"" + basketLink + "\" class=\"" + classes + "\">" + QUrl::fromPercentEncoding(title.toUtf8()) + "</a>";
    return anchor;
}

QString Tools::crossReferenceForHtml(QStringList linkParts, HTMLExporter *exporter)
{
    QString basketLink = linkParts.first();
    QString title;

    bool linkIsEmpty = false;

    if (basketLink == "basket://" || basketLink.isEmpty())
        linkIsEmpty = true;

    title = linkParts.last().trimmed();

    QString url;
    if (basketLink.startsWith(QLatin1String("basket://")))
        url = basketLink.mid(9, basketLink.length() - 9);

    BasketScene *basket = Global::bnpView->basketForFolderName(url);

    // remove the trailing slash.
    url = url.left(url.length() - 1);

    // if the basket we're trying to link to is the basket that was exported then
    // we have to use a special way to refer to it for the links.
    if (basket == exporter->exportedBasket)
        url = "../../" + exporter->fileName;
    else {
        // if we're in the exported basket then the links have to include
        // the sub directories.
        if (exporter->currentBasket == exporter->exportedBasket)
            url.prepend(exporter->basketsFolderName);
        if (!url.isEmpty())
            url.append(".html");
    }

    QString classes = "cross_reference";
    classes += (linkIsEmpty ? " xref_empty" : QString());

    QString css = (linkIsEmpty ? " a.xref_empty { display: block; width: 100%; text-decoration: underline; color: #CC2200; }"
                                 " a:hover.xref_empty { color: #A55858; }"
                               : QString());

    QString anchor = "<style>" + css + "</style><a href=\"" + url + "\" class=\"" + classes + "\">" + QUrl::fromPercentEncoding(title.toUtf8()) + "</a>";
    return anchor;
}

QString Tools::crossReferenceForConversion(QStringList linkParts)
{
    QString basketLink = linkParts.first();
    QString title;

    if (basketLink.startsWith(QLatin1String("basket://")))
        return QString("[[%1|%2]]").arg(basketLink, linkParts.last());

    if (basketLink.endsWith('/'))
        basketLink = basketLink.left(basketLink.length() - 1);

    QStringList pages = basketLink.split('/');

    if (linkParts.count() <= 1)
        title = pages.last();
    else
        title = linkParts.last().trimmed();

    QString url = Global::bnpView->folderFromBasketNameLink(pages);

    url.prepend("basket://");

    // if we don't change the link return it back exactly
    // as it came in because it may not be a link.
    if (url == "basket://" || url.isEmpty()) {
        return linkParts.join(QString()).prepend("[[").append("]]");
    } else {
        return QString("[[%1|%2]]").arg(url, title);
    }
}

QString Tools::htmlToText(const QString &html)
{
    QString text = htmlToParagraph(html);
    text.remove('\n');
    text.replace("</h1>", "\n");
    text.replace("</h2>", "\n");
    text.replace("</h3>", "\n");
    text.replace("</h4>", "\n");
    text.replace("</h5>", "\n");
    text.replace("</h6>", "\n");
    text.replace("</li>", "\n");
    text.replace("</dt>", "\n");
    text.replace("</dd>", "\n");
    text.replace("<dd>", "   ");
    text.replace("</div>", "\n");
    text.replace("</blockquote>", "\n");
    text.replace("</caption>", "\n");
    text.replace("</tr>", "\n");
    text.replace("</th>", "  ");
    text.replace("</td>", "  ");
    text.replace("<br>", "\n");
    text.replace("<br />", "\n");
    text.replace("</p>", "\n");
    // FIXME: Format <table> tags better, if possible
    // TODO: Replace &eacute; and co. by their equivalent!

    // To manage tags:
    int pos = 0;
    int pos2;
    QString tag, tag3;
    // To manage lists:
    int deep = 0;     // The deep of the current line in imbriqued lists
    QList<bool> ul;   // true if current list is a <ul> one, false if it's an <ol> one
    QList<int> lines; // The line number if it is an <ol> list
    // We're removing every other tags, or replace them in the case of li:
    while ((pos = text.indexOf("<"), pos) != -1) {
        // What is the current tag?
        tag = text.mid(pos + 1, 2);
        tag3 = text.mid(pos + 1, 3);
        // Lists work:
        if (tag == "ul") {
            deep++;
            ul.push_back(true);
            lines.push_back(-1);
        } else if (tag == "ol") {
            deep++;
            ul.push_back(false);
            lines.push_back(0);
        } else if (tag3 == "/ul" || tag3 == "/ol") {
            deep--;
            ul.pop_back();
            lines.pop_back();
        }
        // Where the tag closes?
        pos2 = text.indexOf(">");
        if (pos2 != -1) {
            // Remove the tag:
            text.remove(pos, pos2 - pos + 1);
            // And replace li with "* ", "x. "... without forbidding to indent that:
            if (tag == "li") {
                // How many spaces before the line (indentation):
                QString spaces;
                for (int i = 1; i < deep; i++)
                    spaces += QStringLiteral("  ");
                // The bullet or number of the line:
                QString bullet = "* ";
                if (ul.back() == false) {
                    lines.push_back(lines.back() + 1);
                    lines.pop_back();
                    bullet = QString::number(lines.back()) + ". ";
                }
                // Insertion:
                text.insert(pos, spaces + bullet);
            }
            if ((tag3 == "/ul" || tag3 == "/ol") && deep == 0)
                text.insert(pos, "\n"); // Empty line before and after a set of lists
        }
        ++pos;
    }

    text.replace("&gt;", ">");
    text.replace("&lt;", "<");
    text.replace("&quot;", "\"");
    text.replace("&nbsp;", " ");
    text.replace("&amp;", "&"); // CONVERT IN LAST!!

    // HtmlContent produces "\n" for empty note
    if (text == "\n")
        text = QString();

    return text;
}

QString Tools::textDocumentToMinimalHTML(QTextDocument *document)
{
    QString result =
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
        "<html><head><meta name=\"qrichtext\" content=\"1\" /><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><style type=\"text/css\">\n"
        "p, li { white-space: pre-wrap; margin: 0px; }\n"
        "</style></head><body>\n";
    QFont defaultFont;
    int fragCount, blockCount = 0;
    bool leadingBrNeeded = false;

    for (QTextBlock blockIt = document->begin(); blockIt != document->end(); blockIt = blockIt.next(), ++blockCount) {
        result += HTM::PAR;

        // Prepare to detect empty blocks
        fragCount = 0;

        for (QTextBlock::iterator subIt = blockIt.begin(); !(subIt.atEnd()); ++subIt, ++fragCount) {
            QTextFragment currentFragment = subIt.fragment();

            if (currentFragment.isValid()) {
                // Dealing with need to add leading linebreak (see later for
                // further notes)
                if (leadingBrNeeded) {
                    result += HTM::BR;
                    leadingBrNeeded = false;
                }

                QTextCharFormat charFmt = currentFragment.charFormat();
                const QColor &textColor = charFmt.foreground().color();
                bool isTextBlack = (textColor == QColor() || textColor == QColor(Qt::black));

                if (charFmt.font() == defaultFont && isTextBlack) {
                    result += currentFragment.text().toHtmlEscaped();
                    continue;
                }

                // If we use charFmt.fontWeight, setting a tag overrides it and all characters become non-bold.
                // So we use <b> </b> instead
                bool bold = (charFmt.fontWeight() >= QFont::Bold);
                if (bold)
                    result += "<b>";

                // Compose style string (font and color)
                result += "<span style=\"";

                if (charFmt.fontFamily() != defaultFont.family() && !charFmt.fontFamily().isEmpty())
                    result += QString(HTM::FONT_FAMILY).arg(charFmt.fontFamily());

                if (charFmt.fontItalic())
                    result += QString(HTM::FONT_STYLE).arg(HTM::ITALIC);
                if (charFmt.fontUnderline())
                    result += QString(HTM::TEXT_DECORATION).arg(HTM::UNDERLINE);
                if (charFmt.fontStrikeOut())
                    result += QString(HTM::TEXT_DECORATION).arg(HTM::LINE_THROUGH);

                /*if (charFmt.fontWeight() != defaultFont.weight()) {
                    QFont::Weight weight = (charFmt.fontWeight() >= QFont::Bold) ? QFont::Bold : QFont::Normal;
                    result += QString(HTM::FONT_WEIGHT).arg(weight);
                }*/

                if (charFmt.fontPointSize() != defaultFont.pointSize() && charFmt.fontPointSize() != 0)
                    result += QString(HTM::FONT_SIZE).arg(charFmt.fontPointSize());

                if (!isTextBlack)
                    result += QString(HTM::COLOR).arg(textColor.name());

                result += "\">" + currentFragment.text().toHtmlEscaped() + "</span>";

                if (bold)
                    result += "</b>";
            }
        }

        // Detecting empty blocks (Qt4 fails to generate a fragment from an empty line)
        // Inserting a linebreak directly here seems to cause the renderer to render
        // two breaks, so have to append it to the contents of the previous paragraph...
        if (!fragCount) {
            // If the first fragment is an empty fragment, the linebreak must be
            // added to the next fragment otherwise you get the above double breaks
            if (!blockCount)
                leadingBrNeeded = true;

            // Deal with the problem only when the last block is not affected,
            // otherwise you get double breaks again... Blocks counted from 0
            else if (blockCount != (document->blockCount() - 1)) {
                result.chop(7);
                result = result + HTM::BR + HTM::_PAR + HTM::PAR;
            }
        }

        result += HTM::_PAR;
    }

    result += "</body></html>";
    return result;
}

QString Tools::cssFontDefinition(const QFont &font, bool onlyFontFamily)
{
    // The font definition:
    QString definition = font.key() + (font.italic() ? QStringLiteral("italic ") : QString()) + (font.bold() ? QStringLiteral("bold ") : QString()) + QString::number(QFontInfo(font).pixelSize()) + QStringLiteral("px ");

    // Then, try to match the font name with a standard CSS font family:
    QString genericFont;
    if (definition.contains("serif", Qt::CaseInsensitive) || definition.contains("roman", Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("serif");
    }
    // No "else if" because "sans serif" must be counted as "sans". So, the order between "serif" and "sans" is important
    if (definition.contains("sans", Qt::CaseInsensitive) || definition.contains("arial", Qt::CaseInsensitive) || definition.contains("helvetica", Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("sans-serif");
    }
    if (definition.contains("mono", Qt::CaseInsensitive) || definition.contains("courier", Qt::CaseInsensitive) || definition.contains("typewriter", Qt::CaseInsensitive) || definition.contains("console", Qt::CaseInsensitive) ||
        definition.contains("terminal", Qt::CaseInsensitive) || definition.contains("news", Qt::CaseInsensitive)) {
        genericFont = QStringLiteral("monospace");
    }

    // Eventually add the generic font family to the definition:
    QString fontDefinition = QStringLiteral("\"%1\"").arg(font.family());
    if (!genericFont.isEmpty())
        fontDefinition += ", " + genericFont;

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

QString Tools::cssColorName(const QString& colorHex)
{
    static const QMap<QString, QString> cssColors = {
        {"#00ffff", "aqua"    },
        {"#000000", "black"   },
        {"#0000ff", "blue"    },
        {"#ff00ff", "fuchsia" },
        {"#808080", "gray"    },
        {"#008000", "green"   },
        {"#00ff00", "lime"    },
        {"#800000", "maroon"  },
        {"#000080", "navy"    },
        {"#808000", "olive"   },
        {"#800080", "purple"  },
        {"#ff0000", "red"     },
        {"#c0c0c0", "silver"  },
        {"#008080", "teal"    },
        {"#ffffff", "white"   },
        {"#ffff00", "yellow"  },
        // CSS extended colors
        {"#f0f8ff", "aliceblue"            },
        {"#faebd7", "antiquewhite"         },
        {"#7fffd4", "aquamarine"           },
        {"#f0ffff", "azure"                },
        {"#f5f5dc", "beige"                },
        {"#ffe4c4", "bisque"               },
        {"#ffebcd", "blanchedalmond"       },
        {"#8a2be2", "blueviolet"           },
        {"#a52a2a", "brown"                },
        {"#deb887", "burlywood"            },
        {"#5f9ea0", "cadetblue"            },
        {"#7fff00", "chartreuse"           },
        {"#d2691e", "chocolate"            },
        {"#ff7f50", "coral"                },
        {"#6495ed", "cornflowerblue"       },
        {"#fff8dc", "cornsilk"             },
        {"#dc1436", "crimson"              },
        {"#00ffff", "cyan"                 },
        {"#00008b", "darkblue"             },
        {"#008b8b", "darkcyan"             },
        {"#b8860b", "darkgoldenrod"        },
        {"#a9a9a9", "darkgray"             },
        {"#006400", "darkgreen"            },
        {"#bdb76b", "darkkhaki"            },
        {"#8b008b", "darkmagenta"          },
        {"#556b2f", "darkolivegreen"       },
        {"#ff8c00", "darkorange"           },
        {"#9932cc", "darkorchid"           },
        {"#8b0000", "darkred"              },
        {"#e9967a", "darksalmon"           },
        {"#8fbc8f", "darkseagreen"         },
        {"#483d8b", "darkslateblue"        },
        {"#2f4f4f", "darkslategray"        },
        {"#00ced1", "darkturquoise"        },
        {"#9400d3", "darkviolet"           },
        {"#ff1493", "deeppink"             },
        {"#00bfff", "deepskyblue"          },
        {"#696969", "dimgray"              },
        {"#1e90ff", "dodgerblue"           },
        {"#b22222", "firebrick"            },
        {"#fffaf0", "floralwhite"          },
        {"#228b22", "forestgreen"          },
        {"#dcdcdc", "gainsboro"            },
        {"#f8f8ff", "ghostwhite"           },
        {"#ffd700", "gold"                 },
        {"#daa520", "goldenrod"            },
        {"#adff2f", "greenyellow"          },
        {"#f0fff0", "honeydew"             },
        {"#ff69b4", "hotpink"              },
        {"#cd5c5c", "indianred"            },
        {"#4b0082", "indigo"               },
        {"#fffff0", "ivory"                },
        {"#f0e68c", "khaki"                },
        {"#e6e6fa", "lavender"             },
        {"#fff0f5", "lavenderblush"        },
        {"#7cfc00", "lawngreen"            },
        {"#fffacd", "lemonchiffon"         },
        {"#add8e6", "lightblue"            },
        {"#f08080", "lightcoral"           },
        {"#e0ffff", "lightcyan"            },
        {"#fafad2", "lightgoldenrodyellow" },
        {"#90ee90", "lightgreen"           },
        {"#d3d3d3", "lightgrey"            },
        {"#ffb6c1", "lightpink"            },
        {"#ffa07a", "lightsalmon"          },
        {"#20b2aa", "lightseagreen"        },
        {"#87cefa", "lightskyblue"         },
        {"#778899", "lightslategray"       },
        {"#b0c4de", "lightsteelblue"       },
        {"#ffffe0", "lightyellow"          },
        {"#32cd32", "limegreen"            },
        {"#faf0e6", "linen"                },
        {"#ff00ff", "magenta"              },
        {"#66cdaa", "mediumaquamarine"     },
        {"#0000cd", "mediumblue"           },
        {"#ba55d3", "mediumorchid"         },
        {"#9370db", "mediumpurple"         },
        {"#3cb371", "mediumseagreen"       },
        {"#7b68ee", "mediumslateblue"      },
        {"#00fa9a", "mediumspringgreen"    },
        {"#48d1cc", "mediumturquoise"      },
        {"#c71585", "mediumvioletred"      },
        {"#191970", "midnightblue"         },
        {"#f5fffa", "mintcream"            },
        {"#ffe4e1", "mistyrose"            },
        {"#ffe4b5", "moccasin"             },
        {"#ffdead", "navajowhite"          },
        {"#fdf5e6", "oldlace"              },
        {"#6b8e23", "olivedrab"            },
        {"#ffa500", "orange"               },
        {"#ff4500", "orangered"            },
        {"#da70d6", "orchid"               },
        {"#eee8aa", "palegoldenrod"        },
        {"#98fb98", "palegreen"            },
        {"#afeeee", "paleturquoise"        },
        {"#db7093", "palevioletred"        },
        {"#ffefd5", "papayawhip"           },
        {"#ffdab9", "peachpuff"            },
        {"#cd853f", "peru"                 },
        {"#ffc0cb", "pink"                 },
        {"#dda0dd", "plum"                 },
        {"#b0e0e6", "powderblue"           },
        {"#bc8f8f", "rosybrown"            },
        {"#4169e1", "royalblue"            },
        {"#8b4513", "saddlebrown"          },
        {"#fa8072", "salmon"               },
        {"#f4a460", "sandybrown"           },
        {"#2e8b57", "seagreen"             },
        {"#fff5ee", "seashell"             },
        {"#a0522d", "sienna"               },
        {"#87ceeb", "skyblue"              },
        {"#6a5acd", "slateblue"            },
        {"#708090", "slategray"            },
        {"#fffafa", "snow"                 },
        {"#00ff7f", "springgreen"          },
        {"#4682b4", "steelblue"            },
        {"#d2b48c", "tan"                  },
        {"#d8bfd8", "thistle"              },
        {"#ff6347", "tomato"               },
        {"#40e0d0", "turquoise"            },
        {"#ee82ee", "violet"               },
        {"#f5deb3", "wheat"                },
        {"#f5f5f5", "whitesmoke"           },
        {"#9acd32", "yellowgreen"          } };

    return cssColors.value(colorHex, QString());
}


bool Tools::isWebColor(const QColor &color)
{
    int r = color.red();   // The 216 web colors are those colors whose RGB (Red, Green, Blue)
    int g = color.green(); //  values are all in the set (0, 51, 102, 153, 204, 255).
    int b = color.blue();

    return ((r == 0 || r == 51 || r == 102 || r == 153 || r == 204 || r == 255) &&
            (g == 0 || g == 51 || g == 102 || g == 153 || g == 204 || g == 255) &&
            (b == 0 || b == 51 || b == 102 || b == 153 || b == 204 || b == 255));
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
            if (*it != "." && *it != "..")
                deleteRecursively(folderOrFile + '/' + *it);
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
            if (*it != "." && *it != "..")
                deleteMetadataRecursively(folderOrFile + '/' + *it);
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
    int extIndex = fileName.lastIndexOf('.');
    if (extIndex != -1 && extIndex != int(fileName.length() - 1)) { // Extension found and fileName do not ends with '.' !
        extension = fileName.mid(extIndex);
        fileName.truncate(extIndex);
    } // else fileName = fileName and extension = QString()

    // Find the file number, if it exists : Split fileName in fileName and number
    // Example : fileName == "note5-3" => fileName = "note5" and number = 3
    int extNumber = fileName.lastIndexOf('-');
    if (extNumber != -1 && extNumber != int(fileName.length() - 1)) { // Number found and fileName do not ends with '-' !
        bool isANumber;
        int theNumber = fileName.mid(extNumber + 1).toInt(&isANumber);
        if (isANumber) {
            number = theNumber;
            fileName.truncate(extNumber);
        } // else :
    }     // else fileName = fileName and number = 2 (because if the file already exists, the generated name is at last the 2nd)

    QString finalName;
    for (/*int number = 2*/;; ++number) { // TODO: FIXME: If overflow ???
        finalName = fileName + '-' + QString::number(number) + extension;
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
        foreach (const QFileInfo &child, children)
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
    if (source->hasFormat("application/x-kde-cutselection")) {
        QByteArray array = source->data("application/x-kde-cutselection");
        return !array.isEmpty() && QByteArray(array.data(), array.size() + 1).at(0) == '1';
    } else
        return false;
}

void Tools::printChildren(QObject *parent)
{
    const QObjectList objs = parent->children();
    QObject *obj;
    for (int i = 0; i < objs.size(); i++) {
        obj = objs.at(i);
        qDebug() << Q_FUNC_INFO << obj->metaObject()->className() << ": " << obj->objectName() << endl;
    }
}

QString Tools::makeStandardCaption(const QString &userCaption)
{
    QString caption = QGuiApplication::applicationDisplayName();

    if (!userCaption.isEmpty())
        return userCaption + i18nc("Document/application separator in titlebar", " – ") + caption;
    else
        return caption;
}

QByteArray Tools::systemCodeset()
{
    QByteArray codeset;
#if HAVE_LANGINFO_H
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
    return codeset;
}
