/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <KDBusService>

/**
 * @class Application
 * @brief Base application
 * @author Sébastien Laoût <slaout@linux62.org>
 */
class Application : public QApplication
{
public:
    Application(int &argc, char **argv);
    ~Application() override;
    void tryLoadFile(const QStringList &args, const QString& workingDir); //!< Open a file passed as command line argument
private slots:
    /// Activate program window if duplicate instance is started, load file from args
    void onActivateRequested(const QStringList& args, const QString& workingDir);
private:
    KDBusService m_service;
};

#endif // APPLICATION_H
