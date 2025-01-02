/**
 * SPDX-FileCopyrightText: (C) 2024 by Dominik Kummer <devel@arkades.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

class Note;

/**
 * @author Dominik Kummer
 */

class BasketAnimations : public QParallelAnimationGroup
{
    Q_OBJECT
public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    BasketAnimations(QObject *parent = nullptr)
        : QParallelAnimationGroup(parent) {};
    ~BasketAnimations() override = default;
};

class NoteAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    NoteAnimation(Note *target, const QByteArray &propertyName, QObject *parent = nullptr);
    ~NoteAnimation() override = default;

Q_SIGNALS:
    void valueChanged(const QVariant &value);
};

#endif // ANIMATION_H
