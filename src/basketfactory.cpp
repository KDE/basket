/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketfactory.h"

#include <QDir>
#include <QGraphicsView>
#include <QLocale>
#include <QTextStream>
#include <QtXml/QDomElement>

#include <KLocalizedString>
#include <KMessageBox>

#include "basketscene.h"
#include "bnpview.h"
#include "global.h"
#include "note.h" // For balanced column width computation
#include "xmlwork.h"

/** BasketFactory */

// TODO: Don't create a basket with a name that already exists!

QString BasketFactory::newFolderName()
{
    QString folderName;
    QString fullPath;
    QDir dir;

    int i = QDir(Global::basketsFolder()).count();
    QString time = QTime::currentTime().toString(QStringLiteral("hhmmss"));

    for (;; ++i) {
        folderName = QStringLiteral("basket%1-%2/").arg(i).arg(time);
        fullPath = Global::basketsFolder() + folderName;
        dir = QDir(fullPath);
        if (!dir.exists()) // OK : The folder do not yet exists :
            break; //  We've found one !
    }

    return folderName;
}

QString BasketFactory::unpackTemplate(const QString &templateName)
{
    // Find a name for a new folder and create it:
    QString folderName = newFolderName();
    QString fullPath = Global::basketsFolder() + folderName;
    QDir dir;
    if (!dir.mkpath(fullPath)) {
        KMessageBox::error(/*parent=*/nullptr, i18n("Sorry, but the folder creation for this new basket has failed."), i18n("Basket Creation Failed"));
        return {};
    }

    // Unpack the template file to that folder:
    // TODO: REALLY unpack (this hand-creation is temporary, or it could be used in case the template can't be found)
    QFile file(fullPath + QStringLiteral("/.basket"));
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        int nbColumns = (templateName == QStringLiteral("mindmap") || templateName == QStringLiteral("free") ? 0 : templateName.left(1).toInt());
        BasketScene *currentBasket = Global::bnpView->currentBasket();
        int columnWidth =
            (currentBasket && nbColumns > 0 ? (currentBasket->graphicsView()->viewport()->width() - (nbColumns - 1) * Note::RESIZER_WIDTH) / nbColumns : 0);
        stream << QStringLiteral(
                      "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                      "<!DOCTYPE basket>\n"
                      "<basket>\n"
                      " <properties>\n"
                      "  <disposition mindMap=\"%1\" columnCount=\"%2\" free=\"%3\" />\n"
                      " </properties>\n"
                      " <notes>\n")
                      .arg((templateName == QStringLiteral("mindmap") ? QStringLiteral("true") : QStringLiteral("false")),
                           QString::number(nbColumns),
                           (templateName == QStringLiteral("free") || templateName == QStringLiteral("mindmap") ? QStringLiteral("true")
                                                                                                                : QStringLiteral("false")));
        if (nbColumns > 0)
            for (int i = 0; i < nbColumns; ++i)
                stream << QStringLiteral("  <group width=\"%1\"></group>\n").arg(QLatin1Char(columnWidth));
        stream << " </notes>\n"
                  "</basket>\n";
        file.close();
        return folderName;
    } else {
        KMessageBox::error(nullptr, i18n("Sorry, but the template copying for this new basket has failed."), i18n("Basket Creation Failed"));
        return {};
    }
}

void BasketFactory::newBasket(const QString &icon,
                              const QString &name,
                              BasketScene *parent,
                              const QString &backgroundImage,
                              const QColor &backgroundColor,
                              const QColor &textColor,
                              const QString &templateName)
{
    // Unpack the templateName file to a new basket folder:
    QString folderName = unpackTemplate(templateName);
    if (folderName.isEmpty())
        return;

    // Read the properties, change those that should be customized and save the result:
    QDomDocument *document = XMLWork::openFile(QStringLiteral("basket"), Global::basketsFolder() + folderName + QStringLiteral("/.basket"));
    if (!document) {
        KMessageBox::error(/*parent=*/nullptr, i18n("Sorry, but the template customization for this new basket has failed."), i18n("Basket Creation Failed"));
        return;
    }
    QDomElement properties = XMLWork::getElement(document->documentElement(), QStringLiteral("properties"));

    if (!icon.isEmpty()) {
        QDomElement iconElement = XMLWork::getElement(properties, QStringLiteral("icon"));
        if (!iconElement.tagName().isEmpty()) // If there is already an icon, remove it since we will add our own value below
            iconElement.removeChild(iconElement.firstChild());
        XMLWork::addElement(*document, properties, QStringLiteral("icon"), icon);
    }

    if (!name.isEmpty()) {
        QDomElement nameElement = XMLWork::getElement(properties, QStringLiteral("name"));
        if (!nameElement.tagName().isEmpty()) // If there is already a name, remove it since we will add our own value below
            nameElement.removeChild(nameElement.firstChild());
        XMLWork::addElement(*document, properties, QStringLiteral("name"), name);
    }

    if (backgroundColor.isValid()) {
        QDomElement appearanceElement = XMLWork::getElement(properties, QStringLiteral("appearance"));
        if (appearanceElement.tagName().isEmpty()) { // If there is not already an appearance tag, add it since we will access it below
            appearanceElement = document->createElement(QStringLiteral("appearance"));
            properties.appendChild(appearanceElement);
        }
        appearanceElement.setAttribute(QStringLiteral("backgroundColor"), backgroundColor.name());
    }

    if (!backgroundImage.isEmpty()) {
        QDomElement appearanceElement = XMLWork::getElement(properties, QStringLiteral("appearance"));
        if (appearanceElement.tagName().isEmpty()) { // If there is not already an appearance tag, add it since we will access it below
            appearanceElement = document->createElement(QStringLiteral("appearance"));
            properties.appendChild(appearanceElement);
        }
        appearanceElement.setAttribute(QStringLiteral("backgroundImage"), backgroundImage);
    }

    if (textColor.isValid()) {
        QDomElement appearanceElement = XMLWork::getElement(properties, QStringLiteral("appearance"));
        if (appearanceElement.tagName().isEmpty()) { // If there is not already an appearance tag, add it since we will access it below
            appearanceElement = document->createElement(QStringLiteral("appearance"));
            properties.appendChild(appearanceElement);
        }
        appearanceElement.setAttribute(QStringLiteral("textColor"), textColor.name());
    }

    // Load it in the parent basket (it will save the tree and switch to this new basket):
    Global::bnpView->loadNewBasket(folderName, properties, parent);
}
