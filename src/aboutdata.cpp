/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "aboutdata.h"
#include <config.h>

#include <KLocalizedString>
#include <QApplication>

static const char description[] = I18N_NOOP(
    "<p><b>Taking care of your ideas.</b></p>"
    "<p>A note-taking application that makes it easy to record ideas as you think, and quickly find them later. "
    "Organizing your notes has never been so easy.</p>");

// Or how to make order of disorganized thoughts.

AboutData::AboutData()
    : KAboutData(AboutData::componentName(),
                 AboutData::displayName(),
                 VERSION,
                 i18n(description),
                 KAboutLicense::GPL_V2,
                 i18n("Copyright © 2003–2007, Sébastien Laoût; Copyright © 2013–2019, Gleb Baryshev"),
                 QString(),
                 "https://basket.kde.org/")
{
    // Pass basket.kde.org to constructor to be used as D-Bus domain name, but set the displayed address below
    setHomepage("https://cgit.kde.org/basket.git/");
    setBugAddress("https://bugs.kde.org/buglist.cgi?component=general&list_id=1738678&product=basket&resolution=---");

    addAuthor(i18n("OmegaPhil"), i18n("Paste as plaintext option"), "OmegaPhil@startmail.com");

    addAuthor(i18n("Kelvie Wong"), i18n("Maintainer"), "kelvie@ieee.org");

    addAuthor(i18n("Sébastien Laoût"), i18n("Original Author"), "slaout@linux62.org");

    addAuthor(i18n("Petri Damstén"), i18n("Basket encryption, Kontact integration, KnowIt importer"), "damu@iki.fi");

    addAuthor(i18n("Alex Gontmakher"), i18n("Baskets auto lock, save-status icon, HTML copy/paste, basket name tooltip, drop to basket name"), "gsasha@cs.technion.ac.il");

    addAuthor(i18n("Marco Martin"), i18n("Original icon"), "m4rt@libero.it");
}

QString AboutData::componentName()
{
    return QString("basket");
}

QString AboutData::displayName()
{
    return i18n("BasKet Note Pads");
}
