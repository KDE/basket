/**
 * SPDX-FileCopyrightText: (C) (C) 2009 Matt Rogers <mattr@kde.org>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef BASKET_EXPORT_H
#define BASKET_EXPORT_H

#ifndef BASKET_EXPORT
#if defined(MAKE_BASKETCOMMON_LIB)
/* We are building this library */
#define BASKET_EXPORT Q_DECL_EXPORT
#else
/* We are using this library */
#define BASKET_EXPORT Q_DECL_IMPORT
#endif
#endif

#ifndef BASKET_EXPORT_DEPRECATED
#define BASKET_EXPORT_DEPRECATED KDE_DEPRECATED BASKET_EXPORT
#endif

#endif
