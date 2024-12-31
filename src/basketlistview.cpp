/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketlistview.h"

#include <QApplication>
#include <QBitmap>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFocusEvent>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QToolTip>

#include <KIconLoader>
#include <KLocalizedString>
#include <KStringHandler>

#include "basketscene.h"
#include "bnpview.h"
#include "decoratedbasket.h"
#include "global.h"
#include "icon_names.h"
#include "notedrag.h"
#include "settings.h"
#include "tools.h"

/** class BasketListViewItem: */

BasketListViewItem::BasketListViewItem(QTreeWidget *parent, BasketScene *basket)
    : QTreeWidgetItem(parent)
    , m_basket(basket)
    , m_isUnderDrag(false)
    , m_isAbbreviated(false)
{
}

BasketListViewItem::BasketListViewItem(QTreeWidgetItem *parent, BasketScene *basket)
    : QTreeWidgetItem(parent)
    , m_basket(basket)
    , m_isUnderDrag(false)
    , m_isAbbreviated(false)
{
}

BasketListViewItem::BasketListViewItem(QTreeWidget *parent, QTreeWidgetItem *after, BasketScene *basket)
    : QTreeWidgetItem(parent, after)
    , m_basket(basket)
    , m_isUnderDrag(false)
    , m_isAbbreviated(false)
{
}

BasketListViewItem::BasketListViewItem(QTreeWidgetItem *parent, QTreeWidgetItem *after, BasketScene *basket)
    : QTreeWidgetItem(parent, after)
    , m_basket(basket)
    , m_isUnderDrag(false)
    , m_isAbbreviated(false)
{
}

BasketListViewItem::~BasketListViewItem()
{
}

QString BasketListViewItem::escapedName(const QString &string)
{
    // Underlining the Alt+Letter shortcut (and escape all other '&' characters), if any:
    QString basketName = string;
    basketName.replace(QLatin1Char('&'), QStringLiteral("&&")); // First escape all the amperstamp
    QString letter;
    QRegularExpression letterExp(QStringLiteral("^Alt\\+(?:Shift\\+)?(.)$"));

    QString basketShortcut = m_basket->shortcut().toString();
    if (basketShortcut.indexOf(letterExp) != -1) {
        int index;
        QRegularExpressionMatch letterMatch = letterExp.match(basketShortcut);
        letter = letterMatch.captured(1);
        if ((index = basketName.indexOf(letter)) != -1)
            basketName.insert(index, QLatin1Char('&'));
    }

    return basketName;
}

void BasketListViewItem::setup()
{
    setText(/*column=*/0, escapedName(m_basket->basketName()));

    QPixmap icon =
        KIconLoader::global()->loadIcon(m_basket->icon(), KIconLoader::NoGroup, 16, KIconLoader::DefaultState, QStringList(), nullptr, /*canReturnNull=*/false);

    setIcon(/*column=*/0, icon);
    /*
        QBrush brush;

        bool withIcon = m_stateCopy || (m_tagCopy && !m_tagCopy->isMultiState());
        State* state = (m_tagCopy ? m_tagCopy->stateCopies[0]->newState : m_stateCopy->newState);
        brush.setColor(isSelected() ? qApp->palette().color(QPalette::Highlight)  : (withIcon && state->backgroundColor().isValid() ? state->backgroundColor() :
       viewport->palette().color(viewwport->backgroundRole()))); setBackground(brush);
        */
}

BasketListViewItem *BasketListViewItem::lastChild()
{
    int count = childCount();
    if (count <= 0)
        return nullptr;
    return (BasketListViewItem *)(child(count - 1));
}

QStringList BasketListViewItem::childNamesTree(int deep)
{
    QStringList result;

    // Compute indentation spaces:
    QString spaces;
    for (int j = 0; j < deep; ++j)
        spaces += QStringLiteral("  ");

    // Append the names of sub baskets
    if (deep > 0)
        result.append(spaces + basket()->basketName());

    // Append the children:
    for (int i = 0; i < childCount(); i++) {
        QStringList children = ((BasketListViewItem *)child(i))->childNamesTree(deep + 1);
        result.append(children);
    }
    return result;
}

void BasketListViewItem::moveChildsBaskets()
{
    int insertAfterThis = 0;
    if (!parent())
        insertAfterThis = treeWidget()->indexOfTopLevelItem(this);
    for (int i = 0; i < childCount(); i++) {
        // Re-insert the item with the good parent:
        if (parent())
            parent()->insertChild(insertAfterThis, child(i));
        else
            treeWidget()->insertTopLevelItem(insertAfterThis, child(i));
        // And move it at the good place:
        insertAfterThis++;
    }
}

void BasketListViewItem::ensureVisible()
{
    BasketListViewItem *item = this;
    while (item->parent()) {
        item = (BasketListViewItem *)(item->parent());
        item->setExpanded(true);
    }
}

bool BasketListViewItem::isShown()
{
    QTreeWidgetItem *item = parent();
    while (item) {
        if (!item->isExpanded())
            return false;
        item = item->parent();
    }
    return true;
}

bool BasketListViewItem::isCurrentBasket()
{
    return basket() == Global::bnpView->currentBasket();
}

bool BasketListViewItem::isUnderDrag()
{
    return m_isUnderDrag;
}

bool BasketListViewItem::haveChildsLoading()
{
    for (int i = 0; i < childCount(); i++) {
        BasketListViewItem *childItem = (BasketListViewItem *)child(i);
        if (!childItem->basket()->isLoaded() && !childItem->basket()->isLocked())
            return true;
        if (childItem->haveChildsLoading())
            return true;
    }
    return false;
}

bool BasketListViewItem::haveHiddenChildsLoading()
{
    if (isExpanded())
        return false;
    return haveChildsLoading();
}

bool BasketListViewItem::haveChildsLocked()
{
    for (int i = 0; i < childCount(); i++) {
        BasketListViewItem *childItem = (BasketListViewItem *)child(i);
        if (/*!*/ childItem->basket()->isLocked())
            return true;
        if (childItem->haveChildsLocked())
            return true;
    }
    return false;
}

bool BasketListViewItem::haveHiddenChildsLocked()
{
    if (isExpanded())
        return false;
    return haveChildsLocked();
}

int BasketListViewItem::countChildsFound()
{
    int count = 0;
    for (int i = 0; i < childCount(); i++) {
        BasketListViewItem *childItem = (BasketListViewItem *)child(i);
        count += childItem->basket()->countFounds();
        count += childItem->countChildsFound();
    }
    return count;
}

int BasketListViewItem::countHiddenChildsFound()
{
    if (isExpanded())
        return 0;
    return countChildsFound();
}

void BasketListViewItem::setUnderDrag(bool underDrag)
{
    m_isUnderDrag = underDrag;
}

bool BasketListViewItem::isAbbreviated()
{
    return m_isAbbreviated;
}

void BasketListViewItem::setAbbreviated(bool b)
{
    m_isAbbreviated = b;
}

/** class BasketTreeListView: */
QString BasketTreeListView::TREE_ITEM_MIME_STRING = QStringLiteral("application/x-basket-item");

BasketTreeListView::BasketTreeListView(QWidget *parent)
    : QTreeWidget(parent)
    , m_autoOpenItem(nullptr)
    , m_itemUnderDrag(nullptr)
{
    connect(&m_autoOpenTimer, &QTimer::timeout, this, &BasketTreeListView::autoOpen);
    setItemDelegate(new FoundCountIcon(this));
}

void BasketTreeListView::contextMenuEvent(QContextMenuEvent *e)
{
    Q_EMIT contextMenuRequested(e->pos());
}

QStringList BasketTreeListView::mimeTypes() const
{
    QStringList types;
    types << TREE_ITEM_MIME_STRING;
    types << QString::fromUtf8(NoteDrag::NOTE_MIME_STRING);
    return types;
}

QMimeData *BasketTreeListView::mimeData(const QList<QTreeWidgetItem *> &items) const
{
    QString mimeType = TREE_ITEM_MIME_STRING;

    QByteArray data = QByteArray();
    QDataStream out(&data, QIODevice::WriteOnly);

    if (items.isEmpty())
        return new QMimeData();

    for (int i = 0; i < items.count(); ++i) {
        BasketListViewItem *basketItem = static_cast<BasketListViewItem *>(items[i]);
        out << basketItem->basket()->basketName() << basketItem->basket()->folderName() << basketItem->basket()->icon();
    }

    QMimeData *mimeData = new QMimeData();

    mimeData->setData(mimeType, data);
    return mimeData;
}

bool BasketTreeListView::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        QHelpEvent *he = static_cast<QHelpEvent *>(e);
        QTreeWidgetItem *item = itemAt(he->pos());
        BasketListViewItem *bitem = dynamic_cast<BasketListViewItem *>(item);
        if (bitem && bitem->isAbbreviated()) {
            QRect rect = visualItemRect(bitem);
            QToolTip::showText(rect.topLeft(), bitem->basket()->basketName(), viewport(), rect);
        }
        return true;
    }
    return QTreeWidget::event(e);
}

void BasketTreeListView::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    QTreeWidget::mousePressEvent(event);
}

void BasketTreeListView::mouseMoveEvent(QMouseEvent *event)
{
    // QTreeWidget::mouseMoveEvent(event);
    if (!(event->buttons() & Qt::LeftButton)) {
        event->ignore();
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        event->ignore();
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = this->mimeData(this->selectedItems());
    drag->setMimeData(mimeData);

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
    if (dropAction == Qt::MoveAction || dropAction == Qt::CopyAction)
        event->accept();
}

void BasketTreeListView::dragEnterEvent(QDragEnterEvent *event)
{
    // TODO: accept everything? (forwarding dropped basket-notes and arbitrary data to the basket)
    // files: MoveAction vs. CopyAction, or acceptProposedAction()

    QTreeWidget::dragEnterEvent(event);
}

void BasketTreeListView::removeExpands()
{
    QTreeWidgetItemIterator it(this);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->childCount() <= 0)
            item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        ++it;
    }
}

void BasketTreeListView::dragLeaveEvent(QDragLeaveEvent *event)
{
    qDebug() << "BasketTreeListView::dragLeaveEvent";
    m_autoOpenItem = nullptr;
    m_autoOpenTimer.stop();
    setItemUnderDrag(nullptr);
    removeExpands();
    QTreeWidget::dragLeaveEvent(event);
}

void BasketTreeListView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(TREE_ITEM_MIME_STRING)) {
        event->setDropAction(Qt::MoveAction);
        QTreeWidget::dropEvent(event);
    } else { // this handels application/x-basket-note drag events.
        qDebug() << "Forwarding dropped data to the basket";
        event->setDropAction(Qt::MoveAction);
        QTreeWidgetItem *item = itemAt(event->pos());
        BasketListViewItem *bitem = dynamic_cast<BasketListViewItem *>(item);
        if (bitem) {
            bitem->basket()->blindDrop(event->mimeData(), event->dropAction(), event->source());
        } else {
            qDebug() << "Forwarding failed: no bitem found";
        }
    }

    m_autoOpenItem = nullptr;
    m_autoOpenTimer.stop();
    setItemUnderDrag(nullptr);
    removeExpands();

    Global::bnpView->save(); // TODO: Don't save if it was not a basket drop...
}

void BasketTreeListView::dragMoveEvent(QDragMoveEvent *event)
{
    // qDebug() << "BasketTreeListView::dragMoveEvent";

    if (!event->mimeData()->hasFormat(TREE_ITEM_MIME_STRING)) {
        QTreeWidgetItem *item = itemAt(event->pos());
        BasketListViewItem *bitem = dynamic_cast<BasketListViewItem *>(item);
        if (m_autoOpenItem != item) {
            m_autoOpenItem = item;
            m_autoOpenTimer.setSingleShot(true);
            m_autoOpenTimer.start(1700);
        }

        if (item) {
            event->accept();
        }
        setItemUnderDrag(bitem);
    }

    QTreeWidget::dragMoveEvent(event);
}

void BasketTreeListView::setItemUnderDrag(BasketListViewItem *item)
{
    if (m_itemUnderDrag != item) {
        if (m_itemUnderDrag) {
            // Remove drag status from the old item
            m_itemUnderDrag->setUnderDrag(false);
        }

        m_itemUnderDrag = item;

        if (m_itemUnderDrag) {
            // add drag status to the new item
            m_itemUnderDrag->setUnderDrag(true);
        }
    }
}

void BasketTreeListView::autoOpen()
{
    BasketListViewItem *item = (BasketListViewItem *)m_autoOpenItem;
    if (item)
        Global::bnpView->setCurrentBasket(item->basket());
}

void BasketTreeListView::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);
}

/** We should NEVER get focus (because of QWidget::NoFocus focusPolicy())
 * but QTreeView can programatically give us the focus.
 * So we give it to the basket.
 */
void BasketTreeListView::focusInEvent(QFocusEvent *)
{
    BasketScene *basket = Global::bnpView->currentBasket();
    if (basket)
        basket->setFocus();
}

Qt::DropActions BasketTreeListView::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

BasketListViewItem *BasketTreeListView::getBasketInTree(const QModelIndex &index) const
{
    QTreeWidgetItem *item = itemFromIndex(index);
    return dynamic_cast<BasketListViewItem *>(item);
}

void FoundCountIcon::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    // Get access to basket pointer
    BasketListViewItem *basketInTree = m_basketTree->getBasketInTree(index);
    if (basketInTree == nullptr)
        return;

    const int BASKET_ICON_SIZE = 16; // [replace with m_basketTree->iconSize()]
    const int MARGIN = 1;

    BasketScene *basket = basketInTree->basket();

    // If we are filtering all baskets, and are effectively filtering on something:
    bool showLoadingIcon = false;
    bool showEncryptedIcon = false;
    QPixmap countPixmap;
    bool showCountPixmap = Global::bnpView->isFilteringAllBaskets() && Global::bnpView->currentBasket()->decoration()->filterBar()->filterData().isFiltering;
    if (showCountPixmap) {
        showLoadingIcon = (!basket->isLoaded() && !basket->isLocked()) || basketInTree->haveHiddenChildsLoading();
        showEncryptedIcon = basket->isLocked() || basketInTree->haveHiddenChildsLocked();
        bool childrenAreLoading = basketInTree->haveHiddenChildsLoading() || basketInTree->haveHiddenChildsLocked();

        countPixmap = foundCountPixmap(!basket->isLoaded(),
                                       basket->countFounds(),
                                       childrenAreLoading,
                                       basketInTree->countHiddenChildsFound(),
                                       m_basketTree->font(),
                                       option.rect.height() - 2 * MARGIN);
    }
    int effectiveWidth = option.rect.right() - (countPixmap.isNull() ? 0 : countPixmap.width() + MARGIN)
        - (showLoadingIcon || showEncryptedIcon ? BASKET_ICON_SIZE + MARGIN : 0);

    bool drawRoundRect = basket->backgroundColorSetting().isValid() || basket->textColorSetting().isValid();

    // Draw the rounded rectangle:
    if (drawRoundRect) {
        QPixmap roundRectBmp;
        QColor background = basket->backgroundColor();
        int textWidth = m_basketTree->fontMetrics().horizontalAdvance(basketInTree->text(/*column=*/0));
        int iconTextMargin = m_basketTree->style()->pixelMetric(QStyle::PM_FocusFrameHMargin); ///< Space between icon and text

        // Don't forget to update the key computation if parameters
        // affecting the rendering logic change
        QString key =
            QString(QStringLiteral("BLIRR::%1.%2.%3.%4")).arg(option.rect.width()).arg(option.rect.size().height()).arg(textWidth).arg(background.rgb());
        if (!QPixmapCache::find(key, &roundRectBmp)) {
            // Draw first time

            roundRectBmp = QPixmap(option.rect.size());
            roundRectBmp.fill(Qt::transparent);

            QPainter brushPainter(&roundRectBmp);

            int cornerR = option.rect.height() / 2 - MARGIN;

            QRect roundRect(0, MARGIN, BASKET_ICON_SIZE + iconTextMargin + textWidth + 2 * cornerR, option.rect.height() - 2 * MARGIN);

            brushPainter.setPen(background);
            brushPainter.setBrush(background);
            brushPainter.setRenderHint(QPainter::Antialiasing);
            brushPainter.drawRoundedRect(roundRect, cornerR, cornerR);

            QPixmapCache::insert(key, roundRectBmp);
        }

        basketInTree->setBackground(0, QBrush(roundRectBmp));
        basketInTree->setForeground(0, QBrush(basket->textColor()));
    }
    // end if drawRoundRect

    // Render icons on the right
    int y = option.rect.center().y() - BASKET_ICON_SIZE / 2;

    if (!countPixmap.isNull()) {
        painter->drawPixmap(effectiveWidth, y, countPixmap);
        effectiveWidth += countPixmap.width() + MARGIN;
    }
    if (showLoadingIcon) {
        QPixmap icon = KIconLoader::global()->loadIcon(QString::fromUtf8(IconNames::LOADING), KIconLoader::NoGroup, BASKET_ICON_SIZE);
        painter->drawPixmap(effectiveWidth, y, icon);
        effectiveWidth += BASKET_ICON_SIZE + MARGIN;
    }
    if (showEncryptedIcon && !showLoadingIcon) {
        QPixmap icon = KIconLoader::global()->loadIcon(QString::fromUtf8(IconNames::LOCKED), KIconLoader::NoGroup, BASKET_ICON_SIZE);
        painter->drawPixmap(effectiveWidth, y, icon);
    }
}

QPixmap FoundCountIcon::circledTextPixmap(const QString &text, int height, const QFont &font, const QColor &color) const
{
    QString key = QString(QStringLiteral("BLI-%1.%2.%3.%4")).arg(text).arg(height).arg(font.toString()).arg(color.rgb());
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }

    // Compute the sizes of the image components:
    QRectF textRect = QFontMetrics(font).boundingRect(0, 0, /*width=*/1, height, Qt::AlignLeft | Qt::AlignTop, text);
    qreal xMargin = height / 6;
    qreal width = xMargin + textRect.width() + xMargin;

    // Create the background image:
    QPixmap background(3 * width, 3 * height); // We double the size to be able to smooth scale down it (== antialiased curves)
    QPainter backgroundPainter(&background);
    const QPalette &palette = m_basketTree->palette();
    backgroundPainter.fillRect(0, 0, background.width(), background.height(), palette.color(QPalette::Highlight));
    backgroundPainter.end();

    // Draw the curved rectangle:
    QBitmap curvedRectangle(3 * width, 3 * height);
    curvedRectangle.fill(Qt::color0);
    QPainter curvePainter(&curvedRectangle);
    curvePainter.setPen(Qt::color1);
    curvePainter.setBrush(Qt::color1);
    curvePainter.setClipRect(0, 0, 3 * (height / 5), 3 * (height)); // If the width is small, don't fill the right part of the pixmap
    curvePainter.drawEllipse(0, 3 * (-height / 4), 3 * (height), 3 * (height * 3 / 2)); // Don't forget we double the sizes
    curvePainter.setClipRect(3 * (width - height / 5), 0, 3 * (height / 5), 3 * (height));
    curvePainter.drawEllipse(3 * (width - height), 3 * (-height / 4), 3 * (height), 3 * (height * 3 / 2));
    curvePainter.setClipping(false);
    curvePainter.fillRect(3 * (height / 6), 0, 3 * (width - 2 * height / 6), 3 * (height), curvePainter.brush());
    curvePainter.end();

    // Apply the curved rectangle as the mask of the background:
    background.setMask(curvedRectangle);
    QImage resultImage = background.toImage();

    // resultImage.setAlphaBuffer(true);
    // resultImage.convertToFormat(QImage::Format_ARGB32);

    // Scale down the image smoothly to get anti-aliasing:
    QPixmap pmScaled = QPixmap::fromImage(resultImage.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    // Draw the text, and return the result:
    QPainter painter(&pmScaled);
    painter.setPen(color);
    painter.setFont(font);
    painter.drawText(0 + 1, 0, width, height, Qt::AlignHCenter | Qt::AlignVCenter, text);
    painter.end();

    QPixmapCache::insert(key, pmScaled);

    return pmScaled;
}

QPixmap FoundCountIcon::foundCountPixmap(bool isLoading, int countFound, bool childrenAreLoading, int countChildsFound, const QFont &font, int height) const
{
    if (isLoading)
        return QPixmap();

    QFont boldFont(font);
    boldFont.setBold(true);

    QString text;
    if (childrenAreLoading) {
        if (countChildsFound > 0)
            text = i18n("%1+%2+", QString::number(countFound), QString::number(countChildsFound));
        else
            text = i18n("%1+", QString::number(countFound));
    } else {
        if (countChildsFound > 0)
            text = i18n("%1+%2", QString::number(countFound), QString::number(countChildsFound));
        else if (countFound > 0)
            text = QString::number(countFound);
        else
            return QPixmap();
    }

    return circledTextPixmap(text, height, boldFont, m_basketTree->palette().color(QPalette::HighlightedText));
}

#include "moc_basketlistview.cpp"
