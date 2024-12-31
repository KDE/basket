/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 * SPDX-FileCopyrightText: (C) 2020 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifndef _WIN32

#include <QColor>
#include <QObject>

/**
 * @brief This class allows to pick a color on the screen.
 *
 * Internally this class wrap the PickColor function from the
 * Screenshot xdg-portal and should work on X and wayland.
 *
 * @author Carl Schwan
 */
class ColorPicker : public QObject
{
    Q_OBJECT
public:
    explicit ColorPicker(QObject *parent = nullptr);
    virtual ~ColorPicker() override = default;

public Q_SLOTS:
    /**
     * Begin color picking.
     * This function returns immediately, and colorGrabbed() is emitted if user has
     * chosen a color, and not canceled the process (by pressing Escape).
     */
    void grabColor();

Q_SIGNALS:
    /**
     * When user picked a color, this signal is emitted.
     */
    void colorGrabbed(const QColor &color);

protected Q_SLOTS:
    void gotColorResponse(uint response, const QVariantMap &results);
};

#endif
