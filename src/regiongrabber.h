/**
 * SPDX-FileCopyrightText: (C) 2007 Luca Gugelmann <lucag@student.ethz.ch>
 *
 * SPDX-License-Identifier: LGPL-2.0-only
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

protected Q_SLOTS:
    void init();
    void displayHelp();

Q_SIGNALS:
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
