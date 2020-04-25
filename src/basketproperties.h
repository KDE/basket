/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BASKETPROPERTIES_H
#define BASKETPROPERTIES_H

#include <KIconButton>
#include <QDialog>
#include <QtCore/QMap>

#include "ui_basketproperties.h"

class KIconButton;
class QLineEdit;
class QGroupBox;
class QVBoxLayout;
class QRadioButton;
class QString;

class KComboBox;
class KShortcutWidget;
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
    explicit BasketPropertiesDialog(BasketScene *basket, QWidget *parent = 0);
    ~BasketPropertiesDialog() override;
    void ensurePolished();

public slots:
    void applyChanges();

protected slots:
    void capturedShortcut(const QList<QKeySequence> &shortcut);
    void selectColumnsLayout();

private:
    BasketScene *m_basket;
    KColorCombo2 *m_backgroundColor;
    KColorCombo2 *m_textColor;

    QMap<int, QString> m_backgroundImagesMap;
};

#endif // BASKETPROPERTIES_H
