/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HTMLEXPORTER_H
#define HTMLEXPORTER_H

#include <QtCore/QString>
#include <QtCore/QTextStream>

class QProgressDialog;

class BasketScene;
class Note;

/**
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class HTMLExporter
{
public:
    explicit HTMLExporter(BasketScene *basket);
    ~HTMLExporter();

private:
    void prepareExport(BasketScene *basket, const QString &fullPath);
    void exportBasket(BasketScene *basket, bool isSubBasket);
    void exportNote(Note *note, int indent);
    void writeBasketTree(BasketScene *currentBasket);
    void writeBasketTree(BasketScene *currentBasket, BasketScene *basket, int indent);
    void saveToFile(const QString &fullPath, const QByteArray &array);

public:
    QString copyIcon(const QString &iconName, int size);
    QString copyFile(const QString &srcPath, bool createIt);

public: // TODO: make private?
    // Absolute path of the file name the user chosen:
    QString filePath; // eg.: "/home/seb/foo.html"
    QString fileName; // eg.: "foo.html"

    // Absolute & relative paths for the current basket to be exported:
    QString basketFilePath;    // eg.: "/home/seb/foo.html" or "/home/seb/foo.html_files/baskets/basketN.html"
    QString filesFolderPath;   // eg.: "/home/seb/foo.html_files/"
    QString filesFolderName;   // eg.: "foo.html_files/" or "../"
    QString iconsFolderPath;   // eg.: "/home/seb/foo.html_files/icons/"
    QString iconsFolderName;   // eg.: "foo.html_files/icons/" or "../icons/"
    QString imagesFolderPath;  // eg.: "/home/seb/foo.html_files/images/"
    QString imagesFolderName;  // eg.: "foo.html_files/images/" or "../images/"
    QString dataFolderPath;    // eg.: "/home/seb/foo.html_files/data/" or "/home/seb/foo.html_files/baskets/basketN-data/"
    QString dataFolderName;    // eg.: "foo.html_files/data/" or "basketN-data/"
    QString basketsFolderPath; // eg.: "/home/seb/foo.html_files/baskets/"
    QString basketsFolderName; // eg.: "foo.html_files/baskets/" or QString()

    // Various properties of the currently exporting basket:
    QString backgroundColorName;

    // Variables used by every export methods:
    QTextStream stream;
    BasketScene *exportedBasket;
    BasketScene *currentBasket;
    bool withBasketTree;
    QScopedPointer<QProgressDialog> dialog;
};

#endif // HTMLEXPORTER_H
