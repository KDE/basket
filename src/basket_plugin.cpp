/**
 * SPDX-FileCopyrightText: (C) 2009 by Robert Marmorstein <robert@narnia.homeunix.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basket_plugin.h"
#include "global.h"

#include "basket_part.h"

#include <KontactInterface/Core>

EXPORT_KONTACT_PLUGIN(BasketPlugin, basket)

BasketPlugin::BasketPlugin(KontactInterface::Core *core, const QVariantList &)
    : KontactInterface::Plugin(core, core, "Basket")
{
    setComponentData(KontactPluginFactory::componentData());
    Global::basketConfig = KSharedConfig::openConfig("basketrc");
}

BasketPlugin::~BasketPlugin()
{
}

KParts::ReadOnlyPart *BasketPlugin::createPart()
{
    KParts::ReadOnlyPart *part = loadPart();

    connect(part, SIGNAL(showPart()), this, SLOT(showPart()));

    return part;
}

void BasketPlugin::readProperties(const KConfigGroup &config)
{
    if (part()) {
        BasketPart *myPart = static_cast<BasketPart *>(part());
    }
}

void BasketPlugin::saveProperties(KConfigGroup &config)
{
    if (part()) {
        BasketPart *myPart = static_cast<BasketPart *>(part());
    }
}

void BasketPlugin::showPart()
{
    core()->selectPlugin(this);
}
