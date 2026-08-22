/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "passworddialog.h"

#ifdef HAVE_LIBGPGME

#include <QDialogButtonBox>
#include <QPushButton>
#include <QString>

#include <KLocalizedString>
#include <KMessageBox>

#include "basketscene.h"
#include "kgpgme.h"

#include <ui_passworddialog.h>

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::PasswordDialog)
{
    m_ui->setupUi(this);

    // QDialog options
    QPushButton *okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);

    KGpgMe gpg;
    KGpgKeyList list = gpg.keys(true);
    for (KGpgKeyList::iterator it = list.begin(); it != list.end(); ++it) {
        QString name = gpg.checkForUtf8((*it).name);
        m_ui->keyCombo->addItem(QStringLiteral("%1 <%2> %3").arg(name).arg((*it).email).arg((*it).id), (*it).id);
    }
    m_ui->publicPrivateRadioButton->setEnabled(m_ui->keyCombo->count() > 0);
    m_ui->keyCombo->setEnabled(m_ui->keyCombo->count() > 0);
}

PasswordDialog::~PasswordDialog()
{
    delete m_ui;
}

void PasswordDialog::accept()
{
    int n = type();
    if (n == BasketScene::PrivateKeyEncryption && key().isEmpty())
        KMessageBox::error(this, i18n("No private key selected."));
    else
        QDialog::accept();
}

QString PasswordDialog::key() const
{
    return m_ui->keyCombo->currentData().toString();
}

int PasswordDialog::type() const
{
    if (m_ui->noPasswordRadioButton->isChecked())
        return BasketScene::NoEncryption;
    else if (m_ui->passwordRadioButton->isChecked())
        return BasketScene::PasswordEncryption;
    else if (m_ui->publicPrivateRadioButton->isChecked())
        return BasketScene::PrivateKeyEncryption;
    return -1;
}

void PasswordDialog::setKey(const QString &key)
{
    const int index = m_ui->keyCombo->findData(key);
    if (index >= 0) {
        m_ui->keyCombo->setCurrentIndex(index);
    }
}

void PasswordDialog::setType(int type)
{
    if (type == BasketScene::NoEncryption)
        m_ui->noPasswordRadioButton->setChecked(true);
    else if (type == BasketScene::PasswordEncryption)
        m_ui->passwordRadioButton->setChecked(true);
    else if (type == BasketScene::PrivateKeyEncryption)
        m_ui->publicPrivateRadioButton->setChecked(true);
}

#include "moc_passworddialog.cpp"

#endif
