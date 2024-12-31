/**
 * SPDX-FileCopyrightText: (C) 2014 Narfinger <Narfinger@users.noreply.github.com>
 * SPDX-FileCopyrightText: (C) 2014 Gleb Baryshev <gleb.baryshev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GITWRAPPER_H
#define GITWRAPPER_H

#include <QMutex>

class git_repository;
class git_index;
class BasketScene;

/* Static class to encapsulate git operations
 *
 * the commit* operations check if the file or subfolder is newer than the git directory
 **/
class GitWrapper
{
public:
    static QMutex gitMutex;
    static void initializeGitRepository(QString folder);
    static void commitBasketView(); // commits the whole directory
    static void commitCreateBasket(); // commits and checks baskets/baskets.xml
    static void commitDeleteBasket(QString basketFolderName); // deletes a basket directory
    static void commitBasket(BasketScene *basket); // commits and checks baskets/$BASKETNAME
    static void commitTagsXml(); // commits and checks baskets.xml

private:
    static bool commitPattern(git_repository *repo, QString pattern = QStringLiteral("*"), QString message = QStringLiteral("AutoCommit"));
    static bool commitIndex(git_repository *repo, git_index *index, QString message = QString(QStringLiteral("AutoCommit")));
    static void removeDeletedFromIndex(git_repository *repo, git_index *index);
    static git_repository *openRepository();
    static QDateTime getLastCommitDate(git_repository *repo);
    static void gitErrorHandling();
};

#endif // GITWRAPPER_H
