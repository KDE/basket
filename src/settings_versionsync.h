/*
 *   Copyright (C) 2016 by Gleb Baryshev
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef SETTINGS_VERSIONSYNC_H
#define SETTINGS_VERSIONSYNC_H

#include <KCModule>
#include "basket_export.h"

namespace Ui {
class VersionSyncPage;
}

class BASKET_EXPORT VersionSyncPage : public KCModule
{
    Q_OBJECT
    
public:
    explicit VersionSyncPage(QWidget * parent = 0, const char * name = 0);
    ~VersionSyncPage();

    virtual void load() override;
    virtual void save() override;
    virtual void defaults() override;

public slots:
    void setHistorySize(qint64 size_bytes);
    
private slots:
    void on_checkBoxEnable_clicked();
    void on_buttonClearHistory_clicked();

private:
    Ui::VersionSyncPage *ui;
};

#endif // SETTINGS_VERSIONSYNC_H
