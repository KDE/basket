/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TRANSPARENTWIDGET_H
#define TRANSPARENTWIDGET_H

#include <QWidget>

class BasketScene;
class QPaintEvent;
class QMouseEvent;
class QObject;

class TransparentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TransparentWidget(BasketScene *basket);
    void setPosition(int x, int y);
    // void reparent(QWidget *parent, Qt::WFlags f, const QPoint &p, bool showIt = FALSE);
protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    BasketScene *m_basket;
    int m_x;
    int m_y;
};

#endif // TRANSPARENTWIDGET_H
