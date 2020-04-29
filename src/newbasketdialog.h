/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NEWBASKETDIALOG_H
#define NEWBASKETDIALOG_H

#include <QDialog>

#include <QListWidget>
#include <QtCore/QMap>

class KIconButton;
class QLineEdit;
class QMimeData;
class KComboBox;
class QTreeWidgetItem;

class BasketScene;

class KColorCombo2;

/** The class QListWidget allow to drag items. We don't want to, so we disable it.
 * This class also unselect the selected item when the user right click an empty space. We don't want to, so we reselect it if that happens.
 * @author Sébastien Laoût
 */
class SingleSelectionKIconView : public QListWidget
{
    Q_OBJECT
public:
    explicit SingleSelectionKIconView(QWidget *parent = nullptr);
    QMimeData *dragObject();
    QListWidgetItem *selectedItem()
    {
        return m_lastSelected;
    }
private slots:
    void slotSelectionChanged(QListWidgetItem *cur);

private:
    QListWidgetItem *m_lastSelected;
};

/** Struct to store default properties of a new basket.
 * When the dialog shows up, the @p icon is used, as well as the @p backgroundColor.
 * A template is chosen depending on @p freeLayout and @p columnLayout.
 * If @p columnLayout is too high, the template with the more columns will be chosen instead.
 * If the user change the background color in the dialog, then @p backgroundImage and @p textColor will not be used!
 * @author Sébastien Laoût
 */
struct NewBasketDefaultProperties {
    QString icon;
    QString backgroundImage;
    QColor backgroundColor;
    QColor textColor;
    bool freeLayout;
    int columnCount;

    NewBasketDefaultProperties();
};

/** The dialog to create a new basket from a template.
 * @author Sébastien Laoût
 */
class NewBasketDialog : public QDialog
{
    Q_OBJECT
public:
    NewBasketDialog(BasketScene *parentBasket, const NewBasketDefaultProperties &defaultProperties, QWidget *parent = nullptr);
    ~NewBasketDialog() override;
    void ensurePolished();
protected slots:
    void slotOk();
    void returnPressed();
    void manageTemplates();
    void nameChanged(const QString &newName);

private:
    int populateBasketsList(QTreeWidgetItem *item, int indent, int index);
    NewBasketDefaultProperties m_defaultProperties;
    KIconButton *m_icon;
    QLineEdit *m_name;
    KColorCombo2 *m_backgroundColor;
    QListWidget *m_templates;
    KComboBox *m_createIn;
    QMap<int, BasketScene *> m_basketsMap;
    QPushButton *okButton;
};

#endif // NEWBASKETDIALOG_H
