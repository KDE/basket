/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "formatimporter.h"

#include <QApplication>
#include <QLocale>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtXml/QDomDocument>

#include <KIO/CopyJob>
#include <KLocalizedString>
#include <KMessageBox>

#include "basketscene.h"
#include "bnpview.h"
#include "global.h"
#include "notecontent.h"
#include "notefactory.h"
#include "tools.h"
#include "xmlwork.h"

bool FormatImporter::shouldImportBaskets()
{
    // We should import if the application have not successfully loaded any basket...
    if (Global::bnpView->topLevelItemCount() >= 0)
        return false;

    // ... And there is at least one folder in the save folder, with a ".basket" file inside that folder.
    QDir dir(Global::savesFolder(), QString(), QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::NoSymLinks);
    QStringList list = dir.entryList();
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
        if (*it != QStringLiteral(".") && *it != QStringLiteral("..") && dir.exists(Global::savesFolder() + *it + QStringLiteral("/.basket")))
            return true;

    return false;
}

void FormatImporter::copyFolder(const QString &folder, const QString &newFolder)
{
    copyFinished = false;
    KIO::CopyJob *copyJob = KIO::copyAs(QUrl::fromLocalFile(folder), QUrl::fromLocalFile(newFolder), KIO::HideProgressInfo);
    connect(copyJob, &KIO::CopyJob::copyingDone, this, &FormatImporter::slotCopyingDone);
    while (!copyFinished)
        qApp->processEvents();
}

void FormatImporter::moveFolder(const QString &folder, const QString &newFolder)
{
    copyFinished = false;
    KIO::CopyJob *copyJob = KIO::moveAs(QUrl::fromLocalFile(folder), QUrl::fromLocalFile(newFolder), KIO::HideProgressInfo);
    connect(copyJob, &KIO::CopyJob::copyingDone, this, &FormatImporter::slotCopyingDone);
    while (!copyFinished)
        qApp->processEvents();
}

void FormatImporter::slotCopyingDone(KIO::Job *)
{
    //  qDebug() << "Copy finished of " + from.path() + " to " + to.path();
    copyFinished = true;
}

void FormatImporter::importBaskets()
{
    qDebug() << "Import Baskets: Preparing...";

    // Some preliminary preparations (create the destination folders and the basket tree file):
    QDir dirPrep;
    dirPrep.mkdir(Global::savesFolder());
    dirPrep.mkdir(Global::basketsFolder());
    QDomDocument document(QStringLiteral("basketTree"));
    QDomElement root = document.createElement(QStringLiteral("basketTree"));
    document.appendChild(root);

    // First up, establish a list of every baskets, ensure the old order (if any), and count them.
    QStringList baskets;

    // Read the 0.5.0 baskets order:
    QDomDocument *doc = XMLWork::openFile(QStringLiteral("container"), Global::savesFolder() + QStringLiteral("container.baskets"));
    if (doc != nullptr) {
        QDomElement docElem = doc->documentElement();
        QDomElement basketsElem = XMLWork::getElement(docElem, QStringLiteral("baskets"));
        QDomNode n = basketsElem.firstChild();
        while (!n.isNull()) {
            QDomElement e = n.toElement();
            if ((!e.isNull()) && e.tagName() == QStringLiteral("basket"))
                baskets.append(e.text());
            n = n.nextSibling();
        }
    }

    // Then load the baskets that weren't loaded (import < 0.5.0 ones):
    QDir dir(Global::savesFolder(), QString(), QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::NoSymLinks);
    QStringList list = dir.entryList();
    if (list.count() > 2)                                                                          // Pass "." and ".."
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)                      // For each folder
            if (*it != QStringLiteral(".") && *it != QStringLiteral("..") && dir.exists(Global::savesFolder() + *it + QStringLiteral("/.basket"))) // If it can be a basket folder
                if (!(baskets.contains((*it) + QLatin1Char('/'))) && baskets.contains(*it))                     // And if it is not already in the imported baskets list
                    baskets.append(*it);

    qDebug() << "Import Baskets: Found " << baskets.count() << " baskets to import.";

    // Import every baskets:
    int i = 0;
    for (QStringList::iterator it = baskets.begin(); it != baskets.end(); ++it) {
        ++i;
        qDebug() << "Import Baskets: Importing basket " << i << " of " << baskets.count() << "...";

        // Move the folder to the new repository (normal basket) or copy the folder (mirrored folder):
        QString folderName = *it;
        if (folderName.startsWith(QLatin1Char('/'))) { // It was a folder mirror:
            KMessageBox::information(nullptr,
                                     i18n("<p>Folder mirroring is not possible anymore (see <a href='https://basket-notepads.github.io'>basket-notepads.github.io</a> for more information).</p>"
                                          "<p>The folder <b>%1</b> has been copied for the basket needs. You can either delete this folder or delete the basket, or use both. But remember that "
                                          "modifying one will not modify the other anymore as they are now separate entities.</p>",
                                          folderName),
                                     i18n("Folder Mirror Import"),
                                     QString(),
                                     KMessageBox::AllowLink);
            // Also modify folderName to be only the folder name and not the full path anymore:
            QString newFolderName = folderName;
            if (newFolderName.endsWith(QLatin1Char('/')))
                newFolderName = newFolderName.left(newFolderName.length() - 1);
            newFolderName = newFolderName.mid(newFolderName.lastIndexOf(QLatin1Char('/')) + 1);
            newFolderName = Tools::fileNameForNewFile(newFolderName, Global::basketsFolder());
            FormatImporter f;
            f.copyFolder(folderName, Global::basketsFolder() + newFolderName);
            folderName = newFolderName;
        } else
            dir.rename(Global::savesFolder() + folderName, Global::basketsFolder() + folderName); // Move the folder

        // Import the basket structure file and get the properties (to add them in the tree basket-properties cache):
        QDomElement properties = importBasket(folderName);

        // Add it to the XML document:
        QDomElement basketElement = document.createElement(QStringLiteral("basket"));
        root.appendChild(basketElement);
        basketElement.setAttribute(QStringLiteral("folderName"), folderName);
        basketElement.appendChild(properties);
    }

    // Finalize (write to disk and delete now useless files):
    qDebug() << "Import Baskets: Finalizing...";

    QFile file(Global::basketsFolder() + QStringLiteral("baskets.xml"));
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        QString xml = document.toString();
        stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
        stream << xml;
        file.close();
    }

    Tools::deleteRecursively(Global::savesFolder() + QStringLiteral(".tmp"));
    dir.remove(Global::savesFolder() + QStringLiteral("container.baskets"));

    qDebug() << "Import Baskets: Finished.";
}

QDomElement FormatImporter::importBasket(const QString &folderName)
{
    // Load the XML file:
    QDomDocument *document = XMLWork::openFile(QStringLiteral("basket"), Global::basketsFolder() + folderName + QStringLiteral("/.basket"));
    if (!document) {
        qDebug() << "Import Baskets: Failed to read the basket file!";
        return QDomElement();
    }
    QDomElement docElem = document->documentElement();

    // Import properties (change <background color=""> to <appearance backgroundColor="">, and figure out if is a checklist or not):
    QDomElement properties = XMLWork::getElement(docElem, QStringLiteral("properties"));
    QDomElement background = XMLWork::getElement(properties, QStringLiteral("background"));
    QColor backgroundColor = QColor(background.attribute(QStringLiteral("color")));
    if (backgroundColor.isValid() && (backgroundColor != qApp->palette().color(QPalette::Base))) { // Use the default color if it was already that color:
        QDomElement appearance = document->createElement(QStringLiteral("appearance"));
        appearance.setAttribute(QStringLiteral("backgroundColor"), backgroundColor.name());
        properties.appendChild(appearance);
    }
    QDomElement disposition = document->createElement(QStringLiteral("disposition"));
    disposition.setAttribute(QStringLiteral("mindMap"), QStringLiteral("false"));
    disposition.setAttribute(QStringLiteral("columnCount"), QStringLiteral("1"));
    disposition.setAttribute(QStringLiteral("free"), QStringLiteral("false"));
    bool isCheckList = XMLWork::trueOrFalse(XMLWork::getElementText(properties, QStringLiteral("showCheckBoxes")), false);

    // Insert all notes in a group (column): 1/ rename "items" to "group", 2/ add "notes" to root, 3/ move "group" into "notes"
    QDomElement column = XMLWork::getElement(docElem, QStringLiteral("items"));
    column.setTagName(QStringLiteral("group"));
    QDomElement notes = document->createElement(QStringLiteral("notes"));
    notes.appendChild(column);
    docElem.appendChild(notes);

    // Import notes from older representations:
    QDomNode n = column.firstChild();
    while (!n.isNull()) {
        QDomElement e = n.toElement();
        if (!e.isNull()) {
            e.setTagName(QStringLiteral("note"));
            QDomElement content = XMLWork::getElement(e, QStringLiteral("content"));
            // Add Check tag:
            if (isCheckList) {
                bool isChecked = XMLWork::trueOrFalse(e.attribute(QStringLiteral("checked"), QStringLiteral("false")));
                XMLWork::addElement(*document, e, QStringLiteral("tags"), (isChecked ? QStringLiteral("todo_done") : QStringLiteral("todo_unchecked")));
            }
            // Import annotations as folded groups:
            QDomElement parentE = column;
            QString annotations = XMLWork::getElementText(e, QStringLiteral("annotations"), QString());
            if (!annotations.isEmpty()) {
                QDomElement annotGroup = document->createElement(QStringLiteral("group"));
                column.insertBefore(annotGroup, e);
                annotGroup.setAttribute(QStringLiteral("folded"), QStringLiteral("true"));
                annotGroup.appendChild(e);
                parentE = annotGroup;
                // Create the text note and add it to the DOM tree:
                QDomElement annotNote = document->createElement(QStringLiteral("note"));
                annotNote.setAttribute(QStringLiteral("type"), QStringLiteral("text"));
                annotGroup.appendChild(annotNote);
                QString annotFileName = Tools::fileNameForNewFile(QStringLiteral("annotations1.txt"), BasketScene::fullPathForFolderName(folderName));
                QString annotFullPath = BasketScene::fullPathForFolderName(folderName) + QLatin1Char('/') + annotFileName;
                QFile file(annotFullPath);
                if (file.open(QIODevice::WriteOnly)) {
                    QTextStream stream(&file);
                    stream << annotations;
                    file.close();
                }
                XMLWork::addElement(*document, annotNote, QStringLiteral("content"), annotFileName);
                n = annotGroup;
            }
            // Import Launchers from 0.3.x, 0.4.0 and 0.5.0-alphas:
            QString serviceLauncher = e.attribute(QStringLiteral("runcommand"));                // Keep compatibility with 0.4.0 and 0.5.0-alphas versions
            serviceLauncher = XMLWork::getElementText(e, QStringLiteral("action"), serviceLauncher); // Keep compatibility with 0.3.x versions
            if (!serviceLauncher.isEmpty()) {                                   // An import should be done
                // Prepare the launcher note:
                QString title = content.attribute(QStringLiteral("title"), QString());
                QString icon = content.attribute(QStringLiteral("icon"), QString());
                if (title.isEmpty())
                    title = serviceLauncher;
                if (icon.isEmpty())
                    icon = NoteFactory::iconForCommand(serviceLauncher);
                // Import the launcher note:
                // Adapted version of "QString launcherName = NoteFactory::createNoteLauncherFile(serviceLauncher, title, icon, this)":
                QString launcherContent = QStringLiteral(
                                              "[Desktop Entry]\n"
                                              "Exec=%1\n"
                                              "Name=%2\n"
                                              "Icon=%3\n"
                                              "Encoding=UTF-8\n"
                                              "Type=Application\n")
                                              .arg(serviceLauncher, title, icon.isEmpty() ? QStringLiteral("exec") : icon);
                QString launcherFileName = Tools::fileNameForNewFile(QStringLiteral("launcher.desktop"), Global::basketsFolder() + folderName /*+ "/"*/);
                QString launcherFullPath = Global::basketsFolder() + folderName /*+ "/"*/ + launcherFileName;
                QFile file(launcherFullPath);
                if (file.open(QIODevice::WriteOnly)) {
                    QTextStream stream(&file);
                    stream << launcherContent;
                    file.close();
                }
                // Add the element to the DOM:
                QDomElement launcherElem = document->createElement(QStringLiteral("note"));
                parentE.insertBefore(launcherElem, e);
                launcherElem.setAttribute(QStringLiteral("type"), QStringLiteral("launcher"));
                XMLWork::addElement(*document, launcherElem, QStringLiteral("content"), launcherFileName);
            }
            // Import unknown ns to 0.6.0:
            if (e.attribute(QStringLiteral("type")) == QStringLiteral("unknow"))
                e.setAttribute(QStringLiteral("type"), QStringLiteral("unknown"));
            // Import links from version < 0.5.0:
            if (!content.attribute(QStringLiteral("autotitle")).isEmpty() && content.attribute(QStringLiteral("autoTitle")).isEmpty())
                content.setAttribute(QStringLiteral("autoTitle"), content.attribute(QStringLiteral("autotitle")));
            if (!content.attribute(QStringLiteral("autoicon")).isEmpty() && content.attribute(QStringLiteral("autoIcon")).isEmpty())
                content.setAttribute(QStringLiteral("autoIcon"), content.attribute(QStringLiteral("autoicon")));
        }
        n = n.nextSibling();
    }

    // Save the resulting XML file:
    QFile file(Global::basketsFolder() + folderName + QStringLiteral("/.basket"));
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        //      QString xml = document->toString();
        //      stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
        //      stream << xml;
        stream << document->toString(); // Document is ALREADY using UTF-8
        file.close();
    } else
        qDebug() << "Import Baskets: Failed to save the basket file!";

    // Return the newly created properties (to put in the basket tree):
    return properties;
}

#include "moc_formatimporter.cpp"
