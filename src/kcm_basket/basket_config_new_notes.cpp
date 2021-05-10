/*
   SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
   SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "settings.h"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(NewNotesPage, "basket_config_new_notes.json")

#include "basket_config_new_notes.moc"
