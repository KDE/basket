/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FOCUSEDWIDGETS_H
#define FOCUSEDWIDGETS_H

#include <KTextEdit>
#include <QClipboard>

class QEvent;
class QKeyEvent;
class QWheelEvent;

class QMenu;

class FocusedTextEdit : public KTextEdit
{
    Q_OBJECT
public:
    explicit FocusedTextEdit(bool disableUpdatesOnKeyPress, QWidget *parent = nullptr);
    ~FocusedTextEdit() override;
    void paste(QClipboard::Mode mode);
public Q_SLOTS:
    void onSelectionChanged(); //!< Put selected text into the global mouse selection
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void enterEvent(QEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;
Q_SIGNALS:
    void escapePressed();
    void mouseEntered();

private:
    bool m_disableUpdatesOnKeyPress;
};

/** class FocusWidgetFilter
 * @author Kelvie Wong
 *
 * A very simple event filter that returns when escape and return are pressed,
 * and as well, to emit a signal for the mouse event.
 *
 * This allows us to create our own focus model with widgets inside baskets
 * (although I'm not sure how useful this will all be after we port Basket to be
 * use QGraphicsView).
 *
 * Keypresses are filtered (i.e. the widget will not get the key press events),
 * but the enterEvent is not (for backwards compatibility).
 */
class FocusWidgetFilter : public QObject
{
    Q_OBJECT
public:
    /** Constructor
     * @param watched The widget to install the event filter on; also becomes
     * the parent of this object. */
    explicit FocusWidgetFilter(QWidget *watched = nullptr);
    ~FocusWidgetFilter() override
    {
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

Q_SIGNALS:
    void escapePressed();
    void returnPressed();
    void mouseEntered();
};

#endif // FOCUSEDWIDGETS_H
