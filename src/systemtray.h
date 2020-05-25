/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <KStatusNotifierItem>
#include <QIcon>
#include <QSize>

/** A thin wrapper around KSystemTrayIcon until the old SystemTray is ported.
 * As things are ported, items should
 * @author Kelvie Wong
 */
class SystemTray : public KStatusNotifierItem
{
    Q_OBJECT
    Q_DISABLE_COPY(SystemTray);

public:
    explicit SystemTray(QWidget *parent = nullptr);
    ~SystemTray() override;

public Q_SLOTS:
    void updateDisplay();

Q_SIGNALS:
    void showPart();
};

#ifdef USE_OLD_SYSTRAY

/** This class provide a personalized system tray icon.
 * @author Sébastien Laoût
 */
class SystemTray2 : public SystemTray
{
    Q_OBJECT
public:
    explicit SystemTray2(QWidget *parent = nullptr, const char *name = nullptr);
    ~SystemTray2();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *);
    virtual void dropEvent(QDropEvent *event);
    void wheelEvent(QWheelEvent *event);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

private:
    QTimer *m_showTimer;
    QTimer *m_autoShowTimer;
    bool m_canDrag;
    QPoint m_pressPos;
};
#endif // USE_OLD_SYSTRAY

#endif // SYSTEMTRAY_H
