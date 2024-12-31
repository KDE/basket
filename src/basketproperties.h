/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKETPROPERTIES_H
#define BASKETPROPERTIES_H

#include <KIconTheme>
#include <QDialog>
#include <QMap>

#include "ui_basketproperties.h"

class QString;

class QKeySequence;
class KColorCombo2;

class BasketScene;

/** The dialog that hold basket settings.
 * @author Sébastien Laoût
 */
class BasketPropertiesDialog : public QDialog, private Ui::BasketPropertiesUi
{
    Q_OBJECT
public:
    explicit BasketPropertiesDialog(BasketScene *basket, QWidget *parent = nullptr);
    ~BasketPropertiesDialog() override;
    void ensurePolished();

public Q_SLOTS:
    void applyChanges();

protected Q_SLOTS:
    void capturedShortcut(const QList<QKeySequence> &shortcut);
    void selectColumnsLayout();

private:
    BasketScene *m_basket;
    KColorCombo2 *m_backgroundColor;
    KColorCombo2 *m_textColor;

    QMap<int, QString> m_backgroundImagesMap;
};

#endif // BASKETPROPERTIES_H
