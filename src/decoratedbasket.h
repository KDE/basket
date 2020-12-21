/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DECORATEDBASKET_H
#define DECORATEDBASKET_H

class QString;
class BasketScene;
class QVBoxLayout;
class QGraphicsView;

#include <QWidget>

#include "filter.h"

class KMessageWidget;

/** This class handle Basket and add a FilterWidget on top of it.
 * @author Sébastien Laoût
 */
class DecoratedBasket : public QWidget
{
    Q_OBJECT
public:
    DecoratedBasket(QWidget *parent, const QString &folderName);
    ~DecoratedBasket() override  = default;
    void setFilterBarPosition(bool onTop);
    void resetFilter();
    void setFilterBarVisible(bool show, bool switchFocus = true);
    bool isFilterBarVisible()
    {
        return m_filter->isVisible();
    }
    const FilterData &filterData()
    {
        return m_filter->filterData();
    }
    FilterBar *filterBar()
    {
        return m_filter;
    }
    BasketScene *basket()
    {
        return m_basket;
    }

    void resizeEvent(QResizeEvent *event) override;

public Q_SLOTS:
    void showErrorMessage(const QString &errorMessage);

private:
    QVBoxLayout *m_layout;
    FilterBar *m_filter;

    KMessageWidget *m_messageWidget = nullptr;
    BasketScene *m_basket;
};
#endif // DECORATEDBASKET_H
