/**
 * SPDX-FileCopyrightText: (C) 2014 Narfinger <Narfinger@users.noreply.github.com>
 * SPDX-FileCopyrightText: (C) 2014 Gleb Baryshev <gleb.baryshev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QMutexLocker>

#include "basketscene.h"
#include "gitwrapper.h"
#include "settings.h"

#ifdef WITH_LIBGIT2

extern "C" {
#include <git2.h>
}

#include "global.h"

#define GIT_RETURN_IF_DISABLED()                                                                                                                                                                                                               \
    if (!Settings::versionSyncEnabled())                                                                                                                                                                                                       \
        return;

QMutex GitWrapper::gitMutex;

void GitWrapper::initializeGitRepository(QString folder)
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);
    // this is not thread safe, we use locking elsewhere
    git_repository *repo = nullptr;
    QByteArray ba = folder.toUtf8();

    const char *cString = ba.data();

    int error = git_repository_init(&repo, cString, false);
    if (error < 0) {
        const git_error *e = giterr_last();
        qDebug() << e->message;
    }

    git_signature *sig = nullptr;
    git_index *index = nullptr;
    git_oid tree_id;
    git_oid commit_id;
    git_tree *tree = nullptr;

    // no error handling at the moment
    git_signature_now(&sig, "AutoGit", "auto@localhost");
    git_repository_index(&index, repo);
    git_index_write_tree(&tree_id, index);
    git_tree_lookup(&tree, repo, &tree_id);
    git_commit_create_v(&commit_id, repo, "HEAD", sig, sig, nullptr, "Initial commit", tree, 0);

    git_signature_free(sig);
    git_index_free(index);
    git_tree_free(tree);

    // first commit
    commitPattern(repo, QStringLiteral("*"), QStringLiteral("Initial full commit"));
    git_repository_free(repo);
}

void GitWrapper::commitBasketView()
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);

    git_repository *repo = openRepository();
    if (repo == nullptr)
        return;

    const QDateTime gitdate = getLastCommitDate(repo);

    const QString pathtosave = Global::savesFolder();
    QDir basketdir(pathtosave + QStringLiteral("baskets/"));
    bool changed = false;
    basketdir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot); // this automatically skips baskets.xml file
    QDirIterator it(basketdir);
    while (!changed && it.hasNext()) {
        const QFileInfo file(it.next());
        if (file.lastModified() > gitdate)
            changed = true;
    }

    if (changed) {
        commitPattern(repo, QStringLiteral("*"));
    }

    git_repository_free(repo);
}

void GitWrapper::commitCreateBasket()
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);
    git_repository *repo = openRepository();
    if (repo == nullptr)
        return;

    const QDateTime gitdate = getLastCommitDate(repo);

    const QString basketxml = Global::savesFolder() + QStringLiteral("baskets/baskets.xml");
    const QFileInfo basketxmlinfo(basketxml);

    if (gitdate <= basketxmlinfo.lastModified()) {
        git_index *index = nullptr;
        int error = git_repository_index(&index, repo);
        if (error < 0) {
            gitErrorHandling();
            return;
        }
        // this is kind of hacky because somebody could come in between and we still have stuff open
        // change basket.xml
        const QString basketxml(QLatin1String("baskets/baskets.xml"));
        QByteArray basketxmlba = basketxml.toUtf8();
        char *basketxmlCString = basketxmlba.data();
        error = git_index_add_bypath(index, basketxmlCString);
        if (error < 0) {
            gitErrorHandling();
            return;
        }

        bool result = commitIndex(repo, index);
        git_index_free(index);
    }

    git_repository_free(repo);
}

void GitWrapper::commitTagsXml()
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);
    git_repository *repo = openRepository();
    if (repo == nullptr)
        return;

    git_index *index = nullptr;
    int error = git_repository_index(&index, repo);
    if (error < 0) {
        gitErrorHandling();
        return;
    }

    const QString tagsxml(QStringLiteral("tags.xml"));
    QByteArray tagsxmlba = tagsxml.toUtf8();
    char *tagsxmlCString = tagsxmlba.data();
    error = git_index_add_bypath(index, tagsxmlCString);

    bool result = commitIndex(repo, index);
    git_index_free(index);

    git_repository_free(repo);
}

void GitWrapper::commitDeleteBasket(QString basketFolderName)
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);

    git_index *index = nullptr;
    git_repository *repo = openRepository();
    if (repo == nullptr)
        return;

    int error = git_repository_index(&index, repo);
    if (error < 0) {
        gitErrorHandling();
        return;
    }

    // remove the directory
    const QString dir(QStringLiteral("baskets/") + basketFolderName);
    const QByteArray dirba = dir.toUtf8();
    const char *dirCString = dirba.data();
    error = git_index_remove_directory(index, dirCString, 0);
    if (error < 0) {
        gitErrorHandling();
        return;
    }

    // change basket.xml
    const QString basketxml(QStringLiteral("baskets/baskets.xml"));
    QByteArray basketxmlba = basketxml.toUtf8();
    char *basketxmlCString = basketxmlba.data();
    error = git_index_add_bypath(index, basketxmlCString);
    if (error < 0) {
        gitErrorHandling();
        return;
    }
    removeDeletedFromIndex(repo, index);

    bool result = commitIndex(repo, index);

    git_index_free(index);
    git_repository_free(repo);
}

void GitWrapper::commitBasket(BasketScene *basket)
{
    GIT_RETURN_IF_DISABLED()
    QMutexLocker l(&gitMutex);
    git_repository *repo = openRepository();
    if (repo == nullptr)
        return;

    const QDateTime gitdate = getLastCommitDate(repo);

    const QString fullpath = basket->fullPath();
    const QDir basketdir(fullpath);
    bool changed = false;
    QDirIterator it(basketdir);
    while (!changed && it.hasNext()) {
        const QFileInfo file(it.next());
        if (file.fileName() != QStringLiteral(".basket")) {
            if (file.lastModified() >= gitdate)
                changed = true;
        }
    }

    if (changed) {
        git_index *index = nullptr;

        int error = git_repository_index(&index, repo);
        if (error < 0) {
            gitErrorHandling();
            return;
        }

        const QString pattern(QStringLiteral("baskets/") + basket->folderName() + QLatin1Char('*'));

        QByteArray patternba = pattern.toUtf8();
        char *patternCString = patternba.data();
        git_strarray arr = {&patternCString, 1};
        error = git_index_add_all(index, &arr, 0, nullptr, nullptr);
        if (error < 0) {
            gitErrorHandling();
            return;
        }
        const QString basketxml(QStringLiteral("baskets/baskets.xml"));
        QByteArray basketxmlba = basketxml.toUtf8();
        char *basketxmlCString = basketxmlba.data();
        error = git_index_add_bypath(index, basketxmlCString);
        if (error < 0) {
            gitErrorHandling();
            return;
        }

        removeDeletedFromIndex(repo, index);

        bool result = commitIndex(repo, index);

        git_index_free(index);
    }

    git_repository_free(repo);
}

bool GitWrapper::commitPattern(git_repository *repo, QString pattern, QString message)
{
    git_index *index = nullptr;
    int error = git_repository_index(&index, repo);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    QByteArray patternba = pattern.toUtf8();
    char *patternCString = patternba.data();
    git_strarray arr = {&patternCString, 1};
    error = git_index_add_all(index, &arr, 0, nullptr, nullptr);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    bool result = commitIndex(repo, index, message);

    git_index_free(index);

    return true;
}

bool GitWrapper::commitIndex(git_repository *repo, git_index *index, QString message)
{
    //  write git index
    git_signature *sig = nullptr;
    git_oid tree_id;
    git_oid commit_id;
    git_tree *tree = nullptr;

    int error = git_signature_now(&sig, "AutoGit", "auto@localhost");
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    error = git_repository_index(&index, repo);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    git_commit *commit = nullptr; /* parent */
    git_oid oid_parent_commit;    /* the SHA1 for last commit */

    error = git_reference_name_to_id(&oid_parent_commit, repo, "HEAD");
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    error = git_commit_lookup(&commit, repo, &oid_parent_commit);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    error = git_index_write(index);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    error = git_index_write_tree(&tree_id, index);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    error = git_tree_lookup(&tree, repo, &tree_id);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }
    
    const git_commit *parentarray[] = {commit};
    QByteArray commitmessageba = message.toUtf8();
    const char *commitmessageCString = commitmessageba.data();
    error = git_commit_create(&commit_id, repo, "HEAD", sig, sig, nullptr, commitmessageCString, tree, 1, parentarray);
    if (error < 0) {
        gitErrorHandling();
        return false;
    }

    git_signature_free(sig);
    git_tree_free(tree);
    return true;
}

void GitWrapper::removeDeletedFromIndex(git_repository *repo, git_index *index)
{
    git_status_foreach(
        repo,
        [](const char *path, unsigned int status_flags, void *payload) -> int {
            if (status_flags & GIT_STATUS_WT_DELETED) {
                git_index *index = static_cast<git_index *>(payload);
                git_index_remove_bypath(index, path);
            }
            return 0;
        },
        index);
}

git_repository *GitWrapper::openRepository()
{
    QString pathtosave = Global::savesFolder();
    QByteArray pathba = pathtosave.toUtf8();
    const char *pathCString = pathba.data();
    git_repository *repo = nullptr;

    int error = git_repository_open(&repo, pathCString);
    if (error < 0) {
        gitErrorHandling();
        return repo;
    }
    return repo;
}

QDateTime GitWrapper::getLastCommitDate(git_repository *repo)
{
    if (repo == nullptr)
        return QDateTime();
    git_oid oid_parent_commit; /* the SHA1 for last commit */
    int error = git_reference_name_to_id(&oid_parent_commit, repo, "HEAD");
    if (error < 0)
        return QDateTime();

    git_commit *head = nullptr;
    error = git_commit_lookup(&head, repo, &oid_parent_commit);
    if (error < 0)
        return QDateTime();
    int64_t time = static_cast<int64_t>(git_commit_time(head));
    
    QDateTime date;
    QTime t(0,0,0);
    t.addSecs(time);
    date.setTime(t);

    git_commit_free(head);
    return date;
}

void GitWrapper::gitErrorHandling()
{
    const git_error *e = giterr_last();
    qDebug() << "Error in git (error,class,message)" << e->klass << e->message;
}

#else
// make everything noop
void GitWrapper::initializeGitRepository(QString folder)
{
}
void GitWrapper::commitBasketView()
{
}
void GitWrapper::commitCreateBasket()
{
}
void GitWrapper::commitDeleteBasket(QString basketFolderName)
{
}
void GitWrapper::commitBasket(BasketScene *basket)
{
}
void GitWrapper::commitTagsXml()
{
}
bool GitWrapper::commitPattern(git_repository *repo, QString pattern, QString message)
{
    return true;
}
bool GitWrapper::commitIndex(git_repository *repo, git_index *index, QString message)
{
    return true;
}
void GitWrapper::removeDeletedFromIndex(git_repository *repo, git_index *index)
{
}
git_repository *GitWrapper::openRepository()
{
    return 0;
}

QDateTime GitWrapper::getLastCommitDate(git_repository *repo)
{
    return QDateTime();
}

void GitWrapper::gitErrorHandling()
{
}

#endif
