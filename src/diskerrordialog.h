/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DISKERRORDIALOG_H
#define DISKERRORDIALOG_H

#include <QDialog>

class QCloseEvent;
class QKeyEvent;

/** Provide a dialog to avert the user the disk is full.
 * This dialog is modal and is shown until the user has made space on the disk.
 * @author Sébastien Laoût
 */
class DiskErrorDialog : public QDialog
{
    Q_OBJECT
public:
    DiskErrorDialog(const QString &message, QWidget *parent = nullptr);
    ~DiskErrorDialog() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *) override;
};

#endif // DISKERRORDIALOG_H
