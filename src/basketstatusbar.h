/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef BASKETSTATUSBAR_H
#define BASKETSTATUSBAR_H

#include <QtCore/QObject>
#include <QtGui/QPixmap>

#include "basket_export.h"

class QStatusBar;
namespace KParts
{
class StatusBarExtension;
}

class QWidget;
class QLabel;

/**
    @author Sébastien Laoût <slaout@linux62.org>
*/
class BASKET_EXPORT BasketStatusBar : public QObject
{
    Q_OBJECT
public:
    explicit BasketStatusBar(QStatusBar *bar);
    BasketStatusBar(KParts::StatusBarExtension *extension);
    ~BasketStatusBar() override;

public Q_SLOTS:
    /** GUI Main Window actions **/
    void setStatusBarHint(const QString &hint); /// << Set a specific message or update if hint is empty
    void updateStatusBarHint();                 /// << Display the current state message (dragging, editing) or reset the startsbar message
    void postStatusbarMessage(const QString &text);
    void setSelectionStatus(const QString &s);
    void setLockStatus(bool isLocked);
    void setupStatusBar();
    void setUnsavedStatus(bool isUnsaved);

protected:
    QStatusBar *statusBar() const;
    void addWidget(QWidget *widget, int stretch = 0, bool permanent = false);
    void setStatusText(const QString &txt);
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QStatusBar *m_bar;
    KParts::StatusBarExtension *m_extension;
    QLabel *m_selectionStatus;
    QLabel *m_lockStatus;
    QLabel *m_basketStatus;
    QLabel *m_savedStatus;
    QPixmap m_savedStatusPixmap;
};

#endif
