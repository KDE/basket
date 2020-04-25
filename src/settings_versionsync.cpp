/**
 * SPDX-FileCopyrightText: (C) 2016 Gleb Baryshev <gleb.baryshev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "settings_versionsync.h"
#include "aboutdata.h"
#include "settings.h"
#include "tools.h"
#include "ui_settings_versionsync.h"
#include <KLocalizedString>
#include <KMessageBox>
#include <QDir>
#include <QLocale>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

//! Compute size of .git folder and invoke VersionSyncPage::setHistorySize
void showHistorySize(QPointer<VersionSyncPage> versionSyncPage)
{
    QString git = Global::gitFolder();
    qint64 size = QDir(git).exists() ? Tools::computeSizeRecursively(git) : 0;
    if (!versionSyncPage.isNull())
        QMetaObject::invokeMethod(versionSyncPage.data(), "setHistorySize", Qt::QueuedConnection, Q_ARG(qint64, size));
}

VersionSyncPage::VersionSyncPage(QWidget *parent, const char *name)
    : KCModule(parent)
    , ui(new Ui::VersionSyncPage)
{
    KAboutData *about = new AboutData();
    about->setComponentName(name);
    setAboutData(about);

    ui->setupUi(this);

#ifdef WITH_LIBGIT2
    ui->labelWithoutVersionControlSupport->setVisible(false);
    QtConcurrent::run(showHistorySize, this);
#else
    ui->checkBoxEnable->setEnabled(false);
    ui->groupBoxControl->setVisible(false);
#endif

    connect(ui->checkBoxEnable, SIGNAL(toggled(bool)), this, SLOT(changed()));
    load();
}

VersionSyncPage::~VersionSyncPage()
{
    delete ui;
}

void VersionSyncPage::load()
{
    ui->checkBoxEnable->setChecked(Settings::versionSyncEnabled());
#ifdef WITH_LIBGIT2
    on_checkBoxEnable_clicked();
#endif
}

void VersionSyncPage::save()
{
    Settings::setVersionSyncEnabled(ui->checkBoxEnable->isChecked());
}

void VersionSyncPage::defaults()
{
    ui->checkBoxEnable->setChecked(false);
}

void VersionSyncPage::on_checkBoxEnable_clicked()
{
    ui->groupBoxControl->setEnabled(ui->checkBoxEnable->isChecked());
}

void VersionSyncPage::on_buttonClearHistory_clicked()
{
    if (KMessageBox::questionYesNo(this, i18n("Do you really want to remove old versions for all baskets?"), i18n("Version Sync")) == KMessageBox::Yes) {
        Tools::deleteRecursively(Global::gitFolder());
        ui->buttonClearHistory->setEnabled(false);
        setHistorySize(0);
        Global::initializeGitIfNeeded(Global::savesFolder()); // restore .git
    }
}

void VersionSyncPage::setHistorySize(qint64 size_bytes)
{
    QString size_mb = QLocale().toString((float)size_bytes / 1024 / 1024, 'f', 2);
    ui->labelHistorySize->setText(i18n("This will free %1 MB", size_mb));
}
