/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTimer>

#include <KLocalizedString>

#include "aboutdata.h"
#include "bnpview.h"
#include "config.h"
#include "global.h"
#include "mainwindow.h"

#ifdef WITH_LIBGIT2
extern "C" {
#include <git2.h>
}
#endif

using namespace std::chrono_literals;

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_service(KDBusService::Unique)
{
    KLocalizedString::setApplicationDomain("basket");
    setWindowIcon(QIcon::fromTheme(QStringLiteral("basket")));

    KAboutData::setApplicationData(AboutData());
    // BasketPart::createAboutData();

    connect(&m_service, &KDBusService::activateRequested, this, &Application::onActivateRequested);

#ifdef WITH_LIBGIT2
    git_libgit2_init();
#endif
}

Application::~Application()
{
#ifdef WITH_LIBGIT2
    git_libgit2_shutdown();
#endif
}

void Application::tryLoadFile(const QStringList &args, const QString &workingDir)
{
    // Open the basket archive or template file supplied as argument:
    if (args.count() >= 1) {
        QString fileName = QDir(workingDir).filePath(args.last());

        if (QFile::exists(fileName)) {
            QFileInfo fileInfo(fileName);
            if (fileInfo.absoluteFilePath().contains(Global::basketsFolder())) {
                QString folder = fileInfo.absolutePath().split(QLatin1Char('/')).last();
                folder.append(QStringLiteral("/"));
                BNPView::s_basketToOpen = folder;
                QTimer::singleShot(100ms, Global::bnpView, &BNPView::delayedOpenBasket);
            } else if (!fileInfo.isDir()) { // Do not mis-interpret data-folder param!
                // Tags are not loaded until Global::bnpView::lateInit() is called.
                // It is called 0ms after the application start.
                BNPView::s_fileToOpen = fileName;
                QTimer::singleShot(100ms, Global::bnpView, &BNPView::delayedOpenArchive);
                //              Global::bnpView->openArchive(fileName);
            }
        }
    }
}

void Application::onActivateRequested(const QStringList &args, const QString &workingDir)
{
    if (MainWindow *wnd = Global::mainWindow()) {
        // Restore window:
        wnd->show(); // from tray
        wnd->setWindowState(Qt::WindowActive); // from minimized
        // Raise to the top
        wnd->raise();
    }
    tryLoadFile(args, workingDir);
}
