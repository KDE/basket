/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKET_VIEW_H
#define BASKET_VIEW_H

#include <QGraphicsView>

class BasketScene;

class BasketView : public QGraphicsView
{
    Q_OBJECT
public:
    BasketView(BasketScene *scene, QWidget *parent = nullptr);
    ~BasketView() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // BASKET_VIEW_H
