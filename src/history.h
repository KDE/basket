/**
 * SPDX-FileCopyrightText: (C) 2010 Brian C. Milco <bcmilco@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <QUndoCommand>

class BasketScene;

class HistorySetBasket : public QUndoCommand
{
public:
    explicit HistorySetBasket(BasketScene *basket, QUndoCommand *parent = 0);
    void undo() override;
    void redo() override;

private:
    QString m_folderNameOld;
    QString m_folderNameNew;
};

#endif // HISTORY_H
