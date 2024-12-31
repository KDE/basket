/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "htmlexporter.h"

#include "basketlistview.h"
#include "basketscene.h"
#include "bnpview.h"
#include "common.h"
#include "config.h"
#include "linklabel.h"
#include "note.h"
#include "notecontent.h"
#include "tools.h"

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>

#include <KIO/CopyJob> //For KIO::copy
#include <KIO/FileCopyJob> //For KIO::file_copy
#include <KIO/Job>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QList>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QProgressDialog>
#include <QTextStream>

HTMLExporter::HTMLExporter(BasketScene *basket)
    : dialog(new QProgressDialog())
{
    QDir dir;

    // Compute a default file name & path:
    KConfigGroup config = Global::config()->group(QStringLiteral("Export to HTML"));
    QString folder = config.readEntry("lastFolder", QDir::homePath()) + QLatin1Char('/');
    QString url = folder + QString(basket->basketName()).replace(QLatin1Char('/'), QLatin1Char('_')) + QStringLiteral(".html");

    // Ask a file name & path to the user:
    QString filter = QStringLiteral("*.html *.htm|") + i18n("HTML Documents") + QStringLiteral("\n*|") + i18n("All Files");
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        // Ask:
        destination = QFileDialog::getSaveFileName(nullptr, i18n("Export to HTML"), destination, filter);
        // User canceled?
        if (destination.isEmpty())
            return;
        // File already existing? Ask for overriding:
        if (dir.exists(destination)) {
            int result = KMessageBox::questionTwoActionsCancel(
                nullptr,
                QStringLiteral("<qt>")
                    + i18n("The file <b>%1</b> already exists. Do you really want to overwrite it?", QUrl::fromLocalFile(destination).fileName()),
                i18n("Overwrite File?"),
                KGuiItem(i18n("&Overwrite"), QStringLiteral("document-save")),
                KStandardGuiItem::discard());
            if (result == KMessageBox::Cancel)
                return;
            else if (result == KMessageBox::Ok)
                askAgain = false;
        } else
            askAgain = false;
    }

    // Create the progress dialog that will always be shown during the export:
    dialog->setWindowTitle(i18n("Export to HTML"));
    dialog->setLabelText(i18n("Exporting to HTML. Please wait..."));
    dialog->setCancelButton(nullptr);
    dialog->setAutoClose(true);
    dialog->show();

    // Remember the last folder used for HTML exploration:
    config.writeEntry("lastFolder", QUrl::fromLocalFile(destination).adjusted(QUrl::RemoveFilename).path());
    config.sync();

    prepareExport(basket, destination);
    exportBasket(basket, /*isSubBasketScene*/ false);

    dialog->setValue(dialog->value() + 1); // Finishing finished
}

HTMLExporter::~HTMLExporter()
{
}

void HTMLExporter::prepareExport(BasketScene *basket, const QString &fullPath)
{
    dialog->setRange(0,
                     /*Preparation:*/ 1 + /*Finishing:*/ 1 + /*Basket:*/ 1
                         + /*SubBaskets:*/ Global::bnpView->basketCount(Global::bnpView->listViewItemForBasket(basket)));
    dialog->setValue(0);
    qApp->processEvents();

    // Remember the file path chosen by the user:
    filePath = fullPath;
    fileName = QUrl::fromLocalFile(fullPath).fileName();
    exportedBasket = basket;
    currentBasket = nullptr;

    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    withBasketTree = (item->childCount() >= 0);

    // Create and empty the files folder:
    QString filesFolderPath = i18nc("HTML export folder (files)", "%1_files", filePath) + QLatin1Char('/'); // eg.: "/home/seb/foo.html_files/"
    Tools::deleteRecursively(filesFolderPath);
    QDir dir;
    dir.mkdir(filesFolderPath);

    // Create sub-folders:
    iconsFolderPath = filesFolderPath + i18nc("HTML export folder (icons)", "icons") + QLatin1Char('/'); // eg.: "/home/seb/foo.html_files/icons/"
    imagesFolderPath = filesFolderPath + i18nc("HTML export folder (images)", "images") + QLatin1Char('/'); // eg.: "/home/seb/foo.html_files/images/"
    basketsFolderPath = filesFolderPath + i18nc("HTML export folder (baskets)", "baskets") + QLatin1Char('/'); // eg.: "/home/seb/foo.html_files/baskets/"
    dir.mkdir(iconsFolderPath);
    dir.mkdir(imagesFolderPath);
    dir.mkdir(basketsFolderPath);

    dialog->setValue(dialog->value() + 1); // Preparation finished
}

void HTMLExporter::exportBasket(BasketScene *basket, bool isSubBasket)
{
    if (!basket->isLoaded()) {
        basket->load();
    }

    currentBasket = basket;

    bool hasBackgroundColor = false;
    bool hasTextColor = false;

    if (basket->backgroundColorSetting().isValid()) {
        hasBackgroundColor = true;
        backgroundColorName = basket->backgroundColor().name().toLower().mid(1);
    }
    if (basket->textColorSetting().isValid()) {
        hasTextColor = true;
    }

    // Compute the absolute & relative paths for this basket:
    filesFolderPath = i18nc("HTML export folder (files)", "%1_files", filePath) + QLatin1Char('/');
    if (isSubBasket) {
        basketFilePath = basketsFolderPath + basket->folderName().left(basket->folderName().length() - 1) + QStringLiteral(".html");
        filesFolderName = QStringLiteral("../");
        dataFolderName =
            basket->folderName().left(basket->folderName().length() - 1) + QLatin1Char('-') + i18nc("HTML export folder (data)", "data") + QLatin1Char('/');
        dataFolderPath = basketsFolderPath + dataFolderName;
        basketsFolderName = QString();
    } else {
        basketFilePath = filePath;
        filesFolderName = i18nc("HTML export folder (files)", "%1_files", QUrl::fromLocalFile(filePath).fileName()) + QLatin1Char('/');
        dataFolderName = filesFolderName + i18nc("HTML export folder (data)", "data") + QLatin1Char('/');
        dataFolderPath = filesFolderPath + i18nc("HTML export folder (data)", "data") + QLatin1Char('/');
        basketsFolderName = filesFolderName + i18nc("HTML export folder (baskets)", "baskets") + QLatin1Char('/');
    }
    iconsFolderName = (isSubBasket ? QStringLiteral("../") : filesFolderName) + i18nc("HTML export folder (icons)", "icons")
        + QLatin1Char('/'); // eg.: "foo.html_files/icons/"   or "../icons/"
    imagesFolderName = (isSubBasket ? QStringLiteral("../") : filesFolderName) + i18nc("HTML export folder (images)", "images")
        + QLatin1Char('/'); // eg.: "foo.html_files/images/"  or "../images/"

    qDebug() << "Exporting ================================================";
    qDebug() << "  filePath:" << filePath;
    qDebug() << "  basketFilePath:" << basketFilePath;
    qDebug() << "  filesFolderPath:" << filesFolderPath;
    qDebug() << "  filesFolderName:" << filesFolderName;
    qDebug() << "  iconsFolderPath:" << iconsFolderPath;
    qDebug() << "  iconsFolderName:" << iconsFolderName;
    qDebug() << "  imagesFolderPath:" << imagesFolderPath;
    qDebug() << "  imagesFolderName:" << imagesFolderName;
    qDebug() << "  dataFolderPath:" << dataFolderPath;
    qDebug() << "  dataFolderName:" << dataFolderName;
    qDebug() << "  basketsFolderPath:" << basketsFolderPath;
    qDebug() << "  basketsFolderName:" << basketsFolderName;

    // Create the data folder for this basket:
    QDir dir;
    dir.mkdir(dataFolderPath);

    // Generate basket icons:
    QString basketIcon16 = iconsFolderName + copyIcon(basket->icon(), 16);
    QString basketIcon32 = iconsFolderName + copyIcon(basket->icon(), 32);

    // Generate the [+] image for groups:
    QPixmap expandGroup(Note::EXPANDER_WIDTH, Note::EXPANDER_HEIGHT);
    if (hasBackgroundColor) {
        expandGroup.fill(basket->backgroundColor());
    } else {
        expandGroup.fill(QColor(Qt::GlobalColor::transparent));
    }
    QPainter painter(&expandGroup);
    if (hasBackgroundColor) {
        Note::drawExpander(&painter, 0, 0, basket->backgroundColor(), /*expand=*/true, basket);
    } else {
        Note::drawExpander(&painter, 0, 0, QColor(Qt::GlobalColor::transparent), /*expand=*/true, basket);
    }
    painter.end();
    if (hasBackgroundColor) {
        expandGroup.save(imagesFolderPath + QStringLiteral("expand_group_") + backgroundColorName + QStringLiteral(".png"), "PNG");
    } else {
        expandGroup.save(imagesFolderPath + QStringLiteral("expand_group_transparent.png"), "PNG");
    }

    // Generate the [-] image for groups:
    QPixmap foldGroup(Note::EXPANDER_WIDTH, Note::EXPANDER_HEIGHT);
    if (hasBackgroundColor) {
        foldGroup.fill(basket->backgroundColor());
    } else {
        foldGroup.fill(QColor(Qt::GlobalColor::transparent));
    }
    painter.begin(&foldGroup);
    if (hasBackgroundColor) {
        Note::drawExpander(&painter, 0, 0, basket->backgroundColor(), /*expand=*/false, basket);
    } else {
        Note::drawExpander(&painter, 0, 0, QColor(Qt::GlobalColor::transparent), /*expand=*/false, basket);
    }

    painter.end();
    if (hasBackgroundColor) {
        foldGroup.save(imagesFolderPath + QStringLiteral("fold_group_") + backgroundColorName + QStringLiteral(".png"), "PNG");
    } else {
        foldGroup.save(imagesFolderPath + QStringLiteral("fold_group_transparent.png"), "PNG");
    }

    // Open the file to write:
    QFile file(basketFilePath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    stream.setDevice(&file);

    // Output the header:
    QString borderColor;
    if (hasBackgroundColor && hasTextColor) {
        borderColor = Tools::mixColor(basket->backgroundColor(), basket->textColor()).name();
    } else if (hasBackgroundColor) {
        borderColor = Tools::mixColor(basket->backgroundColor(), QColor(Qt::GlobalColor::black)).name();
    } else if (hasTextColor) {
        borderColor = Tools::mixColor(QColor(Qt::GlobalColor::white), basket->textColor()).name();
    }
    stream << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
              "<html>\n"
              " <head>\n"
              "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
              "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><meta name=\"Generator\" content=\""
           << QGuiApplication::applicationDisplayName() << " " << VERSION << " " << KAboutData::applicationData().homepage()
           << "\">\n"
              "  <style type=\"text/css\">\n"
              //      "   @media print {\n"
              //      "    span.printable { display: inline; }\n"
              //      "   }\n"
              "   body { margin: 10px; font: 11px sans-serif; }\n" // TODO: Use user font
              "   h1 { text-align: center; }\n"
              "   img { border: none; vertical-align: middle; }\n";
    if (withBasketTree) {
        stream << "   .tree { margin: 0; padding: 1px 0 1px 1px; width: 150px; _width: 149px; overflow: hidden; float: left; }\n"
                  "   .tree ul { margin: 0 0 0 10px; padding: 0; }\n"
                  "   .tree li { padding: 0; margin: 0; list-style: none; }\n"
                  "   .tree a { display: block; padding: 1px; height: 16px; text-decoration: none;\n"
                  "             white-space: nowrap; word-wrap: normal; text-wrap: suppress; color: black; }\n"
                  "   .tree span { -moz-border-radius: 6px; display: block; float: left;\n"
                  "                line-height: 16px; height: 16px; vertical-align: middle; padding: 0 1px; }\n"
                  "   .tree img { vertical-align: top; padding-right: 1px; }\n"
                  "   .tree .current { background-color: "
               << qApp->palette().color(QPalette::Highlight).name()
               << "; "
                  "-moz-border-radius: 3px 0 0 3px; border-radius: 3px 0 0 3px; color: "
               << qApp->palette().color(QPalette::Highlight).name()
               << "; }\n"
                  "   .basketSurrounder { margin-left: 152px; _margin: 0; _float: right; }\n";
    }

    stream << "   .basket { ";
    if (hasBackgroundColor) {
        stream << "background-color: " << basket->backgroundColor().name() << "; ";
    }
    stream << "border: solid " << borderColor
           << " 1px; "
              "font: "
           << Tools::cssFontDefinition(basket->QGraphicsScene::font()) << "; ";
    if (hasTextColor) {
        stream << "color: " << basket->textColor().name() << "; ";
    }
    stream << "padding: 1px; width: 100%; }\n"
              "   table.basket { border-collapse: collapse; }\n"
              "   .basket * { padding: 0; margin: 0; }\n"
              "   .basket table { width: 100%; border-spacing: 0; _border-collapse: collapse; }\n"
              "   .column { vertical-align: top; }\n"
              "   .columnHandle { width: "
           << Note::RESIZER_WIDTH << "px; background: transparent url('" << imagesFolderName << "column_handle_";
    if (hasBackgroundColor) {
        stream << backgroundColorName;
    } else {
        stream << "transparent";
    }
    stream << ".png') repeat-y; }\n"
              "   .group { margin: 0; padding: 0; border-collapse: collapse; width: 100% }\n"
              "   .groupHandle { margin: 0; width: "
           << Note::GROUP_WIDTH
           << "px; text-align: center; }\n"
              "   .note { padding: 1px 2px; ";
    if (hasBackgroundColor) {
        stream << "background-color : " << basket->backgroundColor().name() << "; ";
    }
    stream << "width: 100%; }\n"
              "   .tags { width: 1px; white-space: nowrap; }\n"
              "   .tags img { padding-right: 2px; }\n";
    if (hasTextColor) {
        stream << LinkLook::soundLook->toCSS(QStringLiteral("sound"), basket->textColor())
               << LinkLook::fileLook->toCSS(QStringLiteral("file"), basket->textColor())
               << LinkLook::localLinkLook->toCSS(QStringLiteral("local"), basket->textColor())
               << LinkLook::networkLinkLook->toCSS(QStringLiteral("network"), basket->textColor())
               << LinkLook::launcherLook->toCSS(QStringLiteral("launcher"), basket->textColor())
               << LinkLook::crossReferenceLook->toCSS(QStringLiteral("cross_reference"), basket->textColor());
    } else {
        stream << LinkLook::soundLook->toCSS(QStringLiteral("sound"), QColor(Qt::GlobalColor::black))
               << LinkLook::fileLook->toCSS(QStringLiteral("file"), QColor(Qt::GlobalColor::black))
               << LinkLook::localLinkLook->toCSS(QStringLiteral("local"), QColor(Qt::GlobalColor::black))
               << LinkLook::networkLinkLook->toCSS(QStringLiteral("network"), QColor(Qt::GlobalColor::black))
               << LinkLook::launcherLook->toCSS(QStringLiteral("launcher"), QColor(Qt::GlobalColor::black))
               << LinkLook::crossReferenceLook->toCSS(QStringLiteral("cross_reference"), QColor(Qt::GlobalColor::black));
    }
    stream << "   .unknown { margin: 1px 2px; border: 1px solid " << borderColor << "; -moz-border-radius: 4px; }\n";
    QList<State *> states = basket->usedStates();
    QString statesCss;
    for (State::List::Iterator it = states.begin(); it != states.end(); ++it)
        statesCss += (*it)->toCSS(imagesFolderPath, imagesFolderName, basket->QGraphicsScene::font());
    stream << statesCss << "   .credits { text-align: right; margin: 3px 0 0 0; _margin-top: -17px; font-size: 80%; color: " << borderColor
           << "; }\n"
              "  </style>\n"
              "  <title>"
           << Tools::textToHTMLWithoutP(basket->basketName())
           << "</title>\n"
              "  <link rel=\"shortcut icon\" type=\"image/png\" href=\""
           << basketIcon16 << "\">\n";
    // Create the column handle image:
    QPixmap columnHandle(Note::RESIZER_WIDTH, 50);
    painter.begin(&columnHandle);
    if (hasBackgroundColor) {
        Note::drawInactiveResizer(&painter, 0, 0, columnHandle.height(), basket->backgroundColor(), /*column=*/true);
    } else {
        Note::drawInactiveResizer(&painter, 0, 0, columnHandle.height(), QColor(Qt::GlobalColor::black), /*column=*/true);
    }
    painter.end();

    if (hasBackgroundColor) {
        columnHandle.save(imagesFolderPath + QStringLiteral("column_handle_") + backgroundColorName + QStringLiteral(".png"), "PNG");
    } else {
        columnHandle.save(imagesFolderPath + QStringLiteral("column_handle_transparent.png"), "PNG");
    }

    stream << " </head>\n"
              " <body>\n"
              "  <h1><img src=\""
           << QUrl(basketIcon32).toString() << "\" width=\"32\" height=\"32\" alt=\"\"> " << Tools::textToHTMLWithoutP(basket->basketName()) << "</h1>\n";

    if (withBasketTree)
        writeBasketTree(basket);

    // If filtering, only export filtered notes, inform to the user:
    // TODO: Filtering tags too!!
    // TODO: Make sure only filtered notes are exported!
    //  if (decoration()->filterData().isFiltering)
    //      stream <<
    //          "  <p>" << i18n("Notes matching the filter &quot;%1&quot;:", Tools::textToHTMLWithoutP(decoration()->filterData().string)) << "</p>\n";

    stream << "  <div class=\"basketSurrounder\">\n";

    stream << "   <div class=\"basket\" style=\"position: relative; min-width: 100%; min-height: calc(100vh - 100px); ";
    if (!basket->isColumnsLayout()) {
        stream << "height: " << basket->sceneRect().height() << "px; width: " << basket->sceneRect().width() << "px; ";
    }
    stream << "\">\n";

    if (basket->isColumnsLayout()) {
        stream << "   <table>\n"
                  "    <tr>\n";
    }

    for (Note *note = basket->firstNote(); note; note = note->next()) {
        exportNote(note, /*indent=*/(basket->isFreeLayout() ? 4 : 5));
    }

    // Output the footer:
    if (basket->isColumnsLayout()) {
        stream << "    </tr>\n"
                  "   </table>\n";
    }

    stream << "   </div>\n";

    stream << QStringLiteral(
                  "  </div>\n"
                  "  <p class=\"credits\">%1</p>\n")
                  .arg(i18n("Made with <a href=\"%1\">%2</a> %3, a tool to take notes and keep information at hand.",
                            KAboutData::applicationData().homepage(),
                            QGuiApplication::applicationDisplayName(),
                            QStringLiteral(VERSION)));

    stream << " </body>\n"
              "</html>\n";

    file.close();
    stream.setDevice(nullptr);
    dialog->setValue(dialog->value() + 1); // Basket exportation finished

    // Recursively export child baskets:
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (item->childCount() >= 0) {
        for (int i = 0; i < item->childCount(); i++) {
            exportBasket(((BasketListViewItem *)item->child(i))->basket(), /*isSubBasket=*/true);
        }
    }
}

void HTMLExporter::exportNote(Note *note, int indent)
{
    QString spaces;

    if (note->isColumn()) {
        QString width;
        if (false /*TODO: DEBUG AND REENABLE: hasResizer()*/) {
            // As we cannot be precise in CSS (say eg. "width: 50%-40px;"),
            // we output a percentage that is approximately correct.
            // For instance, we compute the currently used percentage of width in the basket
            // and try make it the same on a 1024*768 display in a Web browser:
            int availableSpaceForColumnsInThisBasket = note->basket()->sceneRect().width() - (note->basket()->columnsCount() - 1) * Note::RESIZER_WIDTH;
            int availableSpaceForColumnsInBrowser = 1024 /* typical screen width */
                - 25 /* window border and scrollbar width */
                - 2 * 5 /* page margin */
                - (note->basket()->columnsCount() - 1) * Note::RESIZER_WIDTH;
            if (availableSpaceForColumnsInThisBasket <= 0)
                availableSpaceForColumnsInThisBasket = 1;
            int widthValue = (int)(availableSpaceForColumnsInBrowser * (double)note->groupWidth() / availableSpaceForColumnsInThisBasket);
            if (widthValue <= 0)
                widthValue = 1;
            if (widthValue > 100)
                widthValue = 100;
            width = QStringLiteral(" width=\"%1%\"").arg(QString::number(widthValue));
        }
        stream << spaces.fill(QLatin1Char(' '), indent) << "<td class=\"column\"" << width << ">\n";

        // Export child notes:
        for (Note *child = note->firstChild(); child; child = child->next()) {
            stream << spaces.fill(QLatin1Char(' '), indent + 1);
            exportNote(child, indent + 1);
            stream << '\n';
        }

        stream << spaces.fill(QLatin1Char(' '), indent) << "</td>\n";
        if (note->hasResizer())
            stream << spaces.fill(QLatin1Char(' '), indent) << "<td class=\"columnHandle\"></td>\n";
        return;
    }

    QString freeStyle;
    if (note->isFree())
        freeStyle = QStringLiteral(" style=\"position: absolute; left: ") + QString::number(note->x()) + QStringLiteral("px; top: ")
            + QString::number(note->y()) + QStringLiteral("px; width: ") + QString::number(note->groupWidth()) + QStringLiteral("px\"");

    if (note->isGroup()) {
        stream << '\n'
               << spaces.fill(QLatin1Char(' '), indent) << "<table" << freeStyle
               << ">\n"; // Note content is expected to be on the same HTML line, but NOT groups
        int i = 0;
        for (Note *child = note->firstChild(); child; child = child->next()) {
            stream << spaces.fill(QLatin1Char(' '), indent);
            if (i == 0)
                stream << " <tr><td class=\"groupHandle\"><img src=\"" << QUrl(imagesFolderName).toString()
                       << (note->isFolded() ? "expand_group_" : "fold_group_") << backgroundColorName << ".png"
                       << "\" width=\"" << Note::EXPANDER_WIDTH << "\" height=\"" << Note::EXPANDER_HEIGHT << "\"></td>\n";
            else if (i == 1)
                stream << " <tr><td class=\"freeSpace\" rowspan=\"" << note->countDirectChilds() << "\"></td>\n";
            else
                stream << " <tr>\n";
            stream << spaces.fill(QLatin1Char(' '), indent) << "  <td>";
            exportNote(child, indent + 3);
            stream << "</td>\n" << spaces.fill(QLatin1Char(' '), indent) << " </tr>\n";
            ++i;
        }
        stream << '\n' << spaces.fill(QLatin1Char(' '), indent) << "</table>\n" /*<< spaces.fill(' ', indent - 1)*/;
    } else {
        // Additional class for the content (link, netword, color...):
        QString additionalClasses = note->content()->cssClass();
        if (!additionalClasses.isEmpty())
            additionalClasses = QLatin1Char(' ') + additionalClasses;
        // Assign the style of each associated tags:
        for (State::List::Iterator it = note->states().begin(); it != note->states().end(); ++it)
            additionalClasses += QStringLiteral(" tag_") + (*it)->id();
        // stream << spaces.fill(' ', indent);
        stream << "<table class=\"note" << additionalClasses << "\"" << freeStyle << "><tr>";
        if (note->emblemsCount() > 0) {
            stream << "<td class=\"tags\"><nobr>";
            for (State::List::Iterator it = note->states().begin(); it != note->states().end(); ++it)
                if (!(*it)->emblem().isEmpty()) {
                    int emblemSize = 16;
                    QString iconFileName = copyIcon((*it)->emblem(), emblemSize);
                    stream << "<img src=\"" << QUrl(iconsFolderName + iconFileName).toString() << "\" width=\"" << emblemSize << "\" height=\"" << emblemSize
                           << "\" alt=\"" << (*it)->textEquivalent() << "\" title=\"" << (*it)->fullName() << "\">";
                }
            stream << "</nobr></td>";
        }
        stream << "<td>";
        note->content()->exportToHTML(this, indent);
        stream << "</td></tr></table>";
    }
}

void HTMLExporter::writeBasketTree(BasketScene *currentBasket)
{
    stream << "  <ul class=\"tree\">\n";
    writeBasketTree(currentBasket, exportedBasket, 3);
    stream << "  </ul>\n";
}

void HTMLExporter::writeBasketTree(BasketScene *currentBasket, BasketScene *basket, int indent)
{
    // Compute variable HTML code:
    QString spaces;
    QString cssClass = (basket == currentBasket ? QStringLiteral(" class=\"current\"") : QString());
    QString link(QLatin1Char('#'));
    if (currentBasket != basket) {
        if (currentBasket == exportedBasket) {
            link = basketsFolderName + basket->folderName().left(basket->folderName().length() - 1) + QStringLiteral(".html");
        } else if (basket == exportedBasket) {
            link = QStringLiteral("../../") + fileName;
        } else {
            link = basket->folderName().left(basket->folderName().length() - 1) + QStringLiteral(".html");
        }
    }
    QString spanStyle = QStringLiteral(" style=\"");
    if (basket->backgroundColorSetting().isValid()) {
        spanStyle += QStringLiteral("background-color: ") + basket->backgroundColor().name() + QStringLiteral("; ");
    }
    if (basket->textColorSetting().isValid()) {
        spanStyle += QStringLiteral("color: ") + basket->textColor().name() + QStringLiteral("; ");
    }
    spanStyle += QStringLiteral("\"");

    // Write the basket tree line:
    stream << spaces.fill(QLatin1Char(' '), indent) << "<li><a" << cssClass << " href=\"" << link
           << "\">"
              "<span"
           << spanStyle << " title=\"" << Tools::textToHTMLWithoutP(basket->basketName())
           << "\">"
              "<img src=\""
           << iconsFolderName << copyIcon(basket->icon(), 16) << "\" width=\"16\" height=\"16\" alt=\"\">" << Tools::textToHTMLWithoutP(basket->basketName())
           << "</span></a>";

    // Write the sub-baskets lines & end the current one:
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (item->childCount() >= 0) {
        stream << "\n" << spaces.fill(QLatin1Char(' '), indent) << " <ul>\n";
        for (int i = 0; i < item->childCount(); i++)
            writeBasketTree(currentBasket, ((BasketListViewItem *)item->child(i))->basket(), indent + 2);
        stream << spaces.fill(QLatin1Char(' '), indent) << " </ul>\n" << spaces.fill(QLatin1Char(' '), indent) << "</li>\n";
    } else {
        stream << "</li>\n";
    }
}

/** Save an icon to a folder.
 * If an icon with the same name already exist in the destination,
 * it is assumed the icon is already copied, so no action is took.
 * It is optimized so that you can have an empty folder receiving the icons
 * and call copyIcon() each time you encounter one during export process.
 */
QString HTMLExporter::copyIcon(const QString &iconName, int size)
{
    if (iconName.isEmpty()) {
        return QString();
    }

    // Sometimes icon can be "favicons/www.kde.org", we replace the '/' with a '_'
    QString fileName = iconName; // QString::replace() isn't const, so I must copy the string before
    fileName = QStringLiteral("ico") + QString::number(size) + QLatin1Char('_') + fileName.replace(QLatin1Char('/'), QLatin1Char('_')) + QStringLiteral(".png");
    QString fullPath = iconsFolderPath + fileName;
    if (!QFile::exists(fullPath)) {
        KIconLoader::global()->loadIcon(iconName, KIconLoader::Desktop, size).save(fullPath, "PNG");
    }
    return fileName;
}

/** Done: Sometimes we can call two times copyFile() with the same srcPath and dataFolderPath
 *       (eg. when exporting basket to HTML with two links to same filename
 *            (but not necesary same path, as in "/home/foo.txt" and "/foo.txt") )
 *       The first copy isn't yet started, so the dest file isn't created and this method
 *       returns the same filename !!!!!!!!!!!!!!!!!!!!
 */
QString HTMLExporter::copyFile(const QString &srcPath, bool createIt)
{
    QString fileName = Tools::fileNameForNewFile(QUrl::fromLocalFile(srcPath).fileName(), dataFolderPath);
    QString fullPath = dataFolderPath + fileName;

    if (!currentBasket->isEncrypted()) {
        if (createIt) {
            // We create the file to be sure another very near call to copyFile() willn't choose the same name:
            QFile file(QUrl::fromLocalFile(fullPath).path());
            if (file.open(QIODevice::WriteOnly))
                file.close();
            // And then we copy the file AND overwriting the file we just created:
            KIO::file_copy(QUrl::fromLocalFile(srcPath), QUrl::fromLocalFile(fullPath), 0666, KIO::HideProgressInfo | KIO::Resume | KIO::Overwrite);
        } else {
            /*KIO::CopyJob *copyJob = */ KIO::copy(QUrl::fromLocalFile(srcPath), QUrl::fromLocalFile(fullPath), KIO::DefaultFlags); // Do it as before
        }
    } else {
        QByteArray array;
        bool success = FileStorage::loadFromFile(srcPath, &array);

        if (success) {
            saveToFile(fullPath, array);
        } else {
            qDebug() << "Unable to load encrypted file " << srcPath;
        }
    }

    return fileName;
}

void HTMLExporter::saveToFile(const QString &fullPath, const QByteArray &array)
{
    QFile file(QUrl::fromLocalFile(fullPath).path());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(array.toStdString().c_str(), array.size());
        file.close();
    } else {
        qDebug() << "Unable to open file for writing: " << fullPath;
    }
}
