/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "diskerrordialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>
#include <QtCore/QString>
#include <QtGui/QCloseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPixmap>

#include <KIconLoader>
#include <KLocalizedString>

DiskErrorDialog::DiskErrorDialog(const QString &message, QWidget *parent)
    : QDialog(parent)
{
    setObjectName("DiskError");
    setWindowTitle(i18n("Save Error"));
    // enableButtonCancel(false);
    // enableButtonClose(false);
    // enableButton(Close, false);
    // okButton->setEnabled(false);
    setModal(true);
    // QHBoxLayout *layout = new QHBoxLayout(mainWidget(), /*margin=*/0, spacingHint());
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QHBoxLayout *layout = new QHBoxLayout(mainWidget);
    QPixmap icon = KIconLoader::global()->loadIcon("drive-harddisk", KIconLoader::NoGroup, 64, KIconLoader::DefaultState, QStringList(), /*path_store=*/nullptr, /*canReturnNull=*/true);
    QLabel *iconLabel = new QLabel(mainWidget);
    iconLabel->setPixmap(icon);
    iconLabel->setFixedSize(iconLabel->sizeHint());
    QLabel *label = new QLabel("<p><nobr><b><font size='+1'>" + i18n("Error while saving") + "</font></b></nobr></p><p>" + message + "</p>", mainWidget);
    if (!icon.isNull())
        layout->addWidget(iconLabel);
    layout->addWidget(label);
}

DiskErrorDialog::~DiskErrorDialog()
{
}

void DiskErrorDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

void DiskErrorDialog::keyPressEvent(QKeyEvent *)
{
    // Escape should not close the window...
}
