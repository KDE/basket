/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "colorpicker.h"

#include <QApplication>
#include <QColorDialog>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

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

DesktopColorPicker::~DesktopColorPicker()
{
}

void DesktopColorPicker::pickColor()
{
    m_gettingColorFromScreen = true;
    //  Global::mainContainer->setActive(false);
    QTimer::singleShot(50, this, SLOT(slotDelayedPick()));
}

/* When firered from basket context menu, and not from menu, grabMouse doesn't work!
 * It's perhapse because context menu call slotColorFromScreen() and then
 * ungrab the mouse (since menus grab the mouse).
 * But why isn't there such bug with normal menus?...
 * By calling this method with a QTimer::singleShot, we are sure context menu code is
 * finished and we can grab the mouse without loosing the grab:
 */
void DesktopColorPicker::slotDelayedPick()
{
    grabKeyboard();
    grabMouse(Qt::CrossCursor);
}

/* Validate the color
 */
void DesktopColorPicker::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_gettingColorFromScreen) {
        m_gettingColorFromScreen = false;
        releaseMouse();
        releaseKeyboard();

        // Grab color of pixel
        QPoint p = event->globalPos();
        QPixmap pixmap = QPixmap::grabWindow(QApplication::desktop()->winId(), p.x(), p.y(), 1, 1);
        QColor color(pixmap.toImage().pixel(0, 0));

        emit pickedColor(color);
    } else
        QDesktopWidget::mouseReleaseEvent(event);
}

/* Cancel the mode
 */
void DesktopColorPicker::keyPressEvent(QKeyEvent *event)
{
    if (m_gettingColorFromScreen)
        if (event->key() == Qt::Key_Escape) {
            m_gettingColorFromScreen = false;
            releaseMouse();
            releaseKeyboard();
            emit canceledPick();
        }
    QDesktopWidget::keyPressEvent(event);
}
