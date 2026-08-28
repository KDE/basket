/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKETPROPERTIES_H
#define BASKETPROPERTIES_H

#include <QDialog>
#include <QMap>

class QString;

class QKeySequence;
class KColorCombo2;

class BasketScene;

namespace Ui
{
class BasketPropertiesUi;
}

/** The dialog that hold basket settings.
 * @author Sébastien Laoût
 */
class BasketPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BasketPropertiesDialog(BasketScene *basket, QWidget *parent = nullptr);
    ~BasketPropertiesDialog() override;

public Q_SLOTS:
    void applyChanges();

protected Q_SLOTS:
    void capturedShortcut(const QList<QKeySequence> &shortcut);
    void selectColumnsLayout();

protected:
    bool event(QEvent *event) override;

private:
    Ui::BasketPropertiesUi *m_ui;
    BasketScene *m_basket;
    KColorCombo2 *m_backgroundColor;
    KColorCombo2 *m_textColor;

    QMap<int, QString> m_backgroundImagesMap;
};

#endif // BASKETPROPERTIES_H
