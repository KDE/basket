/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "htmlexporter.h"

#include "bnpview.h"
#include "basketlistview.h"
#include "basketscene.h"
#include "note.h"
#include "tools.h"
#include "config.h"
#include "linklabel.h"
#include "notecontent.h"

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KMessageBox>
#include <KLocalizedString>
#include <KIconLoader>

#include <KIO/Job>      //For KIO::file_copy
#include <KIO/CopyJob>  //For KIO::copy

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QList>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QApplication>
#include <QFileDialog>
#include <QLocale>
#include <QProgressDialog>

HTMLExporter::HTMLExporter(BasketScene *basket) : dialog(new QProgressDialog())
{
    QDir dir;

    // Compute a default file name & path:
    KConfigGroup config = Global::config()->group("Export to HTML");
    QString folder = config.readEntry("lastFolder", QDir::homePath()) + "/";
    QString url = folder + QString(basket->basketName()).replace("/", "_") + ".html";

    // Ask a file name & path to the user:
    QString filter = "*.html *.htm|" + i18n("HTML Documents") + "\n*|" + i18n("All Files");
    QString destination = url;
    for (bool askAgain = true; askAgain;) {
        // Ask:
        destination = QFileDialog::getSaveFileName(NULL, i18n("Export to HTML"), destination, filter);
        // User canceled?
        if (destination.isEmpty())
            return;
        // File already existing? Ask for overriding:
        if (dir.exists(destination)) {
            int result = KMessageBox::questionYesNoCancel(
                             0,
                             "<qt>" + i18n("The file <b>%1</b> already exists. Do you really want to override it?",
                                           QUrl::fromLocalFile(destination).fileName()),
                             i18n("Override File?"),
                             KGuiItem(i18n("&Override"), "document-save")
                         );
            if (result == KMessageBox::Cancel)
                return;
            else if (result == KMessageBox::Yes)
                askAgain = false;
        } else
            askAgain = false;
    }

    // Create the progress dialog that will always be shown during the export:
    dialog->setWindowTitle(i18n("Export to HTML"));
    dialog->setLabelText(i18n("Exporting to HTML. Please wait..."));
    dialog->setCancelButton(NULL);
    dialog->setAutoClose(true);
    dialog->show();

    // Remember the last folder used for HTML exporation:
    config.writeEntry("lastFolder", QUrl::fromLocalFile(destination).adjusted(QUrl::RemoveFilename).path());
    config.sync();

    prepareExport(basket, destination);
    exportBasket(basket, /*isSubBasketScene*/false);

    dialog->setValue(dialog->value() + 1); // Finishing finished
}

HTMLExporter::~HTMLExporter()
{
}

void HTMLExporter::prepareExport(BasketScene *basket, const QString &fullPath)
{
    dialog->setRange(0,/*Preparation:*/1 + /*Finishing:*/1 + /*Basket:*/1 + /*SubBaskets:*/Global::bnpView->basketCount(Global::bnpView->listViewItemForBasket(basket)));
    dialog->setValue(0);
    qApp->processEvents();

    // Remember the file path chozen by the user:
    filePath = fullPath;
    fileName = QUrl::fromLocalFile(fullPath).fileName();
    exportedBasket = basket;
    currentBasket = 0;

    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    withBasketTree = (item->childCount() >= 0);

    // Create and empty the files folder:
    QString filesFolderPath = i18nc("HTML export folder (files)", "%1_files", filePath) + "/"; // eg.: "/home/seb/foo.html_files/"
    Tools::deleteRecursively(filesFolderPath);
    QDir dir;
    dir.mkdir(filesFolderPath);

    // Create sub-folders:
    iconsFolderPath   = filesFolderPath + i18nc("HTML export folder (icons)",   "icons")   + "/"; // eg.: "/home/seb/foo.html_files/icons/"
    imagesFolderPath  = filesFolderPath + i18nc("HTML export folder (images)",  "images")  + "/"; // eg.: "/home/seb/foo.html_files/images/"
    basketsFolderPath = filesFolderPath + i18nc("HTML export folder (baskets)", "baskets") + "/"; // eg.: "/home/seb/foo.html_files/baskets/"
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

    // Compute the absolute & relative paths for this basket:
    filesFolderPath   = i18nc("HTML export folder (files)", "%1_files", filePath) + "/";
    if (isSubBasket) {
        basketFilePath    = basketsFolderPath + basket->folderName().left(basket->folderName().length() - 1) + ".html";
        filesFolderName   = "../";
        dataFolderName    = basket->folderName().left(basket->folderName().length() - 1) + "-" + i18nc("HTML export folder (data)", "data") + "/";
        dataFolderPath    = basketsFolderPath + dataFolderName;
        basketsFolderName = "";
    } else {
        basketFilePath    = filePath;
        filesFolderName   = i18nc("HTML export folder (files)", "%1_files", QUrl::fromLocalFile(filePath).fileName()) + "/";
        dataFolderName    = filesFolderName + i18nc("HTML export folder (data)",    "data")  + "/";
        dataFolderPath    = filesFolderPath + i18nc("HTML export folder (data)",    "data")  + "/";
        basketsFolderName = filesFolderName + i18nc("HTML export folder (baskets)", "baskets")  + "/";
    }
    iconsFolderName   = (isSubBasket ? "../" : filesFolderName) + i18nc("HTML export folder (icons)",   "icons")   + "/"; // eg.: "foo.html_files/icons/"   or "../icons/"
    imagesFolderName  = (isSubBasket ? "../" : filesFolderName) + i18nc("HTML export folder (images)",  "images")  + "/"; // eg.: "foo.html_files/images/"  or "../images/"

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

    backgroundColorName = basket->backgroundColor().name().toLower().mid(1);

    // Generate basket icons:
    QString basketIcon16 = iconsFolderName + copyIcon(basket->icon(), 16);
    QString basketIcon32 = iconsFolderName + copyIcon(basket->icon(), 32);

    // Generate the [+] image for groups:
    QPixmap expandGroup(Note::EXPANDER_WIDTH, Note::EXPANDER_HEIGHT);
    expandGroup.fill(basket->backgroundColor());
    QPainter painter(&expandGroup);
    Note::drawExpander(&painter, 0, 0, basket->backgroundColor(), /*expand=*/true, basket);
    painter.end();
    expandGroup.save(imagesFolderPath + "expand_group_" + backgroundColorName + ".png", "PNG");

    // Generate the [-] image for groups:
    QPixmap foldGroup(Note::EXPANDER_WIDTH, Note::EXPANDER_HEIGHT);
    foldGroup.fill(basket->backgroundColor());
    painter.begin(&foldGroup);
    Note::drawExpander(&painter, 0, 0, basket->backgroundColor(), /*expand=*/false, basket);
    painter.end();
    foldGroup.save(imagesFolderPath + "fold_group_" + backgroundColorName + ".png", "PNG");

    // Open the file to write:
    QFile file(basketFilePath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    stream.setDevice(&file);
    stream.setCodec("UTF-8");

    // Compute the colors to draw dragient for notes:
    QColor topBgColor;
    QColor bottomBgColor;
    Note::getGradientColors(basket->backgroundColor(), &topBgColor, &bottomBgColor);
    // Compute the gradient image for notes:
    QString gradientImageFileName = BasketScene::saveGradientBackground(basket->backgroundColor(), basket->QGraphicsScene::font(), imagesFolderPath);

    // Output the header:
    QString borderColor = Tools::mixColor(basket->backgroundColor(), basket->textColor()).name();
    stream <<
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n"
    "<html>\n"
    " <head>\n"
    "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
    "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><meta name=\"Generator\" content=\""
           << QGuiApplication::applicationDisplayName() << " " << VERSION << " " << KAboutData::applicationData().homepage() << "\">\n"
    "  <style type=\"text/css\">\n"
//      "   @media print {\n"
//      "    span.printable { display: inline; }\n"
//      "   }\n"
    "   body { margin: 10px; font: 11px sans-serif; }\n" // TODO: Use user font
    "   h1 { text-align: center; }\n"
    "   img { border: none; vertical-align: middle; }\n";
    if (withBasketTree) {
        stream <<
        "   .tree { margin: 0; padding: 1px 0 1px 1px; width: 150px; _width: 149px; overflow: hidden; float: left; }\n"
        "   .tree ul { margin: 0 0 0 10px; padding: 0; }\n"
        "   .tree li { padding: 0; margin: 0; list-style: none; }\n"
        "   .tree a { display: block; padding: 1px; height: 16px; text-decoration: none;\n"
        "             white-space: nowrap; word-wrap: normal; text-wrap: suppress; color: black; }\n"
        "   .tree span { -moz-border-radius: 6px; display: block; float: left;\n"
        "                line-height: 16px; height: 16px; vertical-align: middle; padding: 0 1px; }\n"
        "   .tree img { vertical-align: top; padding-right: 1px; }\n"
        "   .tree .current { background-color: " << qApp->palette().color(QPalette::Highlight).name() << "; "
        "-moz-border-radius: 3px 0 0 3px; border-radius: 3px 0 0 3px; color: " << qApp->palette().color(QPalette::Highlight).name() << "; }\n"
        "   .basketSurrounder { margin-left: 152px; _margin: 0; _float: right; }\n";
    }
    stream <<
    "   .basket { background-color: " << basket->backgroundColor().name() << "; border: solid " << borderColor << " 1px; "
    "font: " << Tools::cssFontDefinition(basket->QGraphicsScene::font()) << "; color: " << basket->textColor().name() << "; padding: 1px; width: 100%; }\n"
    "   table.basket { border-collapse: collapse; }\n"
    "   .basket * { padding: 0; margin: 0; }\n"
    "   .basket table { width: 100%; border-spacing: 0; _border-collapse: collapse; }\n"
    "   .column { vertical-align: top; }\n"
    "   .columnHandle { width: " << Note::RESIZER_WIDTH << "px; background: transparent url('" << imagesFolderName << "column_handle_" << backgroundColorName << ".png') repeat-y; }\n"
    "   .group { margin: 0; padding: 0; border-collapse: collapse; width: 100% }\n"
    "   .groupHandle { margin: 0; width: " << Note::GROUP_WIDTH << "px; text-align: center; }\n"
    "   .note { padding: 1px 2px; background: " << bottomBgColor.name() << " url('" << imagesFolderName << gradientImageFileName << "')"
    " repeat-x; border-top: solid " << topBgColor.name() <<
    " 1px; border-bottom: solid " << Tools::mixColor(topBgColor, bottomBgColor).name() <<
    " 1px; width: 100%; }\n"
    "   .tags { width: 1px; white-space: nowrap; }\n"
    "   .tags img { padding-right: 2px; }\n"
    << LinkLook::soundLook->toCSS("sound", basket->textColor())
    << LinkLook::fileLook->toCSS("file", basket->textColor())
    << LinkLook::localLinkLook->toCSS("local", basket->textColor())
    << LinkLook::networkLinkLook->toCSS("network", basket->textColor())
    << LinkLook::launcherLook->toCSS("launcher", basket->textColor())
    << LinkLook::crossReferenceLook->toCSS("cross_reference", basket->textColor())
    <<
    "   .unknown { margin: 1px 2px; border: 1px solid " << borderColor << "; -moz-border-radius: 4px; }\n";
    QList<State*> states = basket->usedStates();
    QString statesCss;
    for (State::List::Iterator it = states.begin(); it != states.end(); ++it)
        statesCss += (*it)->toCSS(imagesFolderPath, imagesFolderName, basket->QGraphicsScene::font());
    stream <<
    statesCss <<
    "   .credits { text-align: right; margin: 3px 0 0 0; _margin-top: -17px; font-size: 80%; color: " << borderColor << "; }\n"
    "  </style>\n"
    "  <title>" << Tools::textToHTMLWithoutP(basket->basketName()) << "</title>\n"
    "  <link rel=\"shortcut icon\" type=\"image/png\" href=\"" << basketIcon16 << "\">\n";
    // Create the column handle image:
    QPixmap columnHandle(Note::RESIZER_WIDTH, 50);
    painter.begin(&columnHandle);
    Note::drawInactiveResizer(&painter, 0, 0, columnHandle.height(), basket->backgroundColor(), /*column=*/true);
    painter.end();
    columnHandle.save(imagesFolderPath + "column_handle_" + backgroundColorName + ".png", "PNG");

    stream <<
    " </head>\n"
    " <body>\n"
    "  <h1><img src=\"" << basketIcon32 << "\" width=\"32\" height=\"32\" alt=\"\"> " << Tools::textToHTMLWithoutP(basket->basketName()) << "</h1>\n";

    if (withBasketTree)
        writeBasketTree(basket);

    // If filtering, only export filtered notes, inform to the user:
    // TODO: Filtering tags too!!
    // TODO: Make sure only filtered notes are exported!
//  if (decoration()->filterData().isFiltering)
//      stream <<
//          "  <p>" << i18n("Notes matching the filter &quot;%1&quot;:", Tools::textToHTMLWithoutP(decoration()->filterData().string)) << "</p>\n";

    stream <<
    "  <div class=\"basketSurrounder\">\n";

    if (basket->isColumnsLayout())
        stream <<
        "   <table class=\"basket\">\n"
        "    <tr>\n";
    else
        stream <<
        "   <div class=\"basket\" style=\"position: relative; height: " << basket->sceneRect().height() << "px; width: " << basket->sceneRect().width() << "px; min-width: 100%;\">\n";

    for (Note *note = basket->firstNote(); note; note = note->next())
        exportNote(note, /*indent=*/(basket->isFreeLayout() ? 4 : 5));

    // Output the footer:
    if (basket->isColumnsLayout())
        stream <<
        "    </tr>\n"
        "   </table>\n";
    else
        stream <<
        "   </div>\n";
    stream << QString(
        "  </div>\n"
        "  <p class=\"credits\">%1</p>\n").arg(
            i18n("Made with <a href=\"%1\">%2</a> %3, a tool to take notes and keep information at hand.",
                 KAboutData::applicationData().homepage(), QGuiApplication::applicationDisplayName(), VERSION));

    stream <<
    " </body>\n"
    "</html>\n";

    file.close();
    stream.setDevice(0);
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
        QString width = "";
        if (false/*TODO: DEBUG AND REENABLE: hasResizer()*/) {
            // As we cannot be precise in CSS (say eg. "width: 50%-40px;"),
            // we output a percentage that is approximatively correct.
            // For instance, we compute the currently used percentage of width in the basket
            // and try make make it the same on a 1024*768 display in a Web browser:
            int availableSpaceForColumnsInThisBasket = note->basket()->sceneRect().width() - (note->basket()->columnsCount() - 1) * Note::RESIZER_WIDTH;
            int availableSpaceForColumnsInBrowser    = 1024    /* typical screen width */
                    - 25    /* window border and scrollbar width */
                    - 2 * 5 /* page margin */
                    - (note->basket()->columnsCount() - 1) * Note::RESIZER_WIDTH;
            if (availableSpaceForColumnsInThisBasket <= 0)
                availableSpaceForColumnsInThisBasket = 1;
            int widthValue = (int)(availableSpaceForColumnsInBrowser * (double) note->groupWidth() / availableSpaceForColumnsInThisBasket);
            if (widthValue <= 0)
                widthValue = 1;
            if (widthValue > 100)
                widthValue = 100;
            width = QString(" width=\"%1%\"").arg(QString::number(widthValue));
        }
        stream << spaces.fill(' ', indent) << "<td class=\"column\"" << width << ">\n";

        // Export child notes:
        for (Note *child = note->firstChild(); child; child = child->next()) {
            stream << spaces.fill(' ', indent + 1);
            exportNote(child, indent + 1);
            stream << '\n';
        }

        stream << spaces.fill(' ', indent) << "</td>\n";
        if (note->hasResizer())
            stream << spaces.fill(' ', indent) << "<td class=\"columnHandle\"></td>\n";
        return;
    }

    QString freeStyle;
    if (note->isFree())
        freeStyle = " style=\"position: absolute; left: " + QString::number(note->x()) + "px; top: " + QString::number(note->y()) + "px; width: " + QString::number(note->groupWidth()) + "px\"";

    if (note->isGroup()) {
        stream << '\n' << spaces.fill(' ', indent) << "<table" << freeStyle << ">\n"; // Note content is expected to be on the same HTML line, but NOT groups
        int i = 0;
        for (Note *child = note->firstChild(); child; child = child->next()) {
            stream << spaces.fill(' ', indent);
            if (i == 0)
                stream << " <tr><td class=\"groupHandle\"><img src=\"" << imagesFolderName << (note->isFolded() ? "expand_group_" : "fold_group_") << backgroundColorName << ".png"
                << "\" width=\"" << Note::EXPANDER_WIDTH << "\" height=\"" << Note::EXPANDER_HEIGHT << "\"></td>\n";
            else if (i == 1)
                stream << " <tr><td class=\"freeSpace\" rowspan=\"" << note->countDirectChilds() << "\"></td>\n";
            else
                stream << " <tr>\n";
            stream << spaces.fill(' ', indent) << "  <td>";
            exportNote(child, indent + 3);
            stream << "</td>\n"
            << spaces.fill(' ', indent) << " </tr>\n";
            ++i;
        }
        stream << '\n' << spaces.fill(' ', indent) << "</table>\n" /*<< spaces.fill(' ', indent - 1)*/;
    } else {
        // Additional class for the content (link, netword, color...):
        QString additionalClasses = note->content()->cssClass();
        if (!additionalClasses.isEmpty())
            additionalClasses = " " + additionalClasses;
        // Assign the style of each associted tags:
        for (State::List::Iterator it = note->states().begin(); it != note->states().end(); ++it)
            additionalClasses += " tag_" + (*it)->id();
        //stream << spaces.fill(' ', indent);
        stream << "<table class=\"note" << additionalClasses << "\"" << freeStyle << "><tr>";
        if (note->emblemsCount() > 0) {
            stream << "<td class=\"tags\"><nobr>";
            for (State::List::Iterator it = note->states().begin(); it != note->states().end(); ++it)
                if (!(*it)->emblem().isEmpty()) {
                    int emblemSize = 16;
                    QString iconFileName = copyIcon((*it)->emblem(), emblemSize);
                    stream << "<img src=\"" << iconsFolderName << iconFileName
                    << "\" width=\"" << emblemSize << "\" height=\"" << emblemSize
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
    QString cssClass = (basket == currentBasket ? " class=\"current\"" : "");
    QString link = "#";
    if (currentBasket != basket) {
        if (currentBasket == exportedBasket) {
            link = basketsFolderName + basket->folderName().left(basket->folderName().length() - 1) + ".html";
        } else if (basket == exportedBasket) {
            link = "../../" + fileName;
        } else {
            link = basket->folderName().left(basket->folderName().length() - 1) + ".html";
        }
    }
    QString spanStyle = "";
    if (basket->backgroundColorSetting().isValid() || basket->textColorSetting().isValid()) {
        spanStyle = " style=\"background-color: " + basket->backgroundColor().name() + "; color: " + basket->textColor().name() + "\"";
    }

    // Write the basket tree line:
    stream <<
    spaces.fill(' ', indent) << "<li><a" << cssClass << " href=\"" << link << "\">"
    "<span" << spanStyle << " title=\"" << Tools::textToHTMLWithoutP(basket->basketName()) << "\">"
    "<img src=\"" << iconsFolderName <<  copyIcon(basket->icon(), 16) << "\" width=\"16\" height=\"16\" alt=\"\">" << Tools::textToHTMLWithoutP(basket->basketName()) << "</span></a>";

    // Write the sub-baskets lines & end the current one:
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (item->childCount() >= 0) {
        stream <<
        "\n" <<
        spaces.fill(' ', indent) << " <ul>\n";
        for (int i = 0; i < item->childCount(); i++)
            writeBasketTree(currentBasket, ((BasketListViewItem*)item->child(i))->basket(), indent + 2);
        stream <<
        spaces.fill(' ', indent) << " </ul>\n" <<
        spaces.fill(' ', indent) << "</li>\n";
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
    if (iconName.isEmpty())
        return "";

    // Sometimes icon can be "favicons/www.kde.org", we replace the '/' with a '_'
    QString fileName = iconName; // QString::replace() isn't const, so I must copy the string before
    fileName = "ico" + QString::number(size) + "_" + fileName.replace("/", "_") + ".png";
    QString fullPath = iconsFolderPath + fileName;
    if (!QFile::exists(fullPath))
        DesktopIcon(iconName, size).save(fullPath, "PNG");
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

    if (createIt) {
        // We create the file to be sure another very near call to copyFile() willn't choose the same name:
        QFile file(QUrl::fromLocalFile(fullPath).path());
        if (file.open(QIODevice::WriteOnly))
            file.close();
        // And then we copy the file AND overwriting the file we juste created:
        KIO::file_copy(QUrl::fromLocalFile(srcPath), QUrl::fromLocalFile(fullPath), 0666, KIO::HideProgressInfo | KIO::Resume | KIO::Overwrite);
    } else
        /*KIO::CopyJob *copyJob = */KIO::copy(QUrl::fromLocalFile(srcPath), QUrl::fromLocalFile(fullPath), KIO::DefaultFlags); // Do it as before

    return fileName;
}
