/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKETFACTORY_H
#define BASKETFACTORY_H

class QColor;
class QString;

class BasketScene;

/** Methods to create various baskets (mkdir, init the properties and load it).
 * @author Sébastien Laoût
 */
namespace BasketFactory
{
/** You should use this method to create a new basket: */
void newBasket(const QString &icon, const QString &name, const QString &backgroundImage, const QColor &backgroundColor, const QColor &textColor, const QString &templateName, BasketScene *parent);
/** Internal tool methods to process the method above: */
QString newFolderName();
QString unpackTemplate(const QString &templateName);
}

#endif // BASKETFACTORY_H
