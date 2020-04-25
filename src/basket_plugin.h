/**
 * SPDX-FileCopyrightText: (C) 2009 by Robert Marmorstein <robert@narnia.homeunix.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKET_PLUGIN_H
#define BASKET_PLUGIN_H

#include <KontactInterface/Plugin>

namespace KParts
{
class ReadOnlyPart;
}

class BasketPlugin : public KontactInterface::Plugin
{
    Q_OBJECT

public:
    BasketPlugin(KontactInterface::Core *core, const QVariantList &);
    ~BasketPlugin();

    virtual void readProperties(const KConfigGroup &config);
    virtual void saveProperties(KConfigGroup &config);

private slots:
    void showPart();

protected:
    KParts::ReadOnlyPart *createPart();
};

#endif
