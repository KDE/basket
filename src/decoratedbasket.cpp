/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "decoratedbasket.h"

#include <QGraphicsView>
#include <QVBoxLayout>

#include "basketscene.h"
#include "filter.h"
#include "settings.h"

/** Class DecoratedBasket: */

DecoratedBasket::DecoratedBasket(QWidget *parent, const QString &folderName, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    m_layout = new QVBoxLayout(this);
    m_filter = new FilterBar(this);
    m_basket = new BasketScene(this, folderName);
    m_basket->graphicsView()->setParent(this);
    m_layout->addWidget(m_basket->graphicsView());
    setFilterBarPosition(Settings::filterOnTop());

    m_filter->hide();
    m_basket->setFocus(); // To avoid the filter bar have focus on load

    connect(m_filter, SIGNAL(newFilter(const FilterData &)), m_basket, SLOT(newFilter(const FilterData &)));

    connect(m_basket, SIGNAL(postMessage(const QString &)), Global::bnpView, SLOT(postStatusbarMessage(const QString &)));
    connect(m_basket, SIGNAL(setStatusBarText(const QString &)), Global::bnpView, SLOT(setStatusBarHint(const QString &)));
    connect(m_basket, SIGNAL(resetStatusBarText()), Global::bnpView, SLOT(updateStatusBarHint()));
}

DecoratedBasket::~DecoratedBasket()
{
}

void DecoratedBasket::setFilterBarPosition(bool onTop)
{
    m_layout->removeWidget(m_filter);
    if (onTop) {
        m_layout->insertWidget(0, m_filter);
        setTabOrder(this /*(QWidget*)parent()*/, m_filter);
        setTabOrder(m_filter, m_basket->graphicsView());
        setTabOrder(m_basket->graphicsView(), (QWidget *)parent());
    } else {
        m_layout->addWidget(m_filter);
        setTabOrder(this /*(QWidget*)parent()*/, m_basket->graphicsView());
        setTabOrder(m_basket->graphicsView(), m_filter);
        setTabOrder(m_filter, (QWidget *)parent());
    }
}

void DecoratedBasket::setFilterBarVisible(bool show, bool switchFocus)
{
    //  m_basket->setShowFilterBar(true);//show);
    //  m_basket->save();
    // In this order (m_basket and then m_filter) because setShown(false)
    //  will call resetFilter() that will update actions, and then check the
    //  Ctrl+F action whereas it should be unchecked
    //  FIXME: It's very uggly all those things
    m_filter->setVisible(show);
    if (show) {
        if (switchFocus)
            m_filter->setEditFocus();
    } else if (m_filter->hasEditFocus())
        m_basket->setFocus();
}

void DecoratedBasket::resetFilter()
{
    m_filter->reset();
}

void DecoratedBasket::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_basket->relayoutNotes();
}
