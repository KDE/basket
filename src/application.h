/***************************************************************************
 *   Copyright (C) 2003 by Sébastien Laoût                                 *
 *   slaout@linux62.org                                                    *
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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <KDBusService>

/**
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class Application : public QApplication
{
public:
    Application(int &argc, char **argv);
    ~Application() override;
    int newInstance();
    void tryLoadFile(const QStringList &args, const QString& workingDir); //!< Open a file passed as command line argument
private slots:
    /// Activate program window if duplicate instance is started, load file from args
    void onActivateRequested(const QStringList& args, const QString& workingDir);
private:
    KDBusService m_service;
};

#endif // APPLICATION_H
