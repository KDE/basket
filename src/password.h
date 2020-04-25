/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef PASSWORD_H
#define PASSWORD_H

#include <config.h>

#ifdef HAVE_LIBGPGME

#include "ui_passwordlayout.h"
#include <QDialog>

/**
    @author Petri Damsten <damu@iki.fi>
*/
class Password : public QWidget, public Ui::PasswordLayout
{
    Q_OBJECT
public:
    explicit Password(QWidget *parent = nullptr);
    ~Password();
};

class PasswordDlg : public QDialog
{
    Q_OBJECT
public:
    PasswordDlg(QWidget *parent = 0);
    ~PasswordDlg();

    QString key() const;
    int type() const;
    void setKey(const QString &key);
    void setType(int type);

    /** Reimplemented from {K,Q}Dialog
     */
    void accept();

private:
    Password *w;
};

#endif // HAVE_LIBGPGME

#endif // PASSWORD_H
