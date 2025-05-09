/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKETLISTVIEW_H
#define BASKETLISTVIEW_H

#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeWidget>

class QPixmap;
class QResizeEvent;
class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
class QFocusEvent;
class QDragLeaveEvent;

class BasketScene;

class BasketListViewItem : public QTreeWidgetItem
{
public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    BasketListViewItem(QTreeWidget *parent, BasketScene *basket);
    BasketListViewItem(QTreeWidgetItem *parent, BasketScene *basket);
    BasketListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, BasketScene *basket);
    BasketListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, BasketScene *basket);
    ~BasketListViewItem() override;

    BasketScene *basket()
    {
        return m_basket;
    }
    void setup();
    BasketListViewItem *lastChild();
    QStringList childNamesTree(int deep = 0);
    void moveChildsBaskets();
    void ensureVisible();
    bool isShown();
    bool isCurrentBasket();
    bool isUnderDrag();
    QString escapedName(const QString &string);

    bool haveChildsLoading();
    bool haveHiddenChildsLoading();
    bool haveChildsLocked();
    bool haveHiddenChildsLocked();
    int countChildsFound();
    int countHiddenChildsFound();

    void setUnderDrag(bool);
    bool isAbbreviated();
    void setAbbreviated(bool b);

private:
    BasketScene *m_basket;
    int m_width;
    bool m_isUnderDrag;
    bool m_isAbbreviated;
};

Q_DECLARE_METATYPE(BasketListViewItem *);

class BasketTreeListView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit BasketTreeListView(QWidget *parent = nullptr);
    void dragEnterEvent(QDragEnterEvent *event) override;
    void removeExpands();
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    Qt::DropActions supportedDropActions() const override;

    /*! Retrieve a basket from the tree
     *  @see BasketListViewItem::basket() */
    BasketListViewItem *getBasketInTree(const QModelIndex &index) const;

    static QString TREE_ITEM_MIME_STRING;

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override;
    bool event(QEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *) override;

private:
    QTimer m_autoOpenTimer;
    QTreeWidgetItem *m_autoOpenItem;
Q_SIGNALS:
    void contextMenuRequested(const QPoint &);
private Q_SLOTS:
    void autoOpen();

private:
    void setItemUnderDrag(BasketListViewItem *item);
    BasketListViewItem *m_itemUnderDrag;
    QPoint m_dragStartPosition;
};

/** @class FoundCountIcon
 *  Custom-drawn "little numbers" shown on Filter all */
class FoundCountIcon : public QStyledItemDelegate
{
public:
    FoundCountIcon(BasketTreeListView *basketTree, QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_basketTree(basketTree)
    {
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QPixmap circledTextPixmap(const QString &text, int height, const QFont &font, const QColor &color) const;

    //! @returns Rect-with-number, or null pixmap if nothing was found / basket is loading
    QPixmap foundCountPixmap(bool isLoading, int countFound, bool childrenAreLoading, int countChildsFound, const QFont &font, int height) const;

    BasketTreeListView *m_basketTree;
};

#endif // BASKETLISTVIEW_H
