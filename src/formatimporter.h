/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FORMATIMPORTER_H
#define FORMATIMPORTER_H

#include <QtCore/QObject>

class QDomElement;

namespace KIO
{
class Job;
}

/**
 * @author Sébastien Laoût
 */
class FormatImporter : public QObject
{
    Q_OBJECT
public:
    static bool shouldImportBaskets();
    static void importBaskets();
    static QDomElement importBasket(const QString &folderName);

    void copyFolder(const QString &folder, const QString &newFolder);
    void moveFolder(const QString &folder, const QString &newFolder);
private Q_SLOTS:
    void slotCopyingDone(KIO::Job *);

private:
    bool copyFinished;
};

#endif // FORMATIMPORTER_H
