/***************************************************************************
 *   Copyright (C) 2006 by Sébastien Laoût                                 *
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

#include "archive.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QProgressBar>
#include <QtXml/QDomDocument>
#include <QGuiApplication>
#include <QDebug>
#include <QLocale>
#include <QProgressDialog>
#include <QStandardPaths>

#include <KAboutData>
#include <KMainWindow>          //For Global::MainWindow()
#include <KIconLoader>
#include <KMessageBox>
#include <KTar>
#include <KLocalizedString>

#include "global.h"
#include "bnpview.h"
#include "basketscene.h"
#include "basketlistview.h"
#include "basketfactory.h"
#include "tag.h"
#include "xmlwork.h"
#include "tools.h"
#include "backgroundmanager.h"
#include "formatimporter.h"

void Archive::save(BasketScene *basket, bool withSubBaskets, const QString &destination)
{
    QDir dir;
    QProgressDialog dialog;
    dialog.setWindowTitle(i18n("Save as Basket Archive"));
    dialog.setLabelText(i18n("Saving as basket archive. Please wait..."));
    dialog.setCancelButton(NULL);
    dialog.setAutoClose(true);

    dialog.setRange(0,/*Preparation:*/1 + /*Finishing:*/1 + /*Basket:*/1 + /*SubBaskets:*/(withSubBaskets ? Global::bnpView->basketCount(Global::bnpView->listViewItemForBasket(basket)) : 0));
    dialog.setValue(0);
    dialog.show();

    // Create the temporary folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";
    dir.mkdir(tempFolder);

    // Create the temporary archive file:
    QString tempDestination = tempFolder + "temp-archive.tar.gz";
    KTar tar(tempDestination, "application/x-gzip");
    tar.open(QIODevice::WriteOnly);
    tar.writeDir("baskets", "", "");

    dialog.setValue(dialog.value() + 1);      // Preparation finished

    qDebug() << "Preparation finished out of " << dialog.maximum();

    // Copy the baskets data into the archive:
    QStringList backgrounds;
    Archive::saveBasketToArchive(basket, withSubBaskets, &tar, backgrounds, tempFolder, &dialog);

    // Create a Small baskets.xml Document:
    QString data;
    QXmlStreamWriter stream(&data);
    XMLWork::setupXmlStream(stream, "basketTree");
    Global::bnpView->saveSubHierarchy(Global::bnpView->listViewItemForBasket(basket), stream, withSubBaskets);
    stream.writeEndElement();
    stream.writeEndDocument();
    BasketScene::safelySaveToFile(tempFolder + "baskets.xml", data);
    tar.addLocalFile(tempFolder + "baskets.xml", "baskets/baskets.xml");
    dir.remove(tempFolder + "baskets.xml");

    // Save a Small tags.xml Document:
    QList<Tag*> tags;
    listUsedTags(basket, withSubBaskets, tags);
    Tag::saveTagsTo(tags, tempFolder + "tags.xml");
    tar.addLocalFile(tempFolder + "tags.xml", "tags.xml");
    dir.remove(tempFolder + "tags.xml");

    // Save Tag Emblems (in case they are loaded on a computer that do not have those icons):
    QString tempIconFile = tempFolder + "icon.png";
    for (Tag::List::iterator it = tags.begin(); it != tags.end(); ++it) {
        State::List states = (*it)->states();
        for (State::List::iterator it2 = states.begin(); it2 != states.end(); ++it2) {
            State *state = (*it2);
            QPixmap icon = KIconLoader::global()->loadIcon(
                               state->emblem(), KIconLoader::Small, 16, KIconLoader::DefaultState,
                               QStringList(), 0L, true
                           );
            if (!icon.isNull()) {
                icon.save(tempIconFile, "PNG");
                QString iconFileName = state->emblem().replace('/', '_');
                tar.addLocalFile(tempIconFile, "tag-emblems/" + iconFileName);
            }
        }
    }
    dir.remove(tempIconFile);

    // Finish Tar.Gz Exportation:
    tar.close();

    // Computing the File Preview:
    BasketScene *previewBasket = basket; // FIXME: Use the first non-empty basket!
    //QPixmap previewPixmap(previewBasket->visibleWidth(), previewBasket->visibleHeight());
    QPixmap previewPixmap(previewBasket->width(), previewBasket->height());
    QPainter painter(&previewPixmap);
    // Save old state, and make the look clean ("smile, you are filmed!"):
    NoteSelection *selection = previewBasket->selectedNotes();
    previewBasket->unselectAll();
    Note *focusedNote = previewBasket->focusedNote();
    previewBasket->setFocusedNote(0);
    previewBasket->doHoverEffects(0, Note::None);
    // Take the screenshot:
    previewBasket->render(&painter);
    // Go back to the old look:
    previewBasket->selectSelection(selection);
    previewBasket->setFocusedNote(focusedNote);
    previewBasket->doHoverEffects();
    // End and save our splandid painting:
    painter.end();
    QImage previewImage = previewPixmap.toImage();
    const int PREVIEW_SIZE = 256;
    previewImage = previewImage.scaled(PREVIEW_SIZE, PREVIEW_SIZE, Qt::KeepAspectRatio);
    previewImage.save(tempFolder + "preview.png", "PNG");

    // Finaly Save to the Real Destination file:
    QFile file(destination);
    if (file.open(QIODevice::WriteOnly)) {
        ulong previewSize = QFile(tempFolder + "preview.png").size();
        ulong archiveSize = QFile(tempDestination).size();
        QTextStream stream(&file);
        stream.setCodec("ISO-8859-1");
        stream << "BasKetNP:archive\n"
        << "version:0.6.1\n"
//             << "read-compatible:0.6.1\n"
//             << "write-compatible:0.6.1\n"
        << "preview*:" << previewSize << "\n";

        stream.flush();
        // Copy the Preview File:
        const unsigned long BUFFER_SIZE = 1024;
        char *buffer = new char[BUFFER_SIZE];
        long sizeRead;
        QFile previewFile(tempFolder + "preview.png");
        if (previewFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = previewFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        stream << "archive*:" << archiveSize << "\n";
        stream.flush();

        // Copy the Archive File:
        QFile archiveFile(tempDestination);
        if (archiveFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = archiveFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        // Clean Up:
        delete[] buffer;
        buffer = 0;
        file.close();
    }

    dialog.setValue(dialog.value() + 1); // Finishing finished
    qDebug() << "Finishing finished";

    // Clean Up Everything:
    dir.remove(tempFolder + "preview.png");
    dir.remove(tempDestination);
    dir.rmdir(tempFolder);
}

void Archive::saveBasketToArchive(BasketScene *basket, bool recursive, KTar *tar, QStringList &backgrounds, const QString &tempFolder, QProgressDialog *progress)
{
    // Basket need to be loaded for tags exportation.
    // We load it NOW so that the progress bar really reflect the state of the exportation:
    if (!basket->isLoaded()) {
        basket->load();
    }

    QDir dir;
    // Save basket data:
    tar->addLocalDirectory(basket->fullPath(), "baskets/" + basket->folderName());
    // Save basket icon:
    QString tempIconFile = tempFolder + "icon.png";
    if (!basket->icon().isEmpty() && basket->icon() != "basket") {
        QPixmap icon = KIconLoader::global()->loadIcon(basket->icon(), KIconLoader::Small, 16, KIconLoader::DefaultState,
                       QStringList(), /*path_store=*/0L, /*canReturnNull=*/true);
        if (!icon.isNull()) {
            icon.save(tempIconFile, "PNG");
            QString iconFileName = basket->icon().replace('/', '_');
            tar->addLocalFile(tempIconFile, "basket-icons/" + iconFileName);
        }
    }
    // Save basket backgorund image:
    QString imageName = basket->backgroundImageName();
    if (!basket->backgroundImageName().isEmpty() && !backgrounds.contains(imageName)) {
        QString backgroundPath = Global::backgroundManager->pathForImageName(imageName);
        if (!backgroundPath.isEmpty()) {
            // Save the background image:
            tar->addLocalFile(backgroundPath, "backgrounds/" + imageName);
            // Save the preview image:
            QString previewPath = Global::backgroundManager->previewPathForImageName(imageName);
            if (!previewPath.isEmpty())
                tar->addLocalFile(previewPath, "backgrounds/previews/" + imageName);
            // Save the configuration file:
            QString configPath = backgroundPath + ".config";
            if (dir.exists(configPath))
                tar->addLocalFile(configPath, "backgrounds/" + imageName + ".config");
        }
        backgrounds.append(imageName);
    }

    progress->setValue(progress->value() + 1); // Basket exportation finished
    qDebug() << basket->basketName() << " finished";

    // Recursively save child baskets:
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (recursive) {
        for (int i = 0; i < item->childCount(); i++) {
            saveBasketToArchive(((BasketListViewItem *)item->child(i))->basket(), recursive, tar, backgrounds, tempFolder, progress);
        }
    }
}

void Archive::listUsedTags(BasketScene *basket, bool recursive, QList<Tag*> &list)
{
    basket->listUsedTags(list);
    BasketListViewItem *item = Global::bnpView->listViewItemForBasket(basket);
    if (recursive) {
        for (int i = 0; i < item->childCount(); i++) {
            listUsedTags(((BasketListViewItem *)item->child(i))->basket(), recursive, list);
        }
    }
}

void Archive::open(const QString &path)
{
    // Create the temporar folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";
    QDir dir;
    dir.mkdir(tempFolder);
    const qint64 BUFFER_SIZE = 1024;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec("ISO-8859-1");
        QString line = stream.readLine();
        if (line != "BasKetNP:archive") {
            KMessageBox::error(0, i18n("This file is not a basket archive."), i18n("Basket Archive Error"));
            file.close();
            Tools::deleteRecursively(tempFolder);
            return;
        }
        QString version;
        QStringList readCompatibleVersions;
        QStringList writeCompatibleVersions;
        while (!stream.atEnd()) {
            // Get Key/Value Pair From the Line to Read:
            line = stream.readLine();
            int index = line.indexOf(':');
            QString key;
            QString value;
            if (index >= 0) {
                key = line.left(index);
                value = line.right(line.length() - index - 1);
            } else {
                key = line;
                value = "";
            }
            if (key == "version") {
                version = value;
            } else if (key == "read-compatible") {
                readCompatibleVersions = value.split(";");
            } else if (key == "write-compatible") {
                writeCompatibleVersions = value.split(";");
            } else if (key == "preview*") {
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    KMessageBox::error(0, i18n("This file is corrupted. It can not be opened."), i18n("Basket Archive Error"));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }
                // Get the preview file:
//FIXME: We do not need the preview for now
//              QFile previewFile(tempFolder + "preview.png");
//              if (previewFile.open(QIODevice::WriteOnly)) {
                stream.seek(stream.pos() + size);
            } else if (key == "archive*") {
                if (version != "0.6.1" && readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    KMessageBox::information(
                        0,
                        i18n("This file was created with a recent version of %1. "
                             "It can be opened but not every information will be available to you. "
                             "For instance, some notes may be missing because they are of a type only available in new versions. "
                             "When saving the file back, consider to save it to another file, to preserve the original one.",
                             QGuiApplication::applicationDisplayName()),
                        i18n("Basket Archive Error")
                    );
                }
                if (version != "0.6.1" && !readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    KMessageBox::error(
                        0,
                        i18n("This file was created with a recent version of %1. Please upgrade to a newer version to be able to open that file.",
                             QGuiApplication::applicationDisplayName()),
                        i18n("Basket Archive Error")
                    );
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }

                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    KMessageBox::error(0, i18n("This file is corrupted. It can not be opened."), i18n("Basket Archive Error"));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }

                if (Global::activeMainWindow()) {
                    Global::activeMainWindow()->raise();
                }

                // Get the archive file:
                QString tempArchive = tempFolder + "temp-archive.tar.gz";
                QFile archiveFile(tempArchive);
                file.seek(stream.pos());
                if (archiveFile.open(QIODevice::WriteOnly)) {
                    char *buffer = new char[BUFFER_SIZE];
                    qint64 sizeRead;
                    while ((sizeRead = file.read(buffer, qMin(BUFFER_SIZE, size))) > 0) {
                        archiveFile.write(buffer, sizeRead);
                        size -= sizeRead;
                    }
                    archiveFile.close();
                    delete[] buffer;

                    // Extract the Archive:
                    QString extractionFolder = tempFolder + "extraction/";
                    QDir dir;
                    dir.mkdir(extractionFolder);
                    KTar tar(tempArchive, "application/x-gzip");
                    tar.open(QIODevice::ReadOnly);
                    tar.directory()->copyTo(extractionFolder);
                    tar.close();

                    // Import the Tags:
                    importTagEmblems(extractionFolder); // Import and rename tag emblems BEFORE loading them!
                    QMap<QString, QString> mergedStates = Tag::loadTags(extractionFolder + "tags.xml");
                    if (mergedStates.count() > 0) {
                        Tag::saveTags();
                    }

                    // Import the Background Images:
                    importArchivedBackgroundImages(extractionFolder);

                    // Import the Baskets:
                    renameBasketFolders(extractionFolder, mergedStates);
                    stream.seek(file.pos());

                }
            } else if (key.endsWith('*')) {
                // We do not know what it is, but we should read the embedded-file in order to discard it:
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    KMessageBox::error(0, i18n("This file is corrupted. It can not be opened."), i18n("Basket Archive Error"));
                    file.close();
                    Tools::deleteRecursively(tempFolder);
                    return;
                }
                // Get the archive file:
                char *buffer = new char[BUFFER_SIZE];
                qint64 sizeRead;
                while ((sizeRead = file.read(buffer, qMin(BUFFER_SIZE, size))) > 0) {
                    size -= sizeRead;
                }
                delete[] buffer;
            } else {
                // We do not know what it is, and we do not care.
            }
            // Analyze the Value, if Understood:
        }
        file.close();
    }
    Tools::deleteRecursively(tempFolder);
}

/**
 * When opening a basket archive that come from another computer,
 * it can contains tags that use icons (emblems) that are not present on that computer.
 * Fortunately, basket archives contains a copy of every used icons.
 * This method check for every emblems and import the missing ones.
 * It also modify the tags.xml copy for the emblems to point to the absolute path of the impported icons.
 */
void Archive::importTagEmblems(const QString &extractionFolder)
{
    QDomDocument *document = XMLWork::openFile("basketTags", extractionFolder + "tags.xml");
    if (document == 0)
        return;
    QDomElement docElem = document->documentElement();

    QDir dir;
    dir.mkdir(Global::savesFolder() + "tag-emblems/");
    FormatImporter copier; // Only used to copy files synchronously

    QDomNode node = docElem.firstChild();
    while (!node.isNull()) {
        QDomElement element = node.toElement();
        if ((!element.isNull()) && element.tagName() == "tag") {
            QDomNode subNode = element.firstChild();
            while (!subNode.isNull()) {
                QDomElement subElement = subNode.toElement();
                if ((!subElement.isNull()) && subElement.tagName() == "state") {
                    QString emblemName = XMLWork::getElementText(subElement, "emblem");
                    if (!emblemName.isEmpty()) {
                        QPixmap emblem = KIconLoader::global()->loadIcon(emblemName, KIconLoader::NoGroup, 16, KIconLoader::DefaultState,  QStringList(), 0L, /*canReturnNull=*/true);
                        // The icon does not exists on that computer, import it:
                        if (emblem.isNull()) {
                            // Of the emblem path was eg. "/home/seb/emblem.png", it was exported as "tag-emblems/_home_seb_emblem.png".
                            // So we need to copy that image to "~/.local/share/basket/tag-emblems/emblem.png":
                            int slashIndex = emblemName.lastIndexOf('/');
                            QString emblemFileName = (slashIndex < 0 ? emblemName : emblemName.right(slashIndex - 2));
                            QString source      = extractionFolder + "tag-emblems/" + emblemName.replace('/', '_');
                            QString destination = Global::savesFolder() + "tag-emblems/" + emblemFileName;
                            if (!dir.exists(destination) && dir.exists(source))
                                copier.copyFolder(source, destination);
                            // Replace the emblem path in the tags.xml copy:
                            QDomElement emblemElement = XMLWork::getElement(subElement, "emblem");
                            subElement.removeChild(emblemElement);
                            XMLWork::addElement(*document, subElement, "emblem", destination);
                        }
                    }
                }
                subNode = subNode.nextSibling();
            }
        }
        node = node.nextSibling();
    }
    BasketScene::safelySaveToFile(extractionFolder + "tags.xml", document->toString());
}

void Archive::importArchivedBackgroundImages(const QString &extractionFolder)
{
    FormatImporter copier; // Only used to copy files synchronously
    QString destFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/basket/backgrounds/";
    QDir().mkpath(destFolder); //does not exist at the first run when addWelcomeBaskets is called

    QDir dir(extractionFolder + "backgrounds/", /*nameFilder=*/"*.png", /*sortSpec=*/QDir::Name | QDir::IgnoreCase, /*filterSpec=*/QDir::Files | QDir::NoSymLinks);
    QStringList files = dir.entryList();
    for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
        QString image = *it;
        if (!Global::backgroundManager->exists(image)) {
            // Copy images:
            QString imageSource = extractionFolder + "backgrounds/" + image;
            QString imageDest   = destFolder + image;
            copier.copyFolder(imageSource, imageDest);
            // Copy configuration file:
            QString configSource = extractionFolder + "backgrounds/" + image + ".config";
            QString configDest   = destFolder + image;
            if (dir.exists(configSource))
                copier.copyFolder(configSource, configDest);
            // Copy preview:
            QString previewSource = extractionFolder + "backgrounds/previews/" + image;
            QString previewDest   = destFolder + "previews/" + image;
            if (dir.exists(previewSource)) {
                dir.mkdir(destFolder + "previews/"); // Make sure the folder exists!
                copier.copyFolder(previewSource, previewDest);
            }
            // Append image to database:
            Global::backgroundManager->addImage(imageDest);
        }
    }
}

void Archive::renameBasketFolders(const QString &extractionFolder, QMap<QString, QString> &mergedStates)
{
    QDomDocument *doc = XMLWork::openFile("basketTree", extractionFolder + "baskets/baskets.xml");
    if (doc != 0) {
        QMap<QString, QString> folderMap;
        QDomElement docElem = doc->documentElement();
        QDomNode node = docElem.firstChild();
        renameBasketFolder(extractionFolder, node, folderMap, mergedStates);
        loadExtractedBaskets(extractionFolder, node, folderMap, 0);
    }
}

void Archive::renameBasketFolder(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, QMap<QString, QString> &mergedStates)
{
    QDomNode n = basketNode;
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                // Find a folder name:
                QString newFolderName = BasketFactory::newFolderName();
                folderMap[folderName] = newFolderName;
                // Reserve the folder name:
                QDir dir;
                dir.mkdir(Global::basketsFolder() + newFolderName);
                // Rename the merged tag ids:
//              if (mergedStates.count() > 0) {
                renameMergedStatesAndBasketIcon(extractionFolder + "baskets/" + folderName + ".basket", mergedStates, extractionFolder);
//              }
                // Child baskets:
                QDomNode node = element.firstChild();
                renameBasketFolder(extractionFolder, node, folderMap, mergedStates);
            }
        }
        n = n.nextSibling();
    }
}

void Archive::renameMergedStatesAndBasketIcon(const QString &fullPath, QMap<QString, QString> &mergedStates, const QString &extractionFolder)
{
    QDomDocument *doc = XMLWork::openFile("basket", fullPath);
    if (doc == 0)
        return;
    QDomElement docElem = doc->documentElement();
    QDomElement properties = XMLWork::getElement(docElem, "properties");
    importBasketIcon(properties, extractionFolder);
    QDomElement notes = XMLWork::getElement(docElem, "notes");
    if (mergedStates.count() > 0)
        renameMergedStates(notes, mergedStates);
    BasketScene::safelySaveToFile(fullPath, /*"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + */doc->toString());
}

void Archive::importBasketIcon(QDomElement properties, const QString &extractionFolder)
{
    QString iconName = XMLWork::getElementText(properties, "icon");
    if (!iconName.isEmpty() && iconName != "basket") {
        QPixmap icon = KIconLoader::global()->loadIcon(
                           iconName, KIconLoader::NoGroup, 16, KIconLoader::DefaultState,
                           QStringList(), 0L, /*canReturnNull=*/true
                       );
        // The icon does not exists on that computer, import it:
        if (icon.isNull()) {
            QDir dir;
            dir.mkdir(Global::savesFolder() + "basket-icons/");
            FormatImporter copier; // Only used to copy files synchronously
            // Of the icon path was eg. "/home/seb/icon.png", it was exported as "basket-icons/_home_seb_icon.png".
            // So we need to copy that image to "~/.local/share/basket/basket-icons/icon.png":
            int slashIndex = iconName.lastIndexOf('/');
            QString iconFileName = (slashIndex < 0 ? iconName : iconName.right(slashIndex - 2));
            QString source       = extractionFolder + "basket-icons/" + iconName.replace('/', '_');
            QString destination = Global::savesFolder() + "basket-icons/" + iconFileName;
            if (!dir.exists(destination))
                copier.copyFolder(source, destination);
            // Replace the emblem path in the tags.xml copy:
            QDomElement iconElement = XMLWork::getElement(properties, "icon");
            properties.removeChild(iconElement);
            QDomDocument document = properties.ownerDocument();
            XMLWork::addElement(document, properties, "icon", destination);
        }
    }
}

void Archive::renameMergedStates(QDomNode notes, QMap<QString, QString> &mergedStates)
{
    QDomNode n = notes.firstChild();
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "group") {
                renameMergedStates(n, mergedStates);
            } else if (element.tagName() == "note") {
                QString tags = XMLWork::getElementText(element, "tags");
                if (!tags.isEmpty()) {
                    QStringList tagNames = tags.split(";");
                    for (QStringList::Iterator it = tagNames.begin(); it != tagNames.end(); ++it) {
                        QString &tag = *it;
                        if (mergedStates.contains(tag)) {
                            tag = mergedStates[tag];
                        }
                    }
                    QString newTags = tagNames.join(";");
                    QDomElement tagsElement = XMLWork::getElement(element, "tags");
                    element.removeChild(tagsElement);
                    QDomDocument document = element.ownerDocument();
                    XMLWork::addElement(document, element, "tags", newTags);
                }
            }
        }
        n = n.nextSibling();
    }
}

void Archive::loadExtractedBaskets(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, BasketScene *parent)
{
    bool basketSetAsCurrent = (parent != 0);
    QDomNode n = basketNode;
    while (! n.isNull()) {
        QDomElement element = n.toElement();
        if ((!element.isNull()) && element.tagName() == "basket") {
            QString folderName = element.attribute("folderName");
            if (!folderName.isEmpty()) {
                // Move the basket folder to its destination, while renaming it uniquely:
                QString newFolderName = folderMap[folderName];
                FormatImporter copier;
                // The folder has been "reserved" by creating it. Avoid asking the user to override:
                QDir dir;
                dir.rmdir(Global::basketsFolder() + newFolderName);
                copier.moveFolder(extractionFolder + "baskets/" + folderName, Global::basketsFolder() + newFolderName);
                // Append and load the basket in the tree:
                BasketScene *basket = Global::bnpView->loadBasket(newFolderName);
                BasketListViewItem *basketItem = Global::bnpView->appendBasket(basket, (basket && parent ? Global::bnpView->listViewItemForBasket(parent) : 0));
                basketItem->setExpanded(!XMLWork::trueOrFalse(element.attribute("folded", "false"), false));
                QDomElement properties = XMLWork::getElement(element, "properties");
                importBasketIcon(properties, extractionFolder); // Rename the icon fileName if necessary
                basket->loadProperties(properties);
                // Open the first basket of the archive:
                if (!basketSetAsCurrent) {
                    Global::bnpView->setCurrentBasket(basket);
                    basketSetAsCurrent = true;
                }
                QDomNode node = element.firstChild();
                loadExtractedBaskets(extractionFolder, node, folderMap, basket);
            }
        }
        n = n.nextSibling();
    }
}
