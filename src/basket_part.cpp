/**
 * SPDX-FileCopyrightText: (C) 2003 by Petri Damsten <petri.damsten@iki.fi>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basket_part.h"

#include <KParts/StatusBarExtension>

#include "aboutdata.h"
#include "basketstatusbar.h"
#include "bnpview.h"

K_PLUGIN_FACTORY_DEFINITION(BasketFactory, registerPlugin<BasketPart>();)

BasketPart::BasketPart(QWidget *parentWidget, QObject *parent, const QList<QVariant> &)
    : KParts::ReadWritePart(parent)
{
    // we need an instance
    // setInstance( BasketFactory::instance() );

    BasketStatusBar *bar = new BasketStatusBar(new KParts::StatusBarExtension(this));
    // this should be your custom internal widget
    m_view = new BNPView(parentWidget, "BNPViewPart", this, actionCollection(), bar);
    connect(m_view, SIGNAL(setWindowCaption(const QString &)), this, SLOT(setWindowTitle(const QString &)));
    connect(m_view, SIGNAL(showPart()), this, SIGNAL(showPart()));
    m_view->setFocusPolicy(Qt::ClickFocus);

    // notify the part that this is our internal widget
    setWidget(m_view);

    setComponentName(AboutData::componentName(), AboutData::displayName());

    // set our XML-UI resource file
    setXMLFile("basket_part.rc", true);

    // we are read-write by default
    setReadWrite(true);

    // we are not modified since we haven't done anything yet
    setModified(false);
}

BasketPart::~BasketPart()
{
}

void BasketPart::setReadWrite(bool rw)
{
    // TODO: notify your internal widget of the read-write state
    ReadWritePart::setReadWrite(rw);
}

void BasketPart::setModified(bool modified)
{
    // in any event, we want our parent to do it's thing
    ReadWritePart::setModified(modified);
}

bool BasketPart::openFile()
{
    // TODO
    return false;
}

bool BasketPart::saveFile()
{
    // TODO
    return false;
}

KAboutData *BasketPart::createAboutData()
{
    return new AboutData();
}

void BasketPart::setWindowTitle(const QString &caption)
{
    emit setWindowCaption(caption);
}
