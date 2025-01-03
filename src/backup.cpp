/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "backup.h"

#include "formatimporter.h" // To move a folder
#include "global.h"
#include "settings.h"
#include "tools.h"
#include "variouswidgets.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLocale>
#include <QProgressBar>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
// #include <KRun>
#include <KTar>

#include <KIO/CommandLauncherJob>

#include <unistd.h> // usleep()

/**
 * Backups are wrapped in a .tar.gz, inside that folder name.
 * An archive is not a backup or is corrupted if data are not in that folder!
 */
const QString backupMagicFolder = QStringLiteral("BasKet-Note-Pads_Backup");

/** class BackupDialog: */

BackupDialog::BackupDialog(QWidget *parent)
    : QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Backup & Restore"));

    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    auto *page = new QWidget(this);
    auto *pageVBoxLayout = new QVBoxLayout(page);
    pageVBoxLayout->setContentsMargins({});
    mainLayout->addWidget(page);

    //  pageVBoxLayout->setSpacing(spacingHint());

    QString savesFolder = Global::savesFolder();
    savesFolder = savesFolder.left(savesFolder.length() - 1); // savesFolder ends with "/"

    auto *folderGroup = new QGroupBox(i18n("Save Folder"), page);
    pageVBoxLayout->addWidget(folderGroup);
    mainLayout->addWidget(folderGroup);
    auto *folderGroupLayout = new QVBoxLayout;
    folderGroup->setLayout(folderGroupLayout);
    folderGroupLayout->addWidget(
        new QLabel(QStringLiteral("<qt><nobr>") + i18n("Your baskets are currently stored in that folder:<br><b>%1</b>", savesFolder), folderGroup));
    auto *folderWidget = new QWidget;
    folderGroupLayout->addWidget(folderWidget);

    auto *folderLayout = new QHBoxLayout(folderWidget);
    folderLayout->setContentsMargins(0, 0, 0, 0);

    auto *moveFolder = new QPushButton(i18n("&Move to Another Folder..."), folderWidget);
    auto *useFolder = new QPushButton(i18n("&Use Another Existing Folder..."), folderWidget);
    auto *helpLabel = new HelpLabel(
        i18n("Why to do that?"),
        i18n("<p>You can move the folder where %1 store your baskets to:</p><ul>"
             "<li>Store your baskets in a visible place in your home folder, like ~/Notes or ~/Baskets, so you can manually backup them when you want.</li>"
             "<li>Store your baskets on a server to share them between two computers.<br>"
             "In this case, mount the shared-folder to the local file system and ask %1 to use that mount point.<br>"
             "Warning: you should not run %1 at the same time on both computers, or you risk to loss data while the two applications are desynced.</li>"
             "</ul><p>Please remember that you should not change the content of that folder manually (eg. adding a file in a basket folder will not add that "
             "file to the basket).</p>",
             QGuiApplication::applicationDisplayName()),
        folderWidget);
    folderLayout->addWidget(moveFolder);
    folderLayout->addWidget(useFolder);
    folderLayout->addWidget(helpLabel);
    folderLayout->addStretch();
    connect(moveFolder, &QPushButton::clicked, this, &BackupDialog::moveToAnotherFolder);
    connect(useFolder, &QPushButton::clicked, this, &BackupDialog::useAnotherExistingFolder);

    auto *backupGroup = new QGroupBox(i18n("Backups"), page);
    pageVBoxLayout->addWidget(backupGroup);
    mainLayout->addWidget(backupGroup);
    auto *backupGroupLayout = new QVBoxLayout;
    backupGroup->setLayout(backupGroupLayout);
    auto *backupWidget = new QWidget;
    backupGroupLayout->addWidget(backupWidget);

    auto *backupLayout = new QHBoxLayout(backupWidget);
    backupLayout->setContentsMargins(0, 0, 0, 0);

    auto *backupButton = new QPushButton(i18n("&Backup..."), backupWidget);
    auto *restoreButton = new QPushButton(i18n("&Restore a Backup..."), backupWidget);
    m_lastBackup = new QLabel(QString(), backupWidget);
    backupLayout->addWidget(backupButton);
    backupLayout->addWidget(restoreButton);
    backupLayout->addWidget(m_lastBackup);
    backupLayout->addStretch();
    connect(backupButton, &QPushButton::clicked, this, &BackupDialog::backup);
    connect(restoreButton, &QPushButton::clicked, this, &BackupDialog::restore);

    populateLastBackup();

    (new QWidget(page))->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BackupDialog::reject);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
}

BackupDialog::~BackupDialog() = default;

void BackupDialog::populateLastBackup()
{
    QString lastBackupText = i18n("Last backup: never");
    if (Settings::lastBackup().isValid())
        lastBackupText = i18n("Last backup: %1", Settings::lastBackup().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss")));

    m_lastBackup->setText(lastBackupText);
}

void BackupDialog::moveToAnotherFolder()
{
    const QString currentSavesFolder = Global::savesFolder();
    const QUrl selectedURL = QFileDialog::getExistingDirectoryUrl(this, i18n("Choose a Folder Where to Move Baskets"), QUrl::fromLocalFile(currentSavesFolder));

    if (!selectedURL.isEmpty()) {
        QString folder = selectedURL.path();
        QDir dir(folder);
        // The folder should not exists, or be empty (because KDirSelectDialog will likely create it anyway):
        if (dir.exists()) {
            // Get the content of the folder:
            QStringList content = dir.entryList();
            if (content.count() > 2) { // "." and ".."
                int result =
                    KMessageBox::warningContinueCancel(nullptr,
                                                       QStringLiteral("<qt>") + i18n("The folder <b>%1</b> is not empty. Do you want to overwrite it?", folder),
                                                       i18n("Overwrite Folder?"),
                                                       KGuiItem(i18n("&Overwrite"), QStringLiteral("document-save")));
                if (result == KMessageBox::Cancel)
                    return;
            }
            Tools::deleteRecursively(folder);
        }
        FormatImporter copier;
        copier.moveFolder(currentSavesFolder, folder);
        Backup::setFolderAndRestart(
            folder,
            i18n("Your baskets have been successfully moved to <b>%1</b>. %2 is going to be restarted to take this change into account."));
    }
}

void BackupDialog::useAnotherExistingFolder()
{
    const QString currentSavesFolder = Global::savesFolder();
    const QUrl selectedURL = QFileDialog::getExistingDirectoryUrl(this, i18n("Choose a Folder Where to Move Baskets"), QUrl::fromLocalFile(currentSavesFolder));

    if (!selectedURL.isEmpty()) {
        Backup::setFolderAndRestart(
            selectedURL.path(),
            i18n("Your basket save folder has been successfully changed to <b>%1</b>. %2 is going to be restarted to take this change into account."));
    }
}

void BackupDialog::backup()
{
    QDir dir;

    // Compute a default file name & path (eg. "Baskets_2007-01-31.tar.gz"):
    KConfig *config = KSharedConfig::openConfig().data();
    KConfigGroup configGroup(config, QStringLiteral("Backups"));
    QString folder = configGroup.readEntry("lastFolder", QDir::homePath()) + QLatin1Char('/');
    QString fileName = i18nc("Backup filename (without extension), %1 is the date", "Baskets_%1", QDate::currentDate().toString(Qt::ISODate));
    QString url = folder + fileName;

    // Ask a file name & path to the user:
    QString suffix = QStringLiteral("*.tar.gz|");
    QString title = i18n("Tar Archives Compressed by Gzip");
    QString newline = QStringLiteral("\n*|");
    QString all = i18n("All Files");
    QString filter = suffix + title + newline + all;
    const QString destination = QFileDialog::getSaveFileName(nullptr, i18n("Backup Baskets"), url, filter);

    // User canceled? b
    if (destination.isEmpty()) {
        return;
    }

    QProgressDialog dialog;
    dialog.setWindowTitle(i18n("Backup Baskets"));
    dialog.setLabelText(i18n("Backing up baskets. Please wait..."));
    dialog.setModal(true);
    dialog.setCancelButton(nullptr);
    dialog.setAutoClose(true);

    dialog.setRange(0, 0 /*Busy/Undefined*/);
    dialog.setValue(0);
    dialog.show();

    /* If needed, uncomment this and call similar code in other places below
    QProgressBar* progress = new QProgressBar(dialog);
    progress->setTextVisible(false);
    dialog.setBar(progress);*/

    BackupThread thread(destination, Global::savesFolder());
    thread.start();
    while (thread.isRunning()) {
        dialog.setValue(dialog.value() + 1); // Or else, the animation is not played!
        qApp->processEvents();
        usleep(300); // Not too long because if the backup process is finished, we wait for nothing
    }

    Settings::setLastBackup(QDate::currentDate());
    Settings::saveConfig();
    populateLastBackup();
}

void BackupDialog::restore()
{
    // Get last backup folder:
    KConfig *config = KSharedConfig::openConfig().data();
    KConfigGroup configGroup(config, QStringLiteral("Backups"));
    QString folder = configGroup.readEntry(QStringLiteral("lastFolder"), QDir::homePath()) + QLatin1Char('/');

    // Ask a file name to the user:
    QString suffix = QStringLiteral("*.tar.gz|");
    QString title = i18n("Tar Archives Compressed by Gzip");
    QString newline = QStringLiteral("\n*|");
    QString all = i18n("All Files");
    QString filter = suffix + title + newline + all;

    QString path = QFileDialog::getOpenFileName(this, i18n("Open Basket Archive"), folder, filter);
    if (path.isEmpty()) // User has canceled
        return;

    // Before replacing the basket data folder with the backup content, we safely backup the current baskets to the home folder.
    // So if the backup is corrupted or something goes wrong while restoring (power cut...) the user will be able to restore the old working data:
    QString safetyPath = Backup::newSafetyFolder();
    FormatImporter copier;
    copier.moveFolder(Global::savesFolder(), safetyPath);

    // Add the README file for user to cancel a bad restoration:
    QString readmePath = safetyPath + i18n("README.txt");
    QFile file(readmePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << i18n("This is a safety copy of your baskets like they were before you started to restore the backup %1.",
                       QUrl::fromLocalFile(path).fileName())
                + QStringLiteral("\n\n")
               << i18n("If the restoration was a success and you restored what you wanted to restore, you can remove this folder.") + QStringLiteral("\n\n")
               << i18n("If something went wrong during the restoration process, you can re-use this folder to store your baskets and nothing will be lost.")
                + QStringLiteral("\n\n")
               << i18n("Choose \"Basket\" -> \"Backup & Restore...\" -> \"Use Another Existing Folder...\" and select that folder.") + QLatin1Char('\n');
        file.close();
    }

    QString message = QStringLiteral("<p><nobr>") + i18n("Restoring <b>%1</b>. Please wait...", QUrl::fromLocalFile(path).fileName())
        + QStringLiteral("</nobr></p><p>") + i18n("If something goes wrong during the restoration process, read the file <b>%1</b>.", readmePath);

    auto *dialog = new QProgressDialog();
    dialog->setWindowTitle(i18n("Restore Baskets"));
    dialog->setLabelText(message);
    dialog->setModal(/*modal=*/true);
    dialog->setCancelButton(nullptr);
    dialog->setAutoClose(true);

    dialog->setRange(0, 0 /*Busy/Undefined*/);
    dialog->setValue(0);
    dialog->show();

    // Uncompress:
    RestoreThread thread(path, Global::savesFolder());
    thread.start();
    while (thread.isRunning()) {
        dialog->setValue(dialog->value() + 1); // Or else, the animation is not played!
        qApp->processEvents();
        usleep(300); // Not too long because if the restore process is finished, we wait for nothing
    }

    dialog->hide(); // The restore is finished, do not continue to show it while telling the user the application is going to be restarted
    delete dialog; // If we only hidden it, it reappeared just after having restored a small backup... Very strange.
    dialog = nullptr; // This was annoying since it is modal and the "BasKet Note Pads is going to be restarted" message was not reachable.
    // qApp->processEvents();

    // Check for errors:
    if (!thread.success()) {
        // Restore the old baskets:
        QDir dir;
        dir.remove(readmePath);
        copier.moveFolder(safetyPath, Global::savesFolder());
        // Tell the user:
        KMessageBox::error(
            nullptr,
            i18n("This archive is either not a backup of baskets or is corrupted. It cannot be imported. Your old baskets have been preserved instead."),
            i18n("Restore Error"));
        return;
    }

    // Note: The safety backup is not removed now because the code can has been wrong, somehow, or the user perhaps restored an older backup by error...
    //       The restore process will not be called very often (it is possible it will only be called once or twice around the world during the next years).
    //       So it is rare enough to force the user to remove the safety folder, but keep him in control and let him safely recover from restoration errors.

    Backup::setFolderAndRestart(Global::savesFolder() /*No change*/,
                                i18n("Your backup has been successfully restored to <b>%1</b>. %2 is going to be restarted to take this change into account."));
}

/** class Backup: */

QString Backup::binaryPath;

void Backup::figureOutBinaryPath(const char *argv0, QApplication &app)
{
    /*
       The application can be launched by two ways:
       - Globally (app.applicationFilePath() is good)
       - In KDevelop or with an absolute path (app.applicationFilePath() is wrong)
       This function is called at the very start of main() so that the current directory has not been changed yet.

       Command line (argv[0])   QDir(argv[0]).canonicalPath()                   app.applicationFilePath()
       ======================   =============================================   =========================
       "basket"                 ""                                              "/opt/kde3/bin/basket"
       "./src/.libs/basket"     "/home/seb/prog/basket/debug/src/.lib/basket"   "/opt/kde3/bin/basket"
    */

    binaryPath = QDir(QString::fromUtf8(argv0)).canonicalPath();
    if (binaryPath.isEmpty())
        binaryPath = app.applicationFilePath();
}

void Backup::setFolderAndRestart(const QString &folder, const QString &message)
{
    // Set the folder:
    Settings::setDataFolder(folder);
    Settings::saveConfig();

    // Reassure the user that the application main window disappearance is not a crash, but a normal restart.
    // This is important for users to trust the application in such a critical phase and understands what's happening:
    KMessageBox::information(
        nullptr,
        QStringLiteral("<qt>")
            + message.arg((folder.endsWith(QLatin1Char('/')) ? folder.left(folder.length() - 1) : folder), QGuiApplication::applicationDisplayName()),
        i18n("Restart"));

    // Restart the application:
    auto *job = new KIO::CommandLauncherJob(binaryPath);

    job->setExecutable(QCoreApplication::applicationName());
    job->setDesktopName(QStringLiteral("org.kde.basket"));
    job->start();

    exit(0);
}

QString Backup::newSafetyFolder()
{
    QDir dir;
    QString fullPath;

    fullPath = QDir::homePath() + QLatin1Char('/') + i18nc("Safety folder name before restoring a basket data archive", "Baskets Before Restoration")
        + QLatin1Char('/');
    if (!dir.exists(fullPath))
        return fullPath;

    for (int i = 2;; ++i) {
        fullPath = QDir::homePath() + QLatin1Char('/')
            + i18nc("Safety folder name before restoring a basket data archive", "Baskets Before Restoration (%1)", i) + QLatin1Char('/');
        if (!dir.exists(fullPath))
            return fullPath;
    }

    return {};
}

/** class BackupThread: */

BackupThread::BackupThread(const QString &tarFile, const QString &folderToBackup)
    : m_tarFile(tarFile)
    , m_folderToBackup(folderToBackup)
{
}

void BackupThread::run()
{
    KTar tar(m_tarFile, QStringLiteral("application/x-gzip"));
    tar.open(QIODevice::WriteOnly);
    tar.addLocalDirectory(m_folderToBackup, backupMagicFolder);
    // KArchive does not add hidden files. Basket description files (".basket") are hidden, we add them manually:
    QDir dir(m_folderToBackup + QStringLiteral("baskets/"));
    QStringList baskets = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (QStringList::Iterator it = baskets.begin(); it != baskets.end(); ++it) {
        tar.addLocalFile(m_folderToBackup + QStringLiteral("baskets/") + *it + QStringLiteral("/.basket"),
                         backupMagicFolder + QStringLiteral("/baskets/") + *it + QStringLiteral("/.basket"));
    }
    // We finished:
    tar.close();
}

/** class RestoreThread: */

RestoreThread::RestoreThread(const QString &tarFile, const QString &destFolder)
    : m_tarFile(tarFile)
    , m_destFolder(destFolder)
{
}

void RestoreThread::run()
{
    m_success = false;
    KTar tar(m_tarFile, QStringLiteral("application/x-gzip"));
    tar.open(QIODevice::ReadOnly);
    if (tar.isOpen()) {
        const KArchiveDirectory *directory = tar.directory();
        if (directory->entries().contains(backupMagicFolder)) {
            const KArchiveEntry *entry = directory->entry(backupMagicFolder);
            if (entry->isDirectory()) {
                ((const KArchiveDirectory *)entry)->copyTo(m_destFolder);
                m_success = true;
            }
        }
        tar.close();
    }
}

#include "moc_backup.cpp"
