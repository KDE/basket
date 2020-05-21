/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "password.h"

#ifdef HAVE_LIBGPGME

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

#include "basketscene.h"
#include "kgpgme.h"

PasswordDlg::PasswordDlg(QWidget *parent)
    : QDialog(parent)
    , w(0)
{
    // QDialog options
    setWindowTitle(i18n("Password Protection"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PasswordDlg::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PasswordDlg::reject);
    mainLayout->addWidget(buttonBox);
    okButton->setDefault(true);
    setModal(true);

    QHBoxLayout *toplayout = new QHBoxLayout(mainWidget);
    w = new Password;
    toplayout->addWidget(w, 1);
}

PasswordDlg::~PasswordDlg()
{
    delete w;
}

void PasswordDlg::accept()
{
    int n = type();
    if (n == BasketScene::PrivateKeyEncryption && key().isEmpty())
        KMessageBox::error(w, i18n("No private key selected."));
    else
        QDialog::accept();
}

QString PasswordDlg::key() const
{
    QString s = w->keyCombo->currentText();
    if (s.length() < 16)
        return QString();
    int n = s.lastIndexOf(' ');
    if (n < 0)
        return QString();
    return s.mid(n + 1);
}

int PasswordDlg::type() const
{
    if (w->noPasswordRadioButton->isChecked())
        return BasketScene::NoEncryption;
    else if (w->passwordRadioButton->isChecked())
        return BasketScene::PasswordEncryption;
    else if (w->publicPrivateRadioButton->isChecked())
        return BasketScene::PrivateKeyEncryption;
    return -1;
}

void PasswordDlg::setKey(const QString &key)
{
    for (int i = 0; i < w->keyCombo->count(); ++i) {
        if (w->keyCombo->itemText(i).contains(key)) {
            w->keyCombo->setCurrentIndex(i);
            return;
        }
    }
}

void PasswordDlg::setType(int type)
{
    if (type == BasketScene::NoEncryption)
        w->noPasswordRadioButton->setChecked(true);
    else if (type == BasketScene::PasswordEncryption)
        w->passwordRadioButton->setChecked(true);
    else if (type == BasketScene::PrivateKeyEncryption)
        w->publicPrivateRadioButton->setChecked(true);
}

Password::Password(QWidget *parent)
    : QWidget(parent)
{
    // Setup from the UI file
    setupUi(this);

    KGpgMe gpg;

    KGpgKeyList list = gpg.keys(true);
    for (KGpgKeyList::iterator it = list.begin(); it != list.end(); ++it) {
        QString name = gpg.checkForUtf8((*it).name);

        keyCombo->addItem(QString("%1 <%2> %3").arg(name).arg((*it).email).arg((*it).id));
    }
    publicPrivateRadioButton->setEnabled(keyCombo->count() > 0);
    keyCombo->setEnabled(keyCombo->count() > 0);
}

Password::~Password()
{
}

#endif
