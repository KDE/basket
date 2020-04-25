/**
 * SPDX-FileCopyrightText: (C) 2006 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LIKEBACK_PRIVATE_H
#define LIKEBACK_PRIVATE_H

#include "likeback.h"

#include <QDialog>

#include <QtCore/QTimer>

class QToolButton;
class KTextEdit;
class QRadioButton;
class QCheckBox;
class QGroupBox;
class QNetworkReply;

class Kaction;

class LikeBackBar;

class LikeBackPrivate
{
public:
    LikeBackPrivate();
    ~LikeBackPrivate();
    LikeBackBar *bar;
    KConfig *config;
    const KAboutData *aboutData;
    LikeBack::Button buttons;
    QString hostName;
    QString remotePath;
    quint16 hostPort;
    QStringList acceptedLocales;
    QString acceptedLanguagesMessage;
    LikeBack::WindowListing windowListing;
    bool showBarByDefault;
    bool showBar;
    int disabledCount;
    QString fetchedEmail;
    QAction *action;
};

class LikeBackBar : public QWidget
{
    Q_OBJECT
public:
    explicit LikeBackBar(LikeBack *likeBack);
    ~LikeBackBar() override;
public slots:
    void startTimer();
    void stopTimer();
private slots:
    void autoMove();
    void clickedLike();
    void clickedDislike();
    void clickedBug();
    void clickedFeature();

private:
    LikeBack *m_likeBack;
    QTimer m_timer;
    QToolButton *m_likeButton;
    QToolButton *m_dislikeButton;
    QToolButton *m_bugButton;
    QToolButton *m_featureButton;
};

class LikeBackDialog : public QDialog
{
    Q_OBJECT
public:
    LikeBackDialog(LikeBack::Button reason, const QString &initialComment, const QString &windowPath, const QString &context, LikeBack *likeBack);
    ~LikeBackDialog() override;

private:
    LikeBack *m_likeBack;
    QString m_windowPath;
    QString m_context;
    KTextEdit *m_comment;
    QRadioButton *likeButton;
    QRadioButton *dislikeButton;
    QRadioButton *bugButton;
    QRadioButton *featureButton;
    QCheckBox *m_showButtons;
    QDialogButtonBox *buttonBox;
    QString introductionText();
private slots:
    void ensurePolished();
    void slotDefault();
    void slotOk();
    void changeButtonBarVisible();
    void commentChanged();
    void send();
    void requestFinished(QNetworkReply *reply);
};

#endif // LIKEBACK_PRIVATE_H
