/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketview.h"

#include "basketscene.h"

BasketView::BasketView(BasketScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
}

BasketView::~BasketView()
{
}

void BasketView::resizeEvent(QResizeEvent *)
{
    static_cast<BasketScene *>(scene())->relayoutNotes();
}

#include "moc_basketview.cpp"
