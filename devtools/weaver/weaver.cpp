/**
 * SPDX-FileCopyrightText: 2021 Sebastian Engel <kde@sebastianengel.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "weaver.h"


// TODO include libBasket instead of hardcoded link
#include "../../src/archive.h"

#include <KLocalizedString>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QXmlStreamReader>

void translateErrorCode(const Archive::IOErrorCode code);

Weaver::Weaver(QCommandLineParser *parser,
               const QCommandLineOption &mode_weave,
               const QCommandLineOption &mode_unweave,
               const QCommandLineOption &output,
               const QCommandLineOption &basename,
               const QCommandLineOption &previewImg,
               const QCommandLineOption &force)
    : m_parser(parser)
    , m_weave(mode_weave)
    , m_unweave(mode_unweave)
    , m_output(output)
    , m_basename(basename)
    , m_previewImg(previewImg)
    , m_force(force)
    , m_preview(QString())
{
}

int Weaver::runMain()
{
    if (!m_parser->isSet(m_unweave) && !m_parser->isSet(m_weave)) {
        qCritical().noquote() << i18n("You need to provide at least one --weave/-w or --unweave/-w option");
        return 1;
    }

    if (m_parser->isSet(m_unweave) && m_parser->isSet(m_weave)) {
        qCritical().noquote() << i18n("You cannot use --weave/-w and --unweave/-u options in conjunction.");
        return 1;
    }

    if (m_parser->values(m_unweave).size() > 1 || m_parser->values(m_weave).size() > 1) {
        qWarning().noquote() << i18n(
            "Multiple --weave/-w or --unweave/-u input options found. All but the first input"
            " are going to be ignored.");
    }

    bool ret = true;

    if (m_parser->isSet(m_weave)) {
        ret = weave();
    } else if (m_parser->isSet(m_unweave)) {
        ret = unweave();
    }

    if (!ret) {
        return 1;
    }

    return 0;
}

bool Weaver::unweave()
{
    m_in = m_parser->value(m_unweave);

    if (!isBasketFile(m_in)) {
        qCritical().noquote() << i18n("The source seems to be an invalid .baskets file");
        return false;
    }

    if (m_parser->isSet(m_output)) {
        m_out = m_parser->value(m_output);
        if (!QFileInfo::exists(m_out)) {
            qCritical().noquote() << i18n("Output directory does not exist.");
            return false;
        }
    } else {
        m_out = QFileInfo(m_in).absoluteDir().path();
    }

    QString destination = m_out + QDir::separator();
    if (m_parser->isSet(m_basename)) {
        destination += m_parser->value(m_basename);
    } else {
        destination += QFileInfo(m_in).baseName() + "_baskets";
    }

    Archive::IOErrorCode errorCode = Archive::extractArchive(m_in, destination, !m_parser->isSet(m_force));

    translateErrorCode(errorCode);

    return errorCode == Archive::IOErrorCode::NoError;
}

bool Weaver::weave()
{
    m_in = m_parser->value(m_weave);
    m_preview = m_parser->value(m_previewImg);

    if (!isBasketSourceValid(m_in)) {
        qCritical().noquote() << i18n("The source seems to be invalid.");
        if (!m_parser->isSet(m_force)) {
            return false;
        }
    }

    m_preview = m_parser->value(m_previewImg);
    if (!isPreviewValid(m_preview)) {
        m_preview = QString();
    }

    if (m_parser->isSet(m_output)) {
        m_out = m_parser->value(m_output);
        if (!QFileInfo::exists(m_out)) {
            qCritical().noquote() << i18n("Output directory does not exist.");
            return false;
        }
    } else {
        QDir inputPath(m_in);
        inputPath.cdUp();
        m_out = inputPath.absolutePath();
    }

    QString destination = m_out + QDir::separator();
    if (m_parser->isSet(m_basename)) {
        destination += m_parser->value(m_basename);
    } else {
        destination += QFileInfo(m_in).baseName();
    }

    const QString extenstion = QStringLiteral(".baskets");
    if (!destination.endsWith(extenstion)) {
        destination += extenstion;
    }

    Archive::IOErrorCode errorCode = Archive::createArchiveFromSource(m_in, m_preview, destination, !m_parser->isSet(m_force));

    translateErrorCode(errorCode);

    return errorCode == Archive::IOErrorCode::NoError;
}

bool Weaver::isBasketSourceValid(const QString &basketsDirectory)
{
    QFileInfo dirInfo(basketsDirectory);
    if (!dirInfo.isDir() || !dirInfo.exists()) {
        return false;
    }

    // test the existence of /baskets/baskets.xml
    const QString basketTreePath = basketsDirectory + QDir::separator() + "baskets" + QDir::separator() + "baskets.xml";
    if (!QFileInfo::exists(basketTreePath)) {
        return false;
    }

    QStringList containedBaskets;
    QFile basketTree(basketTreePath);
    if (basketTree.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(&basketTree);

        // find the beginning of the basketTree element
        while (!xml.atEnd()) {
            xml.readNextStartElement();
            if (xml.name() == QStringLiteral("basketTree")) {
                break;
            }
        }
        if (xml.atEnd()) {
            basketTree.close();
            return false;
        }
        // collect all referenced baskets
        while (!xml.atEnd()) {
            xml.readNextStartElement();
            if (xml.name() == QStringLiteral("basket")) {
                if (xml.attributes().hasAttribute(QStringLiteral("folderName"))) {
                    containedBaskets.append(xml.attributes().value(QStringLiteral("folderName")).toString());
                    xml.skipCurrentElement();
                } else {
                    basketTree.close();
                    return false;
                }
            } else {
                break;
            }
        }
    }
    basketTree.close();

    // test whether the referenced subdirectories exist
    for (const QString &bskt : containedBaskets) {
        const QString bsktPath = basketsDirectory + QDir::separator() + "baskets" + QDir::separator() + bskt;
        if (!QFileInfo::exists(bsktPath)) {
            return false;
        }
    }

    return true;
}

bool Weaver::isBasketFile(const QString &basketsFile)
{
    QFile file(basketsFile);

    if (!file.exists()) {
        return false;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString line = stream.readLine();
        stream.setCodec("ISO-8859-1");
        if (line != QStringLiteral("BasKetNP:archive")) {
            file.close();
            return false;
        }

        while (!stream.atEnd()) {
            line = stream.readLine();
            int index = line.indexOf(':');
            QString key;
            QString value;
            if (index >= 0) {
                key = line.left(index);
                value = line.right(line.length() - index - 1);
            } else {
                key = line;
                value = QString();
            }
            // only test existence of keywords
            if (key == QStringLiteral("version") || key == QStringLiteral("read-compatible") ||
                key == QStringLiteral("write-compatible")) {
                continue;
            }
            // test for existence, then skip block of given size
            if (key == QStringLiteral("preview*")) {
                bool ok = false;
                const qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    return false;
                }
                stream.seek(stream.pos() + size);
            }
            // test for existence, then skip block of given size
            else if (key == QStringLiteral("archive*")) {
                bool ok = false;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    return false;
                }
                stream.seek(stream.pos() + size);
            }
            // test unknown embedded file, then skip block of given size
            else if (key.endsWith('*')) {
                bool ok = false;
                qint64 size = value.toULong(&ok);
                if (!ok) {
                    file.close();
                    return false;
                }
                stream.seek(stream.pos() + size);
            }
        }
        file.close();

    } else {
        qCritical().noquote() << i18n("Could not open file.");
        return false;
    }

    return true;
}

bool Weaver::isPreviewValid(const QString &previewFile)
{
    if (previewFile.isEmpty() || !QFileInfo::exists(previewFile)) {
        return false;
    }

    // is the given file a png?
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(previewFile);

    return mime.inherits(QStringLiteral("image/png"));
}

void translateErrorCode(const Archive::IOErrorCode code)
{
    switch (code) {
    case Archive::IOErrorCode::FailedToOpenResource:
        qCritical().noquote() << i18n("Failed to open a file resource.");
        break;
    case Archive::IOErrorCode::NotABasketArchive:
        qCritical().noquote() << i18n("This file is not a basket archive.");
        break;
    case Archive::IOErrorCode::CorruptedBasketArchive:
        qCritical().noquote() << i18n("This file is corrupted. It can not be opened.");
        break;
    case Archive::IOErrorCode::DestinationExists:
        qCritical().noquote() << i18n("The destination path already exists.");
        break;
    case Archive::IOErrorCode::IncompatibleBasketVersion:
        qCritical().noquote() << i18n("This file supplied file format is not supported");
        break;
    case Archive::IOErrorCode::PossiblyCompatibleBasketVersion:
        qWarning().noquote() << i18n("This file was created with a more recent version of BasKet Note Pads. It might not be fully supported");
        [[fallthrough]];
    case Archive::IOErrorCode::NoError:
        break;
    }
}
