/**
 * SPDX-FileCopyrightText: (C) 2024 Dominik Kummer <devel@arkades.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation.h"
#include "note.h"

NoteAnimation::NoteAnimation(Note *target, const QByteArray &propertyName, QObject *parent)
    : QPropertyAnimation(qobject_cast<QObject *>(target), propertyName, parent)
{
}
