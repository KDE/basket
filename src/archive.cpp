/**
 * SPDX-FileCopyrightText: (C) 2006 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "archive.h"

#include <QDebug>
#include <QGuiApplication>
#include <QLocale>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtXml/QDomDocument>

#include <KAboutData>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMainWindow> //For Global::MainWindow()
#include <KMessageBox>
#include <KTar>

#include "backgroundmanager.h"
#include "basketfactory.h"
#include "basketlistview.h"
#include "basketscene.h"
#include "bnpview.h"
#include "common.h"
#include "formatimporter.h"
#include "global.h"
#include "tag.h"
#include "tools.h"
#include "xmlwork.h"

void Archive::save(BasketScene *basket, bool withSubBaskets, const QString &destination)
{
    QDir dir;
    QProgressDialog dialog;
    dialog.setWindowTitle(i18n("Save as Basket Archive"));
    dialog.setLabelText(i18n("Saving as basket archive. Please wait..."));
    dialog.setCancelButton(nullptr);
    dialog.setAutoClose(true);

    dialog.setRange(0, /*Preparation:*/ 1 + /*Finishing:*/ 1 + /*Basket:*/ 1 + /*SubBaskets:*/ (withSubBaskets ? Global::bnpView->basketCount(Global::bnpView->listViewItemForBasket(basket)) : 0));
    dialog.setValue(0);
    dialog.show();

    // Create the temporary folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";
    dir.mkdir(tempFolder);

    // Create the temporary archive file:
    QString tempDestination = tempFolder + "temp-archive.tar.gz";
    KTar tar(tempDestination, "application/x-gzip");
    tar.open(QIODevice::WriteOnly);
    tar.writeDir(QStringLiteral("baskets"), QString(), QString());

    dialog.setValue(dialog.value() + 1); // Preparation finished

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
    FileStorage::safelySaveToFile(tempFolder + "baskets.xml", data);
    tar.addLocalFile(tempFolder + "baskets.xml", "baskets/baskets.xml");
    dir.remove(tempFolder + "baskets.xml");

    // Save a Small tags.xml Document:
    QList<Tag *> tags;
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
            QPixmap icon = KIconLoader::global()->loadIcon(state->emblem(), KIconLoader::Small, 16, KIconLoader::DefaultState, QStringList(), nullptr, true);
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
    // QPixmap previewPixmap(previewBasket->visibleWidth(), previewBasket->visibleHeight());
    QPixmap previewPixmap(previewBasket->width(), previewBasket->height());
    QPainter painter(&previewPixmap);
    // Save old state, and make the look clean ("smile, you are filmed!"):
    NoteSelection *selection = previewBasket->selectedNotes();
    previewBasket->unselectAll();
    Note *focusedNote = previewBasket->focusedNote();
    previewBasket->setFocusedNote(nullptr);
    previewBasket->doHoverEffects(nullptr, Note::None);
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

    // Finally Save to the Real Destination file:
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
        buffer = nullptr;
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
        QPixmap icon = KIconLoader::global()->loadIcon(basket->icon(), KIconLoader::Small, 16, KIconLoader::DefaultState, QStringList(), /*path_store=*/nullptr, /*canReturnNull=*/true);
        if (!icon.isNull()) {
            icon.save(tempIconFile, "PNG");
            QString iconFileName = basket->icon().replace('/', '_');
            tar->addLocalFile(tempIconFile, "basket-icons/" + iconFileName);
        }
    }
    // Save basket background image:
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

void Archive::listUsedTags(BasketScene *basket, bool recursive, QList<Tag *> &list)
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
    // Use the temporary folder:
    QString tempFolder = Global::savesFolder() + "temp-archive/";

    switch (extractArchive(path, tempFolder)) {
    case IOErrorCode::FailedToOpenResource:
        KMessageBox::error(nullptr, i18n("Failed to open a file resource."), i18n("Basket Archive Error"));
        break;
    case IOErrorCode::NotABasketArchive:
        KMessageBox::error(nullptr, i18n("This file is not a basket archive."), i18n("Basket Archive Error"));
        break;
    case IOErrorCode::CorruptedBasketArchive:
        KMessageBox::error(nullptr, i18n("This file is corrupted. It can not be opened."), i18n("Basket Archive Error"));
        break;
    case IOErrorCode::DestinationExists:
        KMessageBox::error(nullptr, i18n("Extraction path already exists."), i18n("Basket Archive Error"));
        break;
    case IOErrorCode::IncompatibleBasketVersion:
        KMessageBox::error(nullptr,
                           i18n("This file was created with a recent version of %1."
                                "Please upgrade to a newer version to be able to open that file.",
                                QGuiApplication::applicationDisplayName()),
                           i18n("Basket Archive Error"));
        break;
    case IOErrorCode::PossiblyCompatibleBasketVersion:
        KMessageBox::information(nullptr,
                                 i18n("This file was created with a recent version of %1. "
                                      "It can be opened but not every information will be available to you. "
                                      "For instance, some notes may be missing because they are of a type only available in new versions. "
                                      "When saving the file back, consider to save it to another file, to preserve the original one.",
                                      QGuiApplication::applicationDisplayName()),
                                 i18n("Basket Archive Error"));
        [[fallthrough]];
    case IOErrorCode::NoError:
        if (Global::activeMainWindow()) {
            Global::activeMainWindow()->raise();
        }
        // Import the Tags:

        importTagEmblems(tempFolder); // Import and rename tag emblems BEFORE loading them!
        QMap<QString, QString> mergedStates = Tag::loadTags(tempFolder + "tags.xml");
        if (mergedStates.count() > 0) {
            Tag::saveTags();
        }

        // Import the Background Images:
        importArchivedBackgroundImages(tempFolder);

        // Import the Baskets:
        renameBasketFolders(tempFolder, mergedStates);

        Tools::deleteRecursively(tempFolder);
        break;
    }
}

Archive::IOErrorCode Archive::extractArchive(const QString &path, const QString &destination)
{
    IOErrorCode retCode = IOErrorCode::NoError;

    QString mDestination;
    if (destination.isEmpty()) {
        // have the archive source the same name as the archive
        mDestination = QFileInfo(path).path() + QDir::separator() + QFileInfo(path).baseName() + "-source";
    } else {
        mDestination = QDir::cleanPath(destination);
    }

    QDir dir;
    if (!dir.mkdir(mDestination)) {
        return IOErrorCode::DestinationExists;
    }
    const qint64 BUFFER_SIZE = 1024;

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec("ISO-8859-1");
        QString line = stream.readLine();
        if (line != "BasKetNP:archive") {
            file.close();
            Tools::deleteRecursively(mDestination);
            return IOErrorCode::NotABasketArchive;
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
                value = QString();
            }
            if (key == "version") {
                version = value;
            } else if (key == "read-compatible") {
                readCompatibleVersions = value.split(';');
            } else if (key == "write-compatible") {
                writeCompatibleVersions = value.split(';');
            } else if (key == "preview*") {
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    Tools::deleteRecursively(mDestination);
                    return IOErrorCode::CorruptedBasketArchive;
                }
                // Get the preview file:
                // FIXME: We do not need the preview for now
                QFile previewFile(mDestination + "preview.png");
                if (previewFile.open(QIODevice::WriteOnly)) {
                    char *buffer = new char[BUFFER_SIZE];
                    qint64 sizeRead;
                    while ((sizeRead = file.read(buffer, qMin(BUFFER_SIZE, size))) > 0) {
                        previewFile.write(buffer, sizeRead);
                        size -= sizeRead;
                    }
                    previewFile.close();
                    delete[] buffer;
                }
                stream.seek(stream.pos());
            } else if (key == "archive*") {
                if (version != "0.6.1" && readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    retCode = IOErrorCode::PossiblyCompatibleBasketVersion;
                }
                if (version != "0.6.1" && !readCompatibleVersions.contains("0.6.1") && !writeCompatibleVersions.contains("0.6.1")) {
                    file.close();
                    Tools::deleteRecursively(mDestination);
                    return IOErrorCode::IncompatibleBasketVersion;
                }

                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    Tools::deleteRecursively(mDestination);
                    return IOErrorCode::CorruptedBasketArchive;
                }

                // Get the archive file:
                QString tempArchive = mDestination + "temp-archive.tar.gz";
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
                    KTar tar(tempArchive, "application/x-gzip");
                    tar.open(QIODevice::ReadOnly);
                    tar.directory()->copyTo(mDestination);
                    tar.close();
                }
            } else if (key.endsWith('*')) {
                // We do not know what it is, but we should read the embedded-file in
                // order to discard it:
                bool ok;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    Tools::deleteRecursively(mDestination);
                    return IOErrorCode::CorruptedBasketArchive;
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
                file.close();
                Tools::deleteRecursively(mDestination);
                return IOErrorCode::NotABasketArchive;
            }
            // Analyze the Value, if Understood:
        }
        file.close();
    }

    return retCode;
}

Archive::IOErrorCode Archive::createArchiveFromSource(const QString &sourcePath, const QString &previewImage, const QString &destination)
{
    QDir mSourcePath(sourcePath);
    QFileInfo destinationFile(destination);
    QFileInfo mPreviewImageFile(previewImage);

    // sourcePath must be a valid directory
    if (!mSourcePath.exists()) {
        return IOErrorCode::FailedToOpenResource;
    }

    // destinationFile must not previously exist;
    if (destinationFile.exists()) {
        return IOErrorCode::DestinationExists;
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return IOErrorCode::FailedToOpenResource;
    }

    // Create the temporary archive file:
    QString tempDestinationFile = tempDir.path() + "temp-archive.tar.gz";
    KTar archive(tempDestinationFile, "application/x-gzip");

    // Prepare the archive for writing.
    if (!archive.open(QIODevice::WriteOnly)) {
        // Failed to open file.
        archive.close();
        Tools::deleteRecursively(tempDir.path());
        return IOErrorCode::FailedToOpenResource;
    }

    // Add files and directories to tar archive
    const auto sourceFiles = mSourcePath.entryList(QDir::Files);
    std::for_each(sourceFiles.constBegin(), sourceFiles.constEnd(), [&](const QString &entry) {
        archive.addLocalFile(entry, entry);
    });
    const auto sourceDirectories = mSourcePath.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    std::for_each(sourceDirectories.constBegin(), sourceDirectories.constEnd(), [&](const QString &entry) {
        archive.addLocalDirectory(entry, entry);
    });
    archive.close();

    // use generic basket icon as preview if no valid image supplied
    /// \todo write a way to create preview the way it's done in Archive::save
    QString previewImagePath = ":/images/128-apps-org.kde.basket.png";
    if (!previewImage.isEmpty() && QFileInfo(previewImage).exists()) {
        previewImagePath = previewImage;
    }

    // Finally Save to the Real Destination file:
    QFile file(destination);
    if (file.open(QIODevice::WriteOnly)) {
        ulong previewSize = QFile(previewImagePath).size();
        ulong archiveSize = QFile(tempDestinationFile).size();
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
        QFile previewFile(previewImagePath);
        if (previewFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = previewFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        stream << "archive*:" << archiveSize << "\n";
        stream.flush();

        // Copy the Archive File:
        QFile archiveFile(tempDestinationFile);
        if (archiveFile.open(QIODevice::ReadOnly)) {
            while ((sizeRead = archiveFile.read(buffer, BUFFER_SIZE)) > 0)
                file.write(buffer, sizeRead);
        }
        // Clean Up:
        delete[] buffer;
        buffer = nullptr;
        file.close();
    }

    return IOErrorCode::NoError;
}

/**
 * When opening a basket archive that come from another computer,
 * it can contains tags that use icons (emblems) that are not present on that computer.
 * Fortunately, basket archives contains a copy of every used icons.
 * This method check for every emblems and import the missing ones.
 * It also modify the tags.xml copy for the emblems to point to the absolute path of the imported icons.
 */
void Archive::importTagEmblems(const QString &extractionFolder)
{
    QDomDocument *document = XMLWork::openFile("basketTags", extractionFolder + "tags.xml");
    if (document == nullptr)
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
                        QPixmap emblem = KIconLoader::global()->loadIcon(emblemName, KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, /*canReturnNull=*/true);
                        // The icon does not exists on that computer, import it:
                        if (emblem.isNull()) {
                            // Of the emblem path was eg. "/home/seb/emblem.png", it was exported as "tag-emblems/_home_seb_emblem.png".
                            // So we need to copy that image to "~/.local/share/basket/tag-emblems/emblem.png":
                            int slashIndex = emblemName.lastIndexOf('/');
                            QString emblemFileName = (slashIndex < 0 ? emblemName : emblemName.right(slashIndex - 2));
                            QString source = extractionFolder + "tag-emblems/" + emblemName.replace('/', '_');
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
    FileStorage::safelySaveToFile(extractionFolder + "tags.xml", document->toString());
}

void Archive::importArchivedBackgroundImages(const QString &extractionFolder)
{
    FormatImporter copier; // Only used to copy files synchronously
    QString destFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/basket/backgrounds/";
    QDir().mkpath(destFolder); // does not exist at the first run when addWelcomeBaskets is called

    QDir dir(extractionFolder + "backgrounds/", /*nameFilder=*/"*.png", /*sortSpec=*/QDir::Name | QDir::IgnoreCase, /*filterSpec=*/QDir::Files | QDir::NoSymLinks);
    QStringList files = dir.entryList();
    for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
        QString image = *it;
        if (!Global::backgroundManager->exists(image)) {
            // Copy images:
            QString imageSource = extractionFolder + "backgrounds/" + image;
            QString imageDest = destFolder + image;
            copier.copyFolder(imageSource, imageDest);
            // Copy configuration file:
            QString configSource = extractionFolder + "backgrounds/" + image + ".config";
            QString configDest = destFolder + image;
            if (dir.exists(configSource))
                copier.copyFolder(configSource, configDest);
            // Copy preview:
            QString previewSource = extractionFolder + "backgrounds/previews/" + image;
            QString previewDest = destFolder + "previews/" + image;
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
    if (doc != nullptr) {
        QMap<QString, QString> folderMap;
        QDomElement docElem = doc->documentElement();
        QDomNode node = docElem.firstChild();
        renameBasketFolder(extractionFolder, node, folderMap, mergedStates);
        loadExtractedBaskets(extractionFolder, node, folderMap, nullptr);
    }
}

void Archive::renameBasketFolder(const QString &extractionFolder, QDomNode &basketNode, QMap<QString, QString> &folderMap, QMap<QString, QString> &mergedStates)
{
    QDomNode n = basketNode;
    while (!n.isNull()) {
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
    if (doc == nullptr)
        return;
    QDomElement docElem = doc->documentElement();
    QDomElement properties = XMLWork::getElement(docElem, "properties");
    importBasketIcon(properties, extractionFolder);
    QDomElement notes = XMLWork::getElement(docElem, "notes");
    if (mergedStates.count() > 0)
        renameMergedStates(notes, mergedStates);
    FileStorage::safelySaveToFile(fullPath, /*"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n" + */ doc->toString());
}

void Archive::importBasketIcon(QDomElement properties, const QString &extractionFolder)
{
    QString iconName = XMLWork::getElementText(properties, "icon");
    if (!iconName.isEmpty() && iconName != "basket") {
        QPixmap icon = KIconLoader::global()->loadIcon(iconName, KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, /*canReturnNull=*/true);
        // The icon does not exists on that computer, import it:
        if (icon.isNull()) {
            QDir dir;
            dir.mkdir(Global::savesFolder() + "basket-icons/");
            FormatImporter copier; // Only used to copy files synchronously
            // Of the icon path was eg. "/home/seb/icon.png", it was exported as "basket-icons/_home_seb_icon.png".
            // So we need to copy that image to "~/.local/share/basket/basket-icons/icon.png":
            int slashIndex = iconName.lastIndexOf('/');
            QString iconFileName = (slashIndex < 0 ? iconName : iconName.right(slashIndex - 2));
            QString source = extractionFolder + "basket-icons/" + iconName.replace('/', '_');
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
    while (!n.isNull()) {
        QDomElement element = n.toElement();
        if (!element.isNull()) {
            if (element.tagName() == "group") {
                renameMergedStates(n, mergedStates);
            } else if (element.tagName() == "note") {
                QString tags = XMLWork::getElementText(element, "tags");
                if (!tags.isEmpty()) {
                    QStringList tagNames = tags.split(';');
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
    bool basketSetAsCurrent = (parent != nullptr);
    QDomNode n = basketNode;
    while (!n.isNull()) {
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
                BasketListViewItem *basketItem = Global::bnpView->appendBasket(basket, (basket && parent ? Global::bnpView->listViewItemForBasket(parent) : nullptr));
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
