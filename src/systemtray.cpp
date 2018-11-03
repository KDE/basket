/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** SystemTray */
#include "systemtray.h"

// Qt
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QLocale>
#include <QDialog>

// Frameworks
#include <KIconEffect>  //Port to Blitz/Quasar?
#include <KLocalizedString>

// Local
#include "basketscene.h"
#include "settings.h"
#include "global.h"
#include "tools.h"
#include "icon_names.h"


/* This function comes directly from JuK: */

/*
 * This function copies the entirety of src into dest, starting in
 * dest at x and y.  This function exists because I was unable to find
 * a function like it in either QImage or kdefx
 */
static bool copyImage(QImage &dest, QImage &src, int x, int y)
{
    if (dest.depth() != src.depth())
        return false;
    if ((x + src.width()) >= dest.width())
        return false;
    if ((y + src.height()) >= dest.height())
        return false;

    // We want to use KIconEffect::overlay to do this, since it handles
    // alpha, but the images need to be the same size.  We can handle that.

    QImage large_src(dest);

    // It would perhaps be better to create large_src based on a size, but
    // this is the easiest way to make a new image with the same depth, size,
    // etc.

    large_src.detach();

    // However, we do have to specifically ensure that setAlphaBuffer is set
    // to false

    //large_src.setAlphaBuffer(false);
    large_src.fill(0); // All transparent pixels
    //large_src.setAlphaBuffer(true);

    int w = src.width();
    int h = src.height();
    for (int dx = 0; dx < w; dx++)
        for (int dy = 0; dy < h; dy++)
            large_src.setPixel(dx + x, dy + y, src.pixel(dx, dy));

    // Apply effect to image

    KIconEffect::overlay(dest, large_src);

    return true;
}


/** Constructor */
SystemTray::SystemTray(QWidget *parent)
        : KStatusNotifierItem(parent)
{
    updateDisplay();
}

/** Destructor */
SystemTray::~SystemTray()
{
    // pass
}

/** Updates the icon and tooltip in the system tray */
void SystemTray::updateDisplay()
{
    BasketScene *basket = Global::bnpView->currentBasket();
    if (!basket)
        return;

    // Update the icon
    if (basket->icon().isEmpty()
            || basket->icon() == "basket"
            || !Settings::showIconInSystray())
        setIconByName("basket");
    else {
        // What pixmap size to use? For example, see how overlay icon is applied:
        // http://api.kde.org/frameworks-api/frameworks5-apidocs/knotifications/html/kstatusnotifieritem_8cpp_source.html

        // Code that comes from JuK:
        QPixmap bgPix = QIcon::fromTheme("basket").pixmap(22);
        QPixmap fgPix = QIcon::fromTheme(basket->icon()).pixmap(16);

        QImage bgImage = bgPix.toImage(); // Probably 22x22
        QImage fgImage = fgPix.toImage(); // Should be 16x16

        KIconEffect::semiTransparent(bgImage);
        copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
                  (bgImage.height() - fgImage.height()) / 2);

        setIconByPixmap(QPixmap::fromImage(bgImage));
    }
    setOverlayIconByName(basket->isLocked() ? IconNames::LOCKED : "");

    // update the tooltip
    QString tip = "<p><nobr>";
    QString basketName = "%1";
    if (basket->isLocked())
        basketName += i18n(" (Locked)");
    tip += Tools::makeStandardCaption(basketName);
    tip = tip.arg(Tools::textToHTMLWithoutP(basket->basketName()));
    setToolTipTitle(tip);
}

#ifdef USE_OLD_SYSTRAY
#define QT3_SUPPORT // No need to port that old stuff
SystemTray::SystemTray(QWidget *parent, const char *name)
        : KSystemTray2(parent, name != 0 ? name : "SystemTray"), m_showTimer(0), m_autoShowTimer(0)
{
    setAcceptDrops(true);

    m_showTimer = new QTimer(this);
    connect(m_showTimer, SIGNAL(timeout()), Global::bnpView, SLOT(setActive()));

    m_autoShowTimer = new QTimer(this);
    connect(m_autoShowTimer, SIGNAL(timeout()), Global::bnpView, SLOT(setActive()));

    // Create pixmaps for the icon:
    m_iconPixmap              = loadIcon("basket");
//  FIXME: When main window is shown at start, the icon is loaded 1 pixel too high
//         and then reloaded instantly after at the right position.
//  setPixmap(m_iconPixmap); // Load it the sooner as possible to avoid flicker
    QImage  lockedIconImage   = m_iconPixmap.convertToImage();
    QPixmap lockOverlayPixmap = loadIcon("object-locked");
    QImage  lockOverlayImage  = lockOverlayPixmap.convertToImage();
    KIconEffect::overlay(lockedIconImage, lockOverlayImage);
    m_lockedIconPixmap.convertFromImage(lockedIconImage);

    updateToolTip(); // Set toolTip AND icon
}

SystemTray::~SystemTray()
{
}

void SystemTray::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {          // Prepare drag
        m_pressPos = event->globalPos();
        m_canDrag  = true;
        event->accept();
    } else if (event->button() & Qt::MidButton) {    // Paste
        Global::bnpView->currentBasket()->setInsertPopupMenu();
        Global::bnpView->currentBasket()->pasteNote(QClipboard::Selection);
        Global::bnpView->currentBasket()->cancelInsertPopupMenu();
        if (Settings::usePassivePopup())
            Global::bnpView->showPassiveDropped(i18n("Pasted selection to basket <i>%1</i>"));
        event->accept();
    } else if (event->button() & Qt::RightButton) { // Popup menu
        QMenu menu(this);
        menu.addSection(SmallIcon("basket"), QGuiApplication::applicationDisplayName());

        Global::bnpView->actNewBasket->plug(&menu);
        Global::bnpView->actNewSubBasket->plug(&menu);
        Global::bnpView->actNewSiblingBasket->plug(&menu);
        menu.insertSeparator();
        Global::bnpView->m_actPaste->plug(&menu);
        Global::bnpView->m_actGrabScreenshot->plug(&menu);
        Global::bnpView->m_actColorPicker->plug(&menu);

        if (!Global::bnpView->isPart()) {
            QAction * action;

            menu.insertSeparator();

            action = Global::bnpView->actionCollection()->action("options_configure_global_keybinding");
            if (action)
                action->plug(&menu);

            action = Global::bnpView->actionCollection()->action("options_configure");
            if (action)
                action->plug(&menu);

            menu.insertSeparator();

            // Minimize / restore : since we manage the popup menu by ourself, we should do that work :
            action = Global::bnpView->actionCollection()->action("minimizeRestore");
            if (action) {
                if (Global::mainWindow()->isVisible())
                    action->setText(i18n("&Minimize"));
                else
                    action->setText(i18n("&Restore"));
                action->plug(&menu);
            }

            action = Global::bnpView->actionCollection()->action("file_quit");
            if (action)
                action->plug(&menu);
        }

        Global::bnpView->currentBasket()->setInsertPopupMenu();
        connect(&menu, SIGNAL(aboutToHide()), Global::bnpView->currentBasket(), SLOT(delayedCancelInsertPopupMenu()));
        menu.exec(event->globalPos());
        event->accept();
    } else
        event->ignore();
}

void SystemTray::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void SystemTray::mouseReleaseEvent(QMouseEvent *event)
{
    m_canDrag = false;
    if (event->button() == Qt::LeftButton)         // Show / hide main window
        if (rect().contains(event->pos())) {       // Accept only if released in systemTray
            toggleActive();
            emit showPart();
            event->accept();
        } else
            event->ignore();
}

void SystemTray::dragEnterEvent(QDragEnterEvent *event)
{
    m_showTimer->start(Settings::dropTimeToShow() * 100, true);
    Global::bnpView->currentBasket()->showFrameInsertTo();
/// m_parentContainer->setStatusBarDrag(); // FIXME: move this line in BasketScene::showFrameInsertTo() ?
    BasketScene::acceptDropEvent(event);
}

void SystemTray::dragMoveEvent(QDragMoveEvent *event)
{
    BasketScene::acceptDropEvent(event);
}

void SystemTray::dragLeaveEvent(QDragLeaveEvent*)
{
    m_showTimer->stop();
    m_canDrag = false;
    Global::bnpView->currentBasket()->resetInsertTo();
    Global::bnpView->updateStatusBarHint();
}

#include <QX11Info>

void SystemTray::dropEvent(QDropEvent *event)
{
    m_showTimer->stop();

    Global::bnpView->currentBasket()->blindDrop(event);

    /*  BasketScene *basket = Global::bnpView->currentBasket();
        if (!basket->isLoaded()) {
            Global::bnpView->showPassiveLoading(basket);
            basket->load();
        }
        basket->contentsDropEvent(event);
        qDebug() << (long) basket->selectedNotes();

        if (Settings::usePassivePopup())
            Global::bnpView->showPassiveDropped(i18n("Dropped to basket <i>%1</i>", m_basketName));*/
}

void SystemTray::updateToolTip()
{
//  return; /////////////////////////////////////////////////////

    BasketScene *basket = Global::bnpView->currentBasket();
    if (!basket)
        return;

    if (basket->icon().isEmpty() || basket->icon() == "basket" || ! Settings::showIconInSystray())
        setPixmap(basket->isLocked() ? m_lockedIconPixmap : m_iconPixmap);
    else {
        // Code that comes from JuK:
        QPixmap bgPix = loadIcon("basket");
        QPixmap fgPix = SmallIcon(basket->icon());

        QImage bgImage = bgPix.convertToImage(); // Probably 22x22
        QImage fgImage = fgPix.convertToImage(); // Should be 16x16
        QImage lockOverlayImage = loadIcon("object-locked").convertToImage();

        KIconEffect::semiTransparent(bgImage);
        copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
                  (bgImage.height() - fgImage.height()) / 2);
        if (basket->isLocked())
            KIconEffect::overlay(bgImage, lockOverlayImage);

        bgPix.convertFromImage(bgImage);
        setPixmap(bgPix);
    }

    //QTimer::singleShot( Container::c_delayTooltipTime, this, SLOT(updateToolTipDelayed()) );
    // No need to delay: it's be called when notes are changed:
    updateToolTipDelayed();
}

void SystemTray::updateToolTipDelayed()
{
    BasketScene *basket = Global::bnpView->currentBasket();

    QString tip = "<p><nobr>" + (basket->isLocked() ? Tools::makeStandardCaption(i18n("%1 (Locked)"))
                                 : Tools::makeStandardCaption("%1"))
                  .arg(Tools::textToHTMLWithoutP(basket->basketName()));

    QToolTip::add(this, tip);
}

void SystemTray::wheelEvent(QWheelEvent *event)
{
    if (event->delta() > 0)
        Global::bnpView->goToPreviousBasket();
    else
        Global::bnpView->goToNextBasket();

    if (Settings::usePassivePopup())
        Global::bnpView->showPassiveContent();
}

void SystemTray::enterEvent(QEvent*)
{
    if (Settings::showOnMouseIn())
        m_autoShowTimer->start(Settings::timeToShowOnMouseIn() * 100, true);
}

void SystemTray::leaveEvent(QEvent*)
{
    m_autoShowTimer->stop();
}

#undef QT3_SUPPORT
#endif // USE_OLD_SYSTRAY
