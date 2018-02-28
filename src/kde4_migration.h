/*
 *   Copyright (C) 2015 by Gleb Baryshev
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

#ifndef KDE4_MIGRATION_H
#define KDE4_MIGRATION_H

#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QScopedPointer>
#include <QStandardPaths>
#include <Kdelibs4ConfigMigrator>
#include <Kdelibs4Migration>
#include <KIO/CopyJob>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include "aboutdata.h"
#include "global.h"


/** Use Kdelibs4ConfigMigrator to detect KDE4 config and then migrate both config and data.
    Suggest deleting the files from the old location

    Prerequisite: existing QApplication in order to run KIO Jobs and QEventLoop
 */
class Kde4Migrator
{
public:
    //! @returns True if both config and data have been migrated
    bool migrateKde4Data() {
        Kdelibs4ConfigMigrator rcMigrator(Global::basketAbout.componentName());
        rcMigrator.setConfigFiles({ BASKET_RC });
        rcMigrator.setUiFiles({ "basketui.rc", "basket_part.rc" });
        if (rcMigrator.migrate())
            qDebug() << "Kdelibs4ConfigMigrator migrate=true";
        else
            return false;

        // Safety check
        // ~/.local/share/basket
        if (QDir(getNewDataDir()).exists()) {
            qDebug() << "Directory" << getNewDataDir() << "already exists, not trying to overwrite";
            return false;
        }

        QString customDataFolder = getCustomDataFolder();
        if (customDataFolder.length() > 0) {
            qDebug() << "Keeping basket data in" << customDataFolder;
            return false; //Do not delete the data folder. Note: old basketrc will not be deleted either
        }

        m_dataMigrator.reset(new Kdelibs4Migration());
        if (m_dataMigrator->kdeHomeFound()) {
            bool copySucceeded = false;
            QEventLoop waitLoop;

            auto onCopyFinished = [&](KJob* job) {
                int error = job->error();
                qDebug() << "KIO::CopyJob finished with result" << error;
                if (error != 0) {
                    qDebug() << job->errorString();
                    QMessageBox::critical(NULL, QGuiApplication::applicationDisplayName(),
                        i18n("Failed to migrate Basket data from KDE4. You will need to close Basket and copy the basket folder manually.\n" \
                        "Source: %1\nDestination: %2\nReason: %3",
                        getOldDataDir(), getNewDataDir(), job->errorString()));
                }
                copySucceeded = (error == 0);
                waitLoop.quit();
            };

            qDebug() << "Kdelibs4Migration: start copying basket data";
            KIO::CopyJob* copyJob = KIO::copyAs(QUrl::fromLocalFile(getOldDataDir()), QUrl::fromLocalFile(getNewDataDir()));
            QObject::connect(copyJob, &KIO::CopyJob::result, onCopyFinished);
            waitLoop.exec();

            if (copySucceeded)
                return true;
        }

        return false;
    }


    void showPostMigrationDialog() {
        QMessageBox msgBox;
        msgBox.setWindowTitle(i18n("Choose action"));
        msgBox.setText(i18n("Basket data from KDE4 have been successfully migrated to %1.\n" \
                            "Unless you are planning to run KDE4 version again, you can delete the old folder %2",
                            getNewDataDir(), getOldDataDir()));
        msgBox.addButton(i18n("Delete (to Trash)"), QMessageBox::YesRole);
        msgBox.addButton(i18n("Keep"), QMessageBox::NoRole);
        msgBox.setIcon(QMessageBox::Information);
        int dialogCode = msgBox.exec();

        if (dialogCode == 0) {
            // ~/.kde/share/config/basketrc
            QString basketrc = m_dataMigrator->locateLocal("config", BASKET_RC);

            QList<QUrl> trashList = { QUrl::fromLocalFile(getOldDataDir()) };
            if (QFile(basketrc).exists())
                trashList.prepend(QUrl::fromLocalFile(basketrc));

            qDebug() << "Move to trash:" << trashList;
            KIO::trash(trashList);
        }
    }


private:
    const QString BASKET_RC = "basketrc";

    QScopedPointer<Kdelibs4Migration> m_dataMigrator;

    QString getOldConfig() {
        return m_dataMigrator->locateLocal("config", BASKET_RC);
    }

    QString getOldDataDir() {
        return m_dataMigrator->saveLocation("data", "basket");
    }

    QString getNewDataDir() {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+ "/basket";
    }

    QString getCustomDataFolder() {
        KSharedConfig::Ptr basketConfig = KSharedConfig::openConfig(BASKET_RC);
        KConfigGroup config = basketConfig->group("Main window");
        return config.readEntry("dataFolder", "");
    }
};

#endif // KDE4_MIGRATION_H
