/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "colorpicker.h"

#ifndef _WIN32

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

#include <basket_debug.h>

QDBusArgument &operator<<(QDBusArgument &arg, const QColor &color)
{
    arg.beginStructure();
    arg << color.redF() << color.greenF() << color.blueF();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, QColor &color)
{
    double red, green, blue;
    arg.beginStructure();
    arg >> red >> green >> blue;
    color.setRedF(red);
    color.setGreenF(green);
    color.setBlueF(blue);
    arg.endStructure();

    return arg;
}

ColorPicker::ColorPicker(QObject *parent)
    : QObject(parent)
{
    setObjectName(QStringLiteral("ColorPicker"));
    qDBusRegisterMetaType<QColor>();
}

void ColorPicker::grabColor()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                          QStringLiteral("/org/freedesktop/portal/desktop"),
                                                          QStringLiteral("org.freedesktop.portal.Screenshot"),
                                                          QStringLiteral("PickColor"));
    message << QStringLiteral("x11:") << QVariantMap{};
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
                                                  SLOT(gotColorResponse(uint, QVariantMap)));
        }
    });
}

void ColorPicker::gotColorResponse(uint response, const QVariantMap &results)
{
    switch (response) {
    // 0: Success, the request is carried out
    case 0:
        if (results.contains(QStringLiteral("color"))) {
            const auto color = qdbus_cast<QColor>(results.value(QStringLiteral("color")));
            Q_EMIT colorGrabbed(color);
        }
        break;
    // 1: The user cancelled the interaction
    case 1:
        qCDebug(BASKET_LOG) << "PickColor operation cancelled by the user";
        break;
    // 2: The user interaction was ended in some other way
    // (also covering any other response code for safety)
    default:
        qCWarning(BASKET_LOG) << "PickColor failed:" << response << results;
    }
}

#include "moc_colorpicker.cpp"

#endif
