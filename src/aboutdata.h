/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ABOUTDATA_H
#define ABOUTDATA_H

#include "basket_export.h"
#include <KAboutData>

/**
 * @class AboutData
 * @brief Store all the information about the application
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class BASKET_EXPORT AboutData : public KAboutData
{
public:
    static QString componentName();
    static QString displayName();

    AboutData();
};

#endif
