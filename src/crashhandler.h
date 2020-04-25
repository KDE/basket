/**
 * SPDX-FileCopyrightText: (C) 2005 Max Howell <max.howell@methylblue.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CRASH_H
#define CRASH_H

#include <QString>

/**
 * @author Max Howell
 * @short The amaroK crash-handler
 *
 * I'm not entirely sure why this had to be inside a class, but it
 * wouldn't work otherwise *shrug*
 */
class Crash
{
public:
    static void crashHandler(int signal);
    static QString getOSVersionInfo();
};

#endif
