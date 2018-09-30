// Code from Amarok.

/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "crashhandler.h"
#include "config.h"

#include <KAboutData>
#include <QTemporaryFile>
#include <QGuiApplication>
#include <QDebug>
#include <KToolInvocation>
#include <KLocalizedString>

#include <kcoreaddons_version.h>

#include <QtCore/QRegExp>
#include <QDir>
#include <QFileInfoList>
#include <QProcess>
#ifdef Q_OS_WIN
#include <QSysInfo>
#endif

#include <cstdio>         //popen, fread
#include <sys/types.h>    //pid_t
#include <sys/wait.h>     //waitpid
#include <unistd.h>       //write, getppid


static QString
runCommand(const QByteArray &command)
{
    static const uint SIZE = 40960; //40 KiB
    static char stdoutBuf[ SIZE ];

//        debug() << "Running: " << command << endl;

    FILE *process = ::popen(command, "r");
    stdoutBuf[ std::fread(static_cast<void*>(stdoutBuf), sizeof(char), SIZE-1, process)] = '\0';
    ::pclose(process);

    return QString::fromLocal8Bit(stdoutBuf);
}

void
Crash::crashHandler(int /*signal*/)
{
#ifndef Q_OS_WIN
    // we need to fork to be able to get a
    // semi-decent bt - I dunno why
    const pid_t pid = ::fork();

    if (pid <= 0) {
        // we are the child process (the result of the fork)
//            debug() << "amaroK is crashing...\n";

        QString subject = "[basket-crash] " VERSION " ";
        QString body = i18n(
                           "%1 has crashed! We're sorry about this.\n"
                           "\n"
                           "But, all is not lost! You could potentially help us fix the crash. "
                           "Information describing the crash is below, so just click send, "
                           "or if you have time, write a brief description of how the crash happened first.\n\n"
                           "Many thanks.", QGuiApplication::applicationDisplayName()) + "\n\n";
        body += "\n\n\n\n\n\n" + i18n(
                    "The information below is to help the developers identify the problem, "
                    "please do not modify it.") + "\n\n\n\n";


        body += "======== DEBUG INFORMATION  =======\n"
                "Version:    " VERSION "\n"
                "CC version: " __VERSION__ "\n" //assuming we're using GCC
                "Frameworks: " KCOREADDONS_VERSION_STRING "\n"
                ;//                    "TagLib:     %2.%3.%4\n";

        /*            body = body
                            .arg( TAGLIB_MAJOR_VERSION )
                            .arg( TAGLIB_MINOR_VERSION )
                            .arg( TAGLIB_PATCH_VERSION );*/

#ifdef NDEBUG
        body += "NDEBUG:     true";
#endif
        body += "\n";
        body += "OS:\n" + Crash::getOSVersionInfo() + "\n";

        /// obtain the backtrace with gdb

    QTemporaryFile temp;
	temp.open();	
        temp.setAutoRemove(true);

        const int handle = temp.handle();

//             QCString gdb_command_string =
//                     "file amaroqApp\n"
//                     "attach " + QCString().setNum( ::getppid() ) + "\n"
//                     "bt\n" "echo \\n\n"
//                     "thread apply all bt\n";

        const QByteArray gdb_batch =
            "bt\n"
            "echo \\n\\n\n"
            "bt full\n"
            "echo \\n\\n\n"
            "echo ==== (gdb) thread apply all bt ====\\n\n"
            "thread apply all bt\n";

        ::write(handle, gdb_batch, gdb_batch.length());
        ::fsync(handle);

        // so we can read stderr too
        ::dup2(fileno(stdout), fileno(stderr));

        QByteArray gdb;
        gdb  = "gdb --nw -n --batch -x ";
        gdb += temp.fileName().toLatin1();
        gdb += " ";
    gdb += QGuiApplication::applicationFilePath();
	gdb += " ";
        gdb += QByteArray().setNum(::getppid());

 
       QString bt = runCommand(gdb);

        /// clean up
        bt.remove("(no debugging symbols found)...");
        bt.remove("(no debugging symbols found)\n");
        bt.replace(QRegExp("\n{2,}"), "\n");   //clean up multiple \n characters
        bt.trimmed();

        /// analyze usefulness
        bool useful = true;
        const QString fileCommandOutput = runCommand("file `which basket`");

        if (fileCommandOutput.indexOf("not stripped") == -1)
            subject += "[___stripped]"; //same length as below
        else
            subject += "[NOTstripped]";

        if (!bt.isEmpty()) {
            const int invalidFrames = bt.count(QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in \\?\\?"));
            const int validFrames = bt.count(QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in [^?]"));
            const int totalFrames = invalidFrames + validFrames;

            if (totalFrames > 0) {
                const double validity = double(validFrames) / totalFrames;
                subject += QString("[validity: %1]").arg(validity, 0, 'f', 2);
                if (validity <= 0.5) useful = false;
            }
            subject += QString("[frames: %1]").arg(totalFrames, 3 /*padding*/);

            if (bt.indexOf(QRegExp(" at \\w*\\.cpp:\\d+\n")) != -1)
                subject += "[line numbers]";
        } else
            useful = false;

//            subject += QString("[%1]").arg( AmarokConfig::soundSystem().remove( QRegExp("-?engine") ) );

//            debug() << subject << endl;

        //TODO -fomit-frame-pointer buggers up the backtrace, so detect it
        //TODO -O optimization can rearrange execution and stuff so show a warning for the developer
        //TODO pass the CXXFLAGS used with the email

        if (useful) {
            body += "==== file `which basket` ==========\n";
            body += fileCommandOutput + "\n";
            body += "==== (gdb) bt =====================\n";
            body += bt;//+ "\n\n";
//                body += "==== kBacktrace() ================\n";
//                body += kBacktrace();

            //TODO startup notification
            KToolInvocation::invokeMailer(
                /*to*/          "kde.basket@gmx.com",
                /*cc*/          QString(),
                /*bcc*/         QString(),
                /*subject*/     subject,
                /*body*/        body,
                /*messageFile*/ QString(),
                /*attachURLs*/  QStringList(),
                /*startup_id*/  "");
        } else {
            qDebug() << "\n" + i18n("%1 has crashed! We're sorry about this.\n\n"
                                    "But, all is not lost! Perhaps an upgrade is already available "
                                    "which fixes the problem. Please check your distribution's software repository.",
                                    QGuiApplication::applicationDisplayName());
        }

        //_exit() exits immediately, otherwise this
        //function is called repeatedly ad finitum
        ::_exit(255);
    }

    else {
        // we are the process that crashed
        
        ::alarm(0);

        // wait for child to exit
        ::waitpid(pid, NULL, 0);
        ::_exit(253);
    }
#endif //#ifndef Q_OS_WIN
}

QString Crash::getOSVersionInfo()
{
    QString result;

    #ifdef Q_OS_UNIX
    QProcess process;
    process.start("lsb_release", QStringList("-a"));
    if (process.waitForFinished()) {
        result += "lsb_release -a:\n";
        result += QString(process.readAll());
    }
    //Read version files in /etc
    QStringList versionFileMasks = QStringList() << "*release" << "*version";
    QFileInfoList versionFiles = QDir("/etc").entryInfoList(versionFileMasks, QDir::Files);
    foreach (const QFileInfo& versionFile, versionFiles) {
        result += versionFile.absoluteFilePath() + ":\n";
        QFile file(versionFile.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            result += stream.readAll();
        }
    }
    #else
    result = QString("Windows WinVersion=0x%1").arg((int)QSysInfo::windowsVersion(), 0, 16);
    #endif
    return result;
}
