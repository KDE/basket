/***************************************************************************
 *   Copyright (C) 2006 by Petri Damsten                                   *
 *   damu@iki.fi                                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
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
