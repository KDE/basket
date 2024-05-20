/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "aboutdata.h"
#include <config.h>

#include <KLocalizedString>
#include <QApplication>


AboutData::AboutData()
    : KAboutData(AboutData::componentName(),
                 AboutData::displayName(),
                 QStringLiteral(VERSION),
                 i18n("<p><b>Taking care of your ideas.</b></p>"
    "<p>A note-taking application that makes it easy to record ideas as you think, and quickly find them later. "
    "Organizing your notes has never been so easy.</p>"),
                 KAboutLicense::GPL_V2,
                 i18n("Copyright © 2003–2007, Sébastien Laoût; Copyright © 2013–2019, Gleb Baryshev"),
                 QString(),
                 QStringLiteral("https://basket.kde.org/"))
{
    // Pass basket.kde.org to constructor to be used as D-Bus domain name, but set the displayed address below
    setHomepage(QStringLiteral("https://invent.kde.org/utilities/basket"));
    setBugAddress("https://bugs.kde.org/enter_bug.cgi?format=guided&amp;product=basket");

    addAuthor(i18n("Carl Schwan"), i18n("Co-Maintainer"), QStringLiteral("carl@carlschwan.eu"), QStringLiteral("https://carlschwan.eu"));
    addAuthor(i18n("Niccolò Venerandi"), i18n("Co-Maintainer"), QStringLiteral("niccolo@venerandi.com"), QStringLiteral("https://niccolo.venerandi.com/"));
    addAuthor(i18n("OmegaPhil"), i18n("Paste as plaintext option"), QStringLiteral("OmegaPhil@startmail.com"));
    addAuthor(i18n("Kelvie Wong"), i18n("Ex-Maintainer"), QStringLiteral("kelvie@ieee.org"));
    addAuthor(i18n("Sébastien Laoût"), i18n("Original Author"), QStringLiteral("slaout@linux62.org"));
    addAuthor(i18n("Petri Damstén"), i18n("Basket encryption, Kontact integration, KnowIt importer"), QStringLiteral("damu@iki.fi"));
    addAuthor(i18n("Alex Gontmakher"), i18n("Baskets auto lock, save-status icon, HTML copy/paste, basket name tooltip, drop to basket name"), QStringLiteral("gsasha@cs.technion.ac.il"));
    addAuthor(i18n("Marco Martin"), i18n("Original icon"), QStringLiteral("m4rt@libero.it"));
}

QString AboutData::componentName()
{
    return QStringLiteral("basket");
}

QString AboutData::displayName()
{
    return i18n("BasKet Note Pads");
}
