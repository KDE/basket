/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QDesktopWidget>

class QKeyEvent;
class QMouseEvent;

/** Class to pick a color on the screen
 * @author Sébastien Laoût
 */
class DesktopColorPicker : public QDesktopWidget
{
    Q_OBJECT
public:
    /** Constructor, initializer and destructor */
    DesktopColorPicker();
    ~DesktopColorPicker() override;
public slots:
    /** Begin color picking.
     * This function returns immediately, and pickedColor() is emitted if user has
     * chosen a color, and not canceled the process (by pressing Escape).
     */
    void pickColor();
signals:
    /** When user picked a color, this signal is emitted.
     */
    void pickedColor(const QColor &color);
    /** When user cancel a picking (by pressing Escape), this signal is emitted.
     */
    void canceledPick();
protected slots:
    void slotDelayedPick();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool m_gettingColorFromScreen;
};

#endif // COLORPICKER_H
