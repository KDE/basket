/**
 * SPDX-FileCopyrightText: (C) 2016 Gleb Baryshev <gleb.baryshev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SETTINGS_VERSIONSYNC_H
#define SETTINGS_VERSIONSYNC_H

#include "basket_export.h"
#include <KCModule>

namespace Ui
{
class VersionSyncPage;
}

class BASKET_EXPORT VersionSyncPage : public KCModule
{
    Q_OBJECT

public:
    explicit VersionSyncPage(QWidget *parent = nullptr, const char *name = nullptr);
    ~VersionSyncPage() override;

    void load() override;
    void save() override;
    void defaults() override;

public slots:
    void setHistorySize(qint64 size_bytes);

private slots:
    void onCheckBoxEnableClicked();
    void onButtonClearHistoryClicked();

private:
    Ui::VersionSyncPage *ui;
};

#endif // SETTINGS_VERSIONSYNC_H
