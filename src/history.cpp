/**
 * SPDX-FileCopyrightText: (C) 2010 Brian C. Milco <bcmilco@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "history.h"
#include "global.h"

#include "basketscene.h"
#include "bnpview.h"

#include <KLocalizedString>
#include <QLocale>

HistorySetBasket::HistorySetBasket(BasketScene *basket, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(i18n("Set current basket to %1", basket->basketName()));
    m_folderNameOld = Global::bnpView->currentBasket()->folderName();
    m_folderNameNew = basket->folderName();
}

void HistorySetBasket::undo()
{
    BasketScene *oldBasket = Global::bnpView->basketForFolderName(m_folderNameOld);
    Global::bnpView->setCurrentBasket(oldBasket);
}

void HistorySetBasket::redo()
{
    BasketScene *curBasket = Global::bnpView->basketForFolderName(m_folderNameNew);
    Global::bnpView->setCurrentBasket(curBasket);
}
