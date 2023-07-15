/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "softwareimporters.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExp>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QtCore/QDir>
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtXml/QDomDocument>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTextEdit>

#include "basketfactory.h"
#include "basketscene.h"
#include "bnpview.h"
#include "debugwindow.h"
#include "global.h"
#include "icon_names.h"
#include "notefactory.h"
#include "tools.h"
#include "xmlwork.h"

/** class TreeImportDialog: */

TreeImportDialog::TreeImportDialog(QWidget *parent)
    : QDialog(parent)
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *topLayout = new QVBoxLayout(page);

    // QDialog options
    setWindowTitle(i18n("Import Hierarchy"));

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    setObjectName("ImportHeirachy");
    setModal(true);

    m_choices = new QGroupBox(i18n("How to Import the Notes?"), page);
    m_choiceLayout = new QVBoxLayout();
    m_choices->setLayout(m_choiceLayout);

    m_hierarchy_choice = new QRadioButton(i18n("&Keep original hierarchy (all notes in separate baskets)"), m_choices);
    m_separate_baskets_choice = new QRadioButton(i18n("&First level notes in separate baskets"), m_choices);
    m_one_basket_choice = new QRadioButton(i18n("&All notes in one basket"), m_choices);

    m_hierarchy_choice->setChecked(true);
    m_choiceLayout->addWidget(m_hierarchy_choice);
    m_choiceLayout->addWidget(m_separate_baskets_choice);
    m_choiceLayout->addWidget(m_one_basket_choice);

    topLayout->addWidget(m_choices);
    topLayout->addStretch(10);

    mainLayout->addWidget(page);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TreeImportDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TreeImportDialog::reject);
    mainLayout->addWidget(buttonBox);
}

TreeImportDialog::~TreeImportDialog()
{
}

int TreeImportDialog::choice()
{
    if (m_hierarchy_choice->isChecked())
        return 1;
    else if (m_separate_baskets_choice->isChecked())
        return 2;
    else
        return 3;
}

/** class TextFileImportDialog: */

TextFileImportDialog::TextFileImportDialog(QWidget *parent)
    : QDialog(parent)
{
    QWidget *page = new QWidget(this);
    QVBoxLayout *topLayout = new QVBoxLayout(page);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // QDialog options
    setWindowTitle(i18n("Import Text File"));
    setObjectName("ImportTextFile");
    setModal(true);

    m_choices = new QGroupBox(i18n("Format of the Text File"), page);
    mainLayout->addWidget(m_choices);
    m_choiceLayout = new QVBoxLayout;
    m_choices->setLayout(m_choiceLayout);

    m_emptyline_choice = new QRadioButton(i18n("Notes separated by an &empty line"), m_choices);
    m_newline_choice = new QRadioButton(i18n("One &note per line"), m_choices);
    m_dash_choice = new QRadioButton(i18n("Notes begin with a &dash (-)"), m_choices);
    m_star_choice = new QRadioButton(i18n("Notes begin with a &star (*)"), m_choices);
    m_anotherSeparator = new QRadioButton(i18n("&Use another separator:"), m_choices);

    m_choiceLayout->addWidget(m_emptyline_choice);
    m_choiceLayout->addWidget(m_newline_choice);
    m_choiceLayout->addWidget(m_dash_choice);
    m_choiceLayout->addWidget(m_star_choice);
    m_choiceLayout->addWidget(m_anotherSeparator);

    QWidget *indentedTextEdit = new QWidget(m_choices);
    m_choiceLayout->addWidget(indentedTextEdit);

    QHBoxLayout *hLayout = new QHBoxLayout(indentedTextEdit);
    hLayout->addSpacing(20);
    m_customSeparator = new KTextEdit(indentedTextEdit);
    hLayout->addWidget(m_customSeparator);

    m_all_in_one_choice = new QRadioButton(i18n("&All in one note"), m_choices);
    m_choiceLayout->addWidget(m_all_in_one_choice);

    m_emptyline_choice->setChecked(true);
    topLayout->addWidget(m_choices);

    connect(m_customSeparator, &KTextEdit::textChanged, this, &TextFileImportDialog::customSeparatorChanged);

    mainLayout->addWidget(page);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &TextFileImportDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &TextFileImportDialog::reject);
    mainLayout->addWidget(buttonBox);
}

TextFileImportDialog::~TextFileImportDialog()
{
}

QString TextFileImportDialog::separator()
{
    if (m_emptyline_choice->isChecked())
        return "\n\n";
    else if (m_newline_choice->isChecked())
        return "\n";
    else if (m_dash_choice->isChecked())
        return "\n-";
    else if (m_star_choice->isChecked())
        return "\n*";
    else if (m_anotherSeparator->isChecked())
        return m_customSeparator->toPlainText();
    else if (m_all_in_one_choice->isChecked())
        return QString();
    else
        return "\n\n";
}

void TextFileImportDialog::customSeparatorChanged()
{
    if (!m_anotherSeparator->isChecked())
        m_anotherSeparator->toggle();
}

/** namespace SoftwareImporters: */

void SoftwareImporters::finishImport(BasketScene *basket)
{
    // Unselect the last inserted group:
    basket->unselectAll();

    // Focus the FIRST note (the last inserted note is currently focused!):
    basket->setFocusedNote(basket->firstNoteShownInStack());

    // Relayout every notes at their new place and simulate a load animation (because already loaded just after the creation).
    // Without a relayouting, notes on the bottom would comes from the top (because they were inserted on top) and clutter the animation load:
    basket->relayoutNotes();
    basket->save();
}

void SoftwareImporters::importTextFile()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr, QString(), "kfiledialog:///:ImportTextFile", "*|All files");
    if (fileName.isEmpty())
        return;

    TextFileImportDialog dialog;
    if (dialog.exec() == QDialog::Rejected)
        return;
    QString separator = dialog.separator();

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString content = stream.readAll();
        QStringList list = (separator.isEmpty() ? QStringList(content) : content.split(separator));

        // First create a basket for it:
        QString title = i18nc("From TextFile.txt", "From %1", QUrl::fromLocalFile(fileName).fileName());
        BasketFactory::newBasket(QStringLiteral("txt"), title);
        BasketScene *basket = Global::bnpView->currentBasket();
        basket->load();

        // Import every notes:
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
            Note *note = NoteFactory::createNoteFromText((*it).trimmed(), basket);
            basket->insertNote(note, basket->firstNote(), Note::BottomColumn);
        }

        // Finish the export:
        finishImport(basket);
    }
}

#include "moc_softwareimporters.cpp"
