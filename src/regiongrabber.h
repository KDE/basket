/*
 *   Copyright (C) 2007 Luca Gugelmann <lucag@student.ethz.ch>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef REGIONGRABBER_H
#define REGIONGRABBER_H

#include <QWidget>
#include <QtCore/QTimer>
#include <QtCore/QVector>

class QPoint;
class QRect;
class QRegion;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;

class RegionGrabber : public QWidget
{
    Q_OBJECT
public:
    RegionGrabber();
    ~RegionGrabber() override;

protected slots:
    void init();
    void displayHelp();

signals:
    void regionGrabbed(const QPixmap &);

protected:
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *e) override;
    void updateHandles();
    QRegion handleMask() const;
    QPoint limitPointToRect(const QPoint &p, const QRect &r) const;
    void grabRect();

    QRect selection;
    bool mouseDown;
    bool newSelection;
    const int handleSize;
    QRect *mouseOverHandle;
    QPoint dragStartPoint;
    QRect selectionBeforeDrag;
    QTimer idleTimer;
    bool showHelp;
    bool grabbing;

    // naming convention for handles
    // T top, B bottom, R Right, L left
    // 2 letters: a corner
    // 1 letter: the handle on the middle of the corresponding side
    QRect TLHandle, TRHandle, BLHandle, BRHandle;
    QRect LHandle, THandle, RHandle, BHandle;

    QVector<QRect *> handles;
    QPixmap pixmap;
};

#endif
