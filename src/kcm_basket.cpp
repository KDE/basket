/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// This must be first
#include "settings.h" //For GeneralPage, etc.
#include "settings_versionsync.h"
#include <KCModule>
#include <config.h>

//----------------------------
// KCM stuff
//----------------------------
extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_general(QWidget *parent, const char *)
{
    GeneralPage *page = new GeneralPage(parent, "kcmbasket_config_general");
    return page;
}
}

extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_baskets(QWidget *parent, const char *)
{
    BasketsPage *page = new BasketsPage(parent, "kcmbasket_config_baskets");
    return page;
}
}

extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_new_notes(QWidget *parent, const char *)
{
    NewNotesPage *page = new NewNotesPage(parent, "kcmbasket_config_new_notes");
    return page;
}
}

extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_notes_appearance(QWidget *parent, const char *)
{
    NotesAppearancePage *page = new NotesAppearancePage(parent, "kcmbasket_config_notes_appearance");
    return page;
}
}

extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_apps(QWidget *parent, const char *)
{
    ApplicationsPage *page = new ApplicationsPage(parent, "kcmbasket_config_apps");
    return page;
}
}

extern "C" {
Q_DECL_EXPORT KCModule *create_basket_config_version_sync(QWidget *parent, const char *)
{
    VersionSyncPage *page = new VersionSyncPage(parent, "kcmbasket_config_version_sync");
    return page;
}
}
