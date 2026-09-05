/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 * SPDX-FileCopyrightText: (C) 2026 Pino Toscano <pino@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifndef _WIN32

#include <QObject>
#include <QPixmap>

/**
 * @brief This class allows to grab a region of the screen.
 *
 * Internally this class wrap the Screenshot function from the
 * Screenshot xdg-portal and should work on X and wayland.
 */
class RegionGrabber : public QObject
{
    Q_OBJECT
public:
    explicit RegionGrabber(QObject *parent = nullptr);
    ~RegionGrabber() override = default;

public Q_SLOTS:
    /**
     * Begin screenshot grabbing.
     * This function returns immediately, and regionGrabbed() is emitted if user
     * selected a region to grab, and not canceled the process (by pressing Escape).
     */
    void grabRegion();

Q_SIGNALS:
    /**
     * When user picked a color, this signal is emitted.
     */
    void regionGrabbed(const QPixmap &pixmap);

protected Q_SLOTS:
    void gotScreenshotResponse(uint response, const QVariantMap &results);
};

#endif
