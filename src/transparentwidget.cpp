/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "transparentwidget.h"
#include "basketscene.h"
#include "basketview.h"

#include <QGraphicsView>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>

/** Class TransparentWidget */

// TODO: Why was Qt::WNoAutoErase used here?
TransparentWidget::TransparentWidget(BasketScene *basket)
    : QWidget(basket->graphicsView()->viewport())
    , m_basket(basket)
{
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true); // To receive mouseMoveEvents

    basket->graphicsView()->viewport()->installEventFilter(this);
}

/*void TransparentWidget::reparent(QWidget *parent, Qt::WFlags f, const QPoint &p, bool showIt)
{
    QWidget::reparent(parent, Qt::WNoAutoErase, p, showIt);
}*/

void TransparentWidget::setPosition(int x, int y)
{
    m_x = x;
    m_y = y;
}

void TransparentWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    //  painter.save();
    painter.translate(-m_x, -m_y);
    // m_basket->drawContents(&painter, m_x, m_y, width(), height());

    //  painter.restore();
    //  painter.setPen(Qt::blue);
    //  painter.drawRect(0, 0, width(), height());
}

void TransparentWidget::mouseMoveEvent(QMouseEvent *event)
{
    //    QMouseEvent *translated = new QMouseEvent(QEvent::MouseMove, event->pos() + QPoint(m_x, m_y), event->button(), event->buttons(), event->modifiers());
    //    m_basket->contentsMouseMoveEvent(translated);
    //    delete translated;
}

bool TransparentWidget::eventFilter(QObject * /*object*/, QEvent *event)
{
    // If the parent basket viewport has changed, we should change too:
    if (event->type() == QEvent::Paint)
        update();

    return false; // Event not consumed, in every cases (because it's only a notification)!
}
