/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "exporterdialog.h"

#include <KUrlRequester>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KLineEdit>

#include <QtCore/QDir>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLocale>

#include "basketscene.h"
#include "global.h"

ExporterDialog::ExporterDialog(BasketScene *basket, QWidget *parent, const char *name)
        : QDialog(parent)
        , m_basket(basket)
{
    // Dialog options
    setObjectName(name);
    setModal(true);
    setWindowTitle(i18n("Export Basket to HTML"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    //PORTING SCRIPT: WARNING mainLayout->addWidget(buttonBox) must be last item in layout. Please move it.
    mainLayout->addWidget(buttonBox);
    okButton->setDefault(true);
    connect(okButton, SIGNAL(clicked()), SLOT(save()));

    QWidget *page  = new QWidget(this);
    QVBoxLayout *pageVBoxLayout = new QVBoxLayout(page);
    pageVBoxLayout->setMargin(0);
    QWidget     *wid  = new QWidget(page);
    pageVBoxLayout->addWidget(wid);
    mainLayout->addWidget(wid);
    //QHBoxLayout *hLay = new QHBoxLayout(wid, /*margin=*/0, spacingHint());
    QHBoxLayout *hLay = new QHBoxLayout(wid);
    mainLayout->addLayout(hLay);
    m_url = new KUrlRequester(QUrl(""), wid);
    mainLayout->addWidget(m_url);
    m_url->setWindowTitle(i18n("HTML Page Filename"));
    m_url->setFilter("text/html");
    m_url->fileDialog()->setAcceptMode(QFileDialog::AcceptSave);
    QLabel *fileLabel = new QLabel(wid);
    mainLayout->addWidget(fileLabel);
    fileLabel->setText(i18n("&Filename:"));
    fileLabel->setBuddy(m_url);
    hLay->addWidget(fileLabel);
    hLay->addWidget(m_url);

    m_embedLinkedFiles    = new QCheckBox(i18n("&Embed linked local files"),              page);
    pageVBoxLayout->addWidget(m_embedLinkedFiles);
    m_embedLinkedFolders  = new QCheckBox(i18n("Embed &linked local folders"),            page);
    pageVBoxLayout->addWidget(m_embedLinkedFolders);
    m_erasePreviousFiles  = new QCheckBox(i18n("Erase &previous files in target folder"), page);
    pageVBoxLayout->addWidget(m_erasePreviousFiles);
    m_formatForImpression = new QCheckBox(i18n("For&mat for impression"),                 page);
    pageVBoxLayout->addWidget(m_formatForImpression);
    m_formatForImpression->hide();

    load();
    m_url->lineEdit()->setFocus();

    //showTile(true);
    // Add a stretch at the bottom:
    // Duplicated code from AddBasketWizard::addStretch(QWidget *parent):
    (new QWidget(page))->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Double the width, because the filename should be visible
    QSize size(sizeHint());
    resize(QSize(size.width() * 2, size.height()));
    /*
    ==========================
    + [00000000000          ] Progress bar!
    + newBasket -> name folder as the basket
    */
}

ExporterDialog::~ExporterDialog()
{
}

void ExporterDialog::show()
{
    QDialog::show();

    QString lineEditText = m_url->lineEdit()->text();
    int selectionStart = lineEditText.lastIndexOf("/") + 1;
    m_url->lineEdit()->setSelection(selectionStart, lineEditText.length() - selectionStart - QString(".html").length());
}

void ExporterDialog::load()
{
    KConfigGroup config = Global::config()->group("HTML Export");

    QString folder = config.readEntry("lastFolder", QDir::homePath()) + "/";
    QString url = folder + QString(m_basket->basketName()).replace("/", "_") + ".html";
    m_url->setUrl(QUrl::fromLocalFile(url));

    m_embedLinkedFiles->setChecked(config.readEntry("embedLinkedFiles",    true));
    m_embedLinkedFolders->setChecked(config.readEntry("embedLinkedFolders",  false));
    m_erasePreviousFiles->setChecked(config.readEntry("erasePreviousFiles",  true));
    m_formatForImpression->setChecked(config.readEntry("formatForImpression", false));
}

void ExporterDialog::save()
{
    KConfigGroup config = Global::config()->group("HTML Export");

    QString folder = QUrl::fromLocalFile(m_url->url().toString()).adjusted(QUrl::RemoveFilename).path();
    config.writeEntry("lastFolder",          folder);
    config.writeEntry("embedLinkedFiles",    m_embedLinkedFiles->isChecked());
    config.writeEntry("embedLinkedFolders",  m_embedLinkedFolders->isChecked());
    config.writeEntry("erasePreviousFiles",  m_erasePreviousFiles->isChecked());
    config.writeEntry("formatForImpression", m_formatForImpression->isChecked());
}

void ExporterDialog::accept()
{
    save();
}

QString ExporterDialog::filePath()
{
    return m_url->url().url();
}

bool ExporterDialog::embedLinkedFiles()
{
    return m_embedLinkedFiles->isChecked();
}

bool ExporterDialog::embedLinkedFolders()
{
    return m_embedLinkedFolders->isChecked();
}

bool ExporterDialog::erasePreviousFiles()
{
    return m_erasePreviousFiles->isChecked();
}

bool ExporterDialog::formatForImpression()
{
    return m_formatForImpression->isChecked();
}

