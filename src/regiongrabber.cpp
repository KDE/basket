/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 * SPDX-FileCopyrightText: (C) 2026 Pino Toscano <pino@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "regiongrabber.h"

#ifndef _WIN32

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QFile>
#include <QUrl>

#include <basket_debug.h>

RegionGrabber::RegionGrabber(QObject *parent)
    : QObject(parent)
{
    setObjectName(QStringLiteral("RegionGrabber"));
}

void RegionGrabber::grabRegion()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                                          QStringLiteral("org.freedesktop.portal.Screenshot"),
                                                          QStringLiteral("Screenshot"));
    const QVariantMap options{
        {QStringLiteral("interactive"), true},
    };
    message << QString() << options;
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    auto *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qCWarning(BASKET_LOG) << "Couldn't get reply";
            qCWarning(BASKET_LOG) << "Error: " << reply.error().message();
        } else {
            QDBusConnection::sessionBus().connect(QString(),
                                                  reply.value().path(),
                                                  QStringLiteral("org.freedesktop.portal.Request"),
                                                  QStringLiteral("Response"),
                                                  this,
                                                  SLOT(gotScreenshotResponse(uint, QVariantMap)));
        }
    });
}

void RegionGrabber::gotScreenshotResponse(uint response, const QVariantMap &results)
{
    qCDebug(BASKET_LOG) << "Screenshot response:" << response << results;
    switch (response) {
    // 0: Success, the request is carried out
    case 0:
        if (results.contains(QStringLiteral("uri"))) {
            const QString uri = results.value(QStringLiteral("uri")).toString();
            const QUrl url(uri);
            if (!url.isLocalFile()) {
                qCWarning(BASKET_LOG) << "Screenshot operation returned a non-local file" << uri;
                return;
            }
            QPixmap pixmap(url.toLocalFile());
            if (pixmap.isNull()) {
                qCWarning(BASKET_LOG) << "Screenshot operation returned an invalid image" << uri;
                return;
            }
            Q_EMIT regionGrabbed(pixmap);
            // Remove the local file, otherwise they pile up without the user knowing
            QFile::remove(url.toLocalFile());
        }
        break;
    // 1: The user cancelled the interaction
    case 1:
        qCDebug(BASKET_LOG) << "Screenshot operation cancelled by the user";
        break;
    // 2: The user interaction was ended in some other way
    // (also covering any other response code for safety)
    default:
        qCWarning(BASKET_LOG) << "Screenshot failed:" << response << results;
        break;
    }
}

#include "moc_regiongrabber.cpp"

#endif
