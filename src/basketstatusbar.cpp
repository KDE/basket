/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketstatusbar.h"

#include <QLocale>
#include <QStatusBar>

#include <QLabel>
#include <QMouseEvent>
#include <QObject>
#include <QPixmap>

#include <KIconLoader>
#include <KLocalizedString>
#include <KParts/StatusBarExtension>

#include "basketscene.h"
#include "bnpview.h"
#include "global.h"
#include "tools.h"

BasketStatusBar::BasketStatusBar(QStatusBar *bar)
    : m_bar(bar)
    , m_extension(nullptr)
    , m_selectionStatus(nullptr)
    , m_lockStatus(nullptr)
    , m_basketStatus(nullptr)
    , m_savedStatus(nullptr)
{
}

BasketStatusBar::BasketStatusBar(KParts::StatusBarExtension *extension)
    : m_bar(nullptr)
    , m_extension(extension)
    , m_selectionStatus(nullptr)
    , m_lockStatus(nullptr)
    , m_basketStatus(nullptr)
    , m_savedStatus(nullptr)
{
}

BasketStatusBar::~BasketStatusBar()
{
    // delete m_extension;
}

QStatusBar *BasketStatusBar::statusBar() const
{
    if (m_extension != nullptr) {
        return m_extension->statusBar();
    }

    return m_bar;
}

void BasketStatusBar::addWidget(QWidget *widget, int stretch, bool permanent)
{
    if (m_extension != nullptr) {
        m_extension->addStatusBarItem(widget, stretch, permanent);
    } else if (permanent) {
        m_bar->addPermanentWidget(widget, stretch);
    } else {
        m_bar->addWidget(widget, stretch);
    }
}

void BasketStatusBar::setupStatusBar()
{
    QWidget *parent = statusBar();
    QObjectList lst = parent->findChildren<QObject *>(QStringLiteral("KRSqueezedTextLabel"));

    // Tools::printChildren(parent);
    if (lst.count() == 0) {
        m_basketStatus = new QLabel(parent);
        QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        policy.setHorizontalStretch(0);
        policy.setVerticalStretch(0);
        policy.setHeightForWidth(false);
        m_basketStatus->setSizePolicy(policy);
        addWidget(m_basketStatus, 1, false); // Fit all extra space and is hiddable
    } else {
        m_basketStatus = qobject_cast<QLabel *>(lst.at(0));
    }
    lst.clear();

    m_selectionStatus = new QLabel(i18n("Loading..."), parent);
    addWidget(m_selectionStatus, 0, true);

    m_lockStatus = new QLabel(nullptr /*this*/);
    m_lockStatus->setMinimumSize(18, 18);
    m_lockStatus->setAlignment(Qt::AlignCenter);
    //  addWidget( m_lockStatus, 0, true );
    m_lockStatus->installEventFilter(this);

    m_savedStatusPixmap = QIcon::fromTheme(QStringLiteral("document-save")).pixmap(KIconLoader::SizeSmall);
    m_savedStatus = new QLabel(parent);
    m_savedStatus->setPixmap(m_savedStatusPixmap);
    m_savedStatus->setFixedSize(m_savedStatus->sizeHint());
    m_savedStatus->clear();
    // m_savedStatus->setPixmap(m_savedStatusIconSet.pixmap(QIconSet::Small, QIconSet::Disabled));
    // m_savedStatus->setEnabled(false);
    addWidget(m_savedStatus, 0, true);
    m_savedStatus->setToolTip(QStringLiteral("<p>") + i18n("Shows if there are changes that have not yet been saved."));
}

void BasketStatusBar::postStatusbarMessage(const QString &text)
{
    if (statusBar() != nullptr) {
        statusBar()->showMessage(text, 2000);
    }
}

void BasketStatusBar::setStatusText(const QString &txt)
{
    if (m_basketStatus != nullptr && m_basketStatus->text() != txt) {
        m_basketStatus->setText(txt);
    }
}

void BasketStatusBar::setStatusBarHint(const QString &hint)
{
    if (hint.isEmpty()) {
        updateStatusBarHint();
    } else {
        setStatusText(hint);
    }
}

void BasketStatusBar::updateStatusBarHint()
{
    QString message;

    if (Global::bnpView->currentBasket()->isDuringDrag()) {
        message = i18n("Ctrl+drop: copy, Shift+drop: move, Shift+Ctrl+drop: link.");
    }
    // Too much noise information:
    //  else if (currentBasket()->inserterShown() && currentBasket()->inserterSplit() && !currentBasket()->inserterGroup())
    //      message = i18n("Click to insert a note, right click for more options. Click on the right of the line to group instead of insert.");
    //  else if (currentBasket()->inserterShown() && currentBasket()->inserterSplit() && currentBasket()->inserterGroup())
    //      message = i18n("Click to group a note, right click for more options. Click on the left of the line to group instead of insert.");
    else if (Global::debugWindow != nullptr) {
        message = QStringLiteral("DEBUG: ") + Global::bnpView->currentBasket()->folderName();
    }

    setStatusText(message);
}

void BasketStatusBar::setLockStatus(bool isLocked)
{
    if (m_lockStatus == nullptr) {
        return;
    }

    if (isLocked) {
        QPixmap encryptedIcon = QIcon::fromTheme(QStringLiteral("encrypted.png")).pixmap(KIconLoader::SizeSmall);
        m_lockStatus->setPixmap(encryptedIcon);
        m_lockStatus->setToolTip(i18n("<p>This basket is <b>locked</b>.<br>Click to unlock it.</p>").replace(QLatin1Char(' '), QStringLiteral("&nbsp;")));
    } else {
        m_lockStatus->clear();
        m_lockStatus->setToolTip(i18n("<p>This basket is <b>unlocked</b>.<br>Click to lock it.</p>").replace(QLatin1Char(' '), QStringLiteral("&nbsp;")));
    }
}

void BasketStatusBar::setSelectionStatus(const QString &s)
{
    if (m_selectionStatus != nullptr) {
        m_selectionStatus->setText(s);
    }
}

void BasketStatusBar::setUnsavedStatus(bool isUnsaved)
{
    if (m_savedStatus == nullptr) {
        return;
    }

    if (isUnsaved) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        if (m_savedStatus->pixmap(Qt::ReturnByValueConstant::ReturnByValue).isNull()) {
#else
        if (m_savedStatus->pixmap()->isNull()) {
#endif
            m_savedStatus->setPixmap(m_savedStatusPixmap);
        }
    } else {
        m_savedStatus->clear();
    }
}

bool BasketStatusBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_lockStatus && event->type() == QEvent::MouseButtonPress) {
        auto *mevent = dynamic_cast<QMouseEvent *>(event);
        if (static_cast<bool>(mevent->button() & Qt::LeftButton)) {
            Global::bnpView->lockBasket();
            return true;
        }
    }
    return QObject::eventFilter(obj, event); // standard event processing
}

#include "moc_basketstatusbar.cpp"
