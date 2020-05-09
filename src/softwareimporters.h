/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SOFTWAREIMPORTERS_H
#define SOFTWAREIMPORTERS_H

#include <QDialog>

class QString;
class QGroupBox;
class QDomElement;
class QRadioButton;
class KTextEdit;
class QVBoxLayout;

class BasketScene;
class Note;

/** The dialog to ask how to import hierarchical data.
 * @author Sébastien Laoût
 */
class TreeImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TreeImportDialog(QWidget *parent = nullptr);
    ~TreeImportDialog() override;
    int choice();

private:
    QGroupBox *m_choices;
    QVBoxLayout *m_choiceLayout;
    QRadioButton *m_hierarchy_choice;
    QRadioButton *m_separate_baskets_choice;
    QRadioButton *m_one_basket_choice;
};

/** The dialog to ask how to import text files.
 * @author Sébastien Laoût
 */
class TextFileImportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextFileImportDialog(QWidget *parent = nullptr);
    ~TextFileImportDialog() override;
    QString separator();
protected slots:
    void customSeparatorChanged();

private:
    QGroupBox *m_choices;
    QVBoxLayout *m_choiceLayout;
    QRadioButton *m_emptyline_choice;
    QRadioButton *m_newline_choice;
    QRadioButton *m_dash_choice;
    QRadioButton *m_star_choice;
    QRadioButton *m_all_in_one_choice;
    QRadioButton *m_anotherSeparator;
    KTextEdit *m_customSeparator;
};

/** Functions that import data from other softwares.
 * @author Sébastien Laoût
 */
namespace SoftwareImporters
{
void finishImport(BasketScene *basket);

// The importers in themselves:
void importTextFile();
}

#endif // SOFTWAREIMPORTERS_H
