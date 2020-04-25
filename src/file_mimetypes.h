/**
 * SPDX-FileCopyrightText: (C) 2015 by Gleb Baryshev <gleb.baryshev@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILE_MIMETYPES_H
#define FILE_MIMETYPES_H

namespace MimeTypes
{
#define StrRes static const char *const

StrRes LAUNCHER = "application/x-desktop";
StrRes HTML = "text/html";
StrRes TEXT = "text/plain";

StrRes ANIMATION = "image/gif";
StrRes ANIMATION_MNG = "movie/x-mng";

StrRes IMAGE = "image/";
StrRes AUDIO = "audio/";

#undef StrRes
}

#endif // FILE_MIMETYPES_H
