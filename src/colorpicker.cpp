/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "colorpicker.h"

#include <QApplication>
#include <QKeyEvent>
#include <QScreen>
#include <QTimer>

/// ///

/** DektopColorPicker */

/* From Qt documentation:
 * " Note that only visible widgets can grab mouse input.
 *   If isVisible() returns FALSE for a widget, that widget cannot call grabMouse(). "
 * So, we should use an always visible widget to be able to pick a color from screen,
 * even by first  the main window (user rarely want to grab a color from BasKet!)
 * or use a global shortcut (main window can be hidden when hitting that shortcut).
 */

DesktopColorPicker::DesktopColorPicker()
    : QDesktopWidget()
{
    setObjectName("DesktopColorPicker");
    m_gettingColorFromScreen = false;
}

void DesktopColorPicker::pickColor()
{
    m_gettingColorFromScreen = true;
    //  Global::mainContainer->setActive(false);
    QTimer::singleShot(50, this, &DesktopColorPicker::slotDelayedPick);
}

/* When firered from basket context menu, and not from menu, grabMouse doesn't work!
 * It's perhaps because context menu call slotColorFromScreen() and then
 * ungrab the mouse (since menus grab the mouse).
 * But why isn't there such bug with normal menus?...
 * By calling this method with a QTimer::singleShot, we are sure context menu code is
 * finished and we can grab the mouse without loosing the grab:
 */
void DesktopColorPicker::slotDelayedPick()
{
    grabKeyboard();
    grabMouse(Qt::CrossCursor);
    setMouseTracking(true);
}

/* Validate the color
 */
void DesktopColorPicker::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_gettingColorFromScreen) {
        m_gettingColorFromScreen = false;
        releaseMouse();
        releaseKeyboard();
        setMouseTracking(false);

        // copied logic from https://code.woboq.org/qt5/qtbase/src/widgets/dialogs/qcolordialog.cpp.html
        const QPoint globalPos = QCursor::pos();
        const QDesktopWidget *desktop = QApplication::desktop();
        const QPixmap pixmap = QGuiApplication::primaryScreen()->grabWindow(desktop->winId(), globalPos.x(), globalPos.y(), 1, 1);
        QImage i = pixmap.toImage();
        Q_EMIT pickedColor(i.pixel(0, 0));
    } else {
        QDesktopWidget::mouseReleaseEvent(event);
    }
}

/* Cancel the mode
 */
void DesktopColorPicker::keyPressEvent(QKeyEvent *event)
{
    if (m_gettingColorFromScreen) {
        if (event->key() == Qt::Key_Escape) {
            m_gettingColorFromScreen = false;
            releaseMouse();
            releaseKeyboard();
            setMouseTracking(false);
            emit canceledPick();
        }
    }
    QDesktopWidget::keyPressEvent(event);
}
