/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "colorpicker.h"

#ifndef _WIN32

#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusConnection>
#include <QDebug>

QDBusArgument &operator <<(QDBusArgument &arg, const QColor &color)
{
    arg.beginStructure();
    arg << color.redF() << color.greenF() << color.blueF();
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator >>(const QDBusArgument &arg, QColor &color)
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
    QDBusMessage message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.portal.Desktop"),
                                                          QLatin1String("/org/freedesktop/portal/desktop"),
                                                          QLatin1String("org.freedesktop.portal.Screenshot"),
                                                          QLatin1String("PickColor"));
    message << QLatin1String("x11:") << QVariantMap{};
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this] (QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qWarning() << "Couldn't get reply";
            qWarning() << "Error: " << reply.error().message();
        } else {
            QDBusConnection::sessionBus().connect(QString(),
                                                  reply.value().path(),
                                                  QLatin1String("org.freedesktop.portal.Request"),
                                                  QLatin1String("Response"),
                                                  this,
                                                  SLOT(gotColorResponse(uint, QVariantMap)));
        }
    });
}

void ColorPicker::gotColorResponse(uint response, const QVariantMap& results)
{
    if (!response) {
        if (results.contains(QLatin1String("color"))) {
            const QColor color = qdbus_cast<QColor>(results.value(QLatin1String("color")));
            Q_EMIT colorGrabbed(color);
        }
    } else {
        qWarning() << "Failed to take screenshot";
    }
}

#include "moc_colorpicker.cpp"

#endif
