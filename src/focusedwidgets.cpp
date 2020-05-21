/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "focusedwidgets.h"

#include <QApplication>
#include <QMimeData>
#include <QScrollBar>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>

#include "basketscene.h"
#include "bnpview.h"
#include "global.h"
#include "settings.h"

#ifdef KeyPress
#undef KeyPress
#endif

/** class FocusedTextEdit */

FocusedTextEdit::FocusedTextEdit(bool disableUpdatesOnKeyPress, QWidget *parent)
    : KTextEdit(parent)
    , m_disableUpdatesOnKeyPress(disableUpdatesOnKeyPress)
{
    connect(this, &FocusedTextEdit::selectionChanged, this, &FocusedTextEdit::onSelectionChanged);
}

FocusedTextEdit::~FocusedTextEdit()
{
}

void FocusedTextEdit::paste(QClipboard::Mode mode)
{
    const QMimeData *md = QApplication::clipboard()->mimeData(mode);
    if (md)
        insertFromMimeData(md);
}

void FocusedTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
        return;
    }

    if (m_disableUpdatesOnKeyPress)
        setUpdatesEnabled(false);

    KTextEdit::keyPressEvent(event);

    // Workaround (for ensuring the cursor to be visible): signal not emitted when pressing those keys:
    if (event->key() == Qt::Key_Home || event->key() == Qt::Key_End || event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown)
        emit cursorPositionChanged();

    if (m_disableUpdatesOnKeyPress) {
        setUpdatesEnabled(true);
        if (!document()->isEmpty())
            ensureCursorVisible();
    }
}

void FocusedTextEdit::wheelEvent(QWheelEvent *event)
{
    // If we're already scrolled all the way to the top or bottom, we pass the
    // wheel event onto the basket.
    QScrollBar *sb = verticalScrollBar();
    if ((event->delta() > 0 && sb->value() > sb->minimum()) || (event->delta() < 0 && sb->value() < sb->maximum()))
        KTextEdit::wheelEvent(event);
    // else
    //    Global::bnpView->currentBasket()->graphicsView()->wheelEvent(event);
}

void FocusedTextEdit::enterEvent(QEvent *event)
{
    emit mouseEntered();
    KTextEdit::enterEvent(event);
}

void FocusedTextEdit::insertFromMimeData(const QMimeData *source)
{
    // When user always wants plaintext pasting, if both HTML and text data is
    // present, only send plain text data (the provided source is readonly and I
    // also can't just pass it to QMimeData constructor as the latter is 'private')
    if (Settings::pasteAsPlainText() && source->hasHtml() && source->hasText()) {
        QMimeData alteredSource;
        alteredSource.setData("text/plain", source->data("text/plain"));
        KTextEdit::insertFromMimeData(&alteredSource);
    } else
        KTextEdit::insertFromMimeData(source);
}

void FocusedTextEdit::onSelectionChanged()
{
    if (textCursor().selectedText().length() > 0) {
        QMimeData *md = createMimeDataFromSelection();
        QApplication::clipboard()->setMimeData(md, QClipboard::Selection);
    }
}

/** class FocusWidgetFilter */
FocusWidgetFilter::FocusWidgetFilter(QWidget *parent)
    : QObject(parent)
{
    if (parent)
        parent->installEventFilter(this);
}

bool FocusWidgetFilter::eventFilter(QObject *, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_Return:
            emit returnPressed();
            return true;
        case Qt::Key_Escape:
            emit escapePressed();
            return true;
        default:
            return false;
        };
    }
    case QEvent::Enter:
        emit mouseEntered();
        // pass through
    default:
        return false;
    };
}
