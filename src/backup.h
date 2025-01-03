/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BACKUP_H
#define BACKUP_H

#include <QDialog>

#include <QThread>

class QApplication;
class QLabel;

#include "basket_export.h"

/**
 * @author Sébastien Laoût
 */
class BackupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BackupDialog(QWidget *parent = nullptr);
    ~BackupDialog() override;
private Q_SLOTS:
    void moveToAnotherFolder();
    void useAnotherExistingFolder();
    void backup();
    void restore();
    void populateLastBackup();

private:
    QLabel *m_lastBackup = nullptr;
};

/**
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class BASKET_EXPORT Backup
{
public:
    static void figureOutBinaryPath(const char *argv0, QApplication &app);
    static void setFolderAndRestart(const QString &folder, const QString &message);
    static QString newSafetyFolder();

private:
    static QString binaryPath;
};

class BackupThread : public QThread
{
public:
    BackupThread(const QString &tarFile, const QString &folderToBackup);

protected:
    void run() override;

private:
    QString m_tarFile;
    QString m_folderToBackup;
};

class RestoreThread : public QThread
{
public:
    RestoreThread(const QString &tarFile, const QString &destFolder);
    inline bool success()
    {
        return m_success;
    }

protected:
    void run() override;

private:
    QString m_tarFile;
    QString m_destFolder;
    bool m_success;
};

#endif // BACKUP_H
