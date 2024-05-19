/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/** Common methods extracted here to simplify the dependency between classes
 */

#ifndef BASKET_COMMON_H
#define BASKET_COMMON_H

class QByteArray;
class QString;

namespace FileStorage {
    bool loadFromFile(const QString &fullPath, QString *string);
    bool loadFromFile(const QString &fullPath, QByteArray *array);
    bool saveToFile(const QString &fullPath, const QString &string, bool isEncrypted = false);
    bool saveToFile(const QString &fullPath, const QByteArray &array, bool isEncrypted = false); //[Encrypt and] save binary content
    bool safelySaveToFile(const QString &fullPath, const QByteArray &array);
    bool safelySaveToFile(const QString &fullPath, const QString &string);
}

#endif
