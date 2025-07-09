/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "backgroundmanager.h"

#include <KConfig>
#include <KConfigGroup>

#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>

/** class BackgroundEntry: */

BackgroundEntry::BackgroundEntry(const QString &location)
{
    this->location = location;
    name = QUrl::fromLocalFile(location).fileName();
    tiled = false;
    pixmap = nullptr;
    preview = nullptr;
    customersCount = 0;
}

BackgroundEntry::~BackgroundEntry()
{
    delete pixmap;
    delete preview;
}

/** class OpaqueBackgroundEntry: */

OpaqueBackgroundEntry::OpaqueBackgroundEntry(const QString &name, const QColor &color)
{
    this->name = name;
    this->color = color;
    pixmap = nullptr;
    customersCount = 0;
}

OpaqueBackgroundEntry::~OpaqueBackgroundEntry()
{
    delete pixmap;
}

/** class BackgroundManager: */

BackgroundManager::BackgroundManager()
{
    /// qDebug() << "BackgroundManager: Found the following background images in  ";
    QStringList directories = QStandardPaths::standardLocations(
        QStandardPaths::GenericDataLocation) /* WARNING: no more trailing slashes */; // eg. { "/home/seb/.kde/share/apps/", "/usr/share/apps/" }
    // For each folder:
    for (QStringList::Iterator it = directories.begin(); it != directories.end(); ++it) {
        // For each file in those directories:
        QDir dir(*it + QStringLiteral("basket/backgrounds/"),
                 /*nameFilder=*/QStringLiteral("*.png"),
                 /*sortSpec=*/QDir::Name | QDir::IgnoreCase,
                 /*filterSpec=*/QDir::Files | QDir::NoSymLinks);
        ///     qDebug() << *it + "basket/backgrounds/  ";
        QStringList files = dir.entryList();
        for (QStringList::Iterator it2 = files.begin(); it2 != files.end(); ++it2) // TODO: If an image name is present in two folders?
            addImage(*it + QStringLiteral("basket/backgrounds/") + *it2);
    }

    /// qDebug() << ":";
    /// for (BackgroundsList::Iterator it = m_backgroundsList.begin(); it != m_backgroundsList.end(); ++it)
    ///     qDebug() << "* " << (*it)->location << "  [ref: " << (*it)->name << "]";

    connect(&m_garbageTimer, &QTimer::timeout, this, [this]() {
        doGarbage();
    });
}

BackgroundManager::~BackgroundManager()
{
    qDeleteAll(m_backgroundsList);
    qDeleteAll(m_opaqueBackgroundsList);
}

void BackgroundManager::addImage(const QString &fullPath)
{
    m_backgroundsList.append(new BackgroundEntry(fullPath));
}

BackgroundEntry *BackgroundManager::backgroundEntryFor(const QString &image)
{
    for (BackgroundsList::Iterator it = m_backgroundsList.begin(); it != m_backgroundsList.end(); ++it)
        if ((*it)->name == image)
            return *it;
    return nullptr;
}

OpaqueBackgroundEntry *BackgroundManager::opaqueBackgroundEntryFor(const QString &image, const QColor &color)
{
    for (OpaqueBackgroundsList::Iterator it = m_opaqueBackgroundsList.begin(); it != m_opaqueBackgroundsList.end(); ++it)
        if ((*it)->name == image && (*it)->color == color)
            return *it;
    return nullptr;
}

bool BackgroundManager::subscribe(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);
    if (entry) {
        // If it's the first time something subscribe to this image:
        if (!entry->pixmap) {
            // Try to load the pixmap:
            entry->pixmap = new QPixmap(entry->location);
            // Try to figure out if it's a tiled background image or not (default to NO):
            KConfig config(entry->location + QStringLiteral(".config"), KConfig::SimpleConfig);
            KConfigGroup configGroup = config.group(QStringLiteral("BasKet Background Image Configuration"));
            entry->tiled = configGroup.readEntry("tiled", false);
        }
        // Return if the image loading has failed:
        if (entry->pixmap->isNull()) {
            ///         qDebug() << "BackgroundManager: Failed to load " << entry->location;
            return false;
        }
        // Success: effectively subscribe:
        ++entry->customersCount;
        return true;
    } else {
        // Don't exist: subscription failed:
        ///     qDebug() << "BackgroundManager: Requested unexisting image: " << image;
        return false;
    }
}

bool BackgroundManager::subscribe(const QString &image, const QColor &color)
{
    BackgroundEntry *backgroundEntry = backgroundEntryFor(image);

    // First, if the image doesn't exist, isn't subscribed, or failed to load then we don't go further:
    if (!backgroundEntry || !backgroundEntry->pixmap || backgroundEntry->pixmap->isNull()) {
        ///     qDebug() << "BackgroundManager: Requested an unexisting or unsubscribed image: (" << image << "," << color.name() << ")...";
        return false;
    }

    OpaqueBackgroundEntry *opaqueBackgroundEntry = opaqueBackgroundEntryFor(image, color);

    // If this couple is requested for the first time or it haven't been subscribed for a long time enough, create it:
    if (!opaqueBackgroundEntry) {
        ///     qDebug() << "BackgroundManager: Computing (" << image << "," << color.name() << ")...";
        opaqueBackgroundEntry = new OpaqueBackgroundEntry(image, color);
        opaqueBackgroundEntry->pixmap = new QPixmap(backgroundEntry->pixmap->size());
        opaqueBackgroundEntry->pixmap->fill(color);
        QPainter painter(opaqueBackgroundEntry->pixmap);
        painter.drawPixmap(0, 0, *(backgroundEntry->pixmap));
        painter.end();
        m_opaqueBackgroundsList.append(opaqueBackgroundEntry);
    }

    // We are now sure the entry exist, do the subscription:
    ++opaqueBackgroundEntry->customersCount;
    return true;
}

void BackgroundManager::unsubscribe(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);

    if (!entry) {
        ///     qDebug() << "BackgroundManager: Wanted to unsubscribe a not subscribed image: " << image;
        return;
    }

    --entry->customersCount;
    if (entry->customersCount <= 0)
        requestDelayedGarbage();
}

void BackgroundManager::unsubscribe(const QString &image, const QColor &color)
{
    OpaqueBackgroundEntry *entry = opaqueBackgroundEntryFor(image, color);

    if (!entry) {
        ///     qDebug() << "BackgroundManager: Wanted to unsubscribe a not subscribed colored image: (" << image << "," << color.name() << ")";
        return;
    }

    --entry->customersCount;
    if (entry->customersCount <= 0)
        requestDelayedGarbage();
}

QPixmap *BackgroundManager::pixmap(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);

    if (!entry || !entry->pixmap || entry->pixmap->isNull()) {
        ///     qDebug() << "BackgroundManager: Requested an unexisting or unsubscribed image: " << image;
        return nullptr;
    }

    return entry->pixmap;
}

QPixmap *BackgroundManager::opaquePixmap(const QString &image, const QColor &color)
{
    OpaqueBackgroundEntry *entry = opaqueBackgroundEntryFor(image, color);

    if (!entry || !entry->pixmap || entry->pixmap->isNull()) {
        ///     qDebug() << "BackgroundManager: Requested an unexisting or unsubscribed colored image: (" << image << "," << color.name() << ")";
        return nullptr;
    }

    return entry->pixmap;
}

bool BackgroundManager::tiled(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);

    if (!entry || !entry->pixmap || entry->pixmap->isNull()) {
        ///     qDebug() << "BackgroundManager: Requested an unexisting or unsubscribed image: " << image;
        return false;
    }

    return entry->tiled;
}

bool BackgroundManager::exists(const QString &image)
{
    for (BackgroundsList::iterator it = m_backgroundsList.begin(); it != m_backgroundsList.end(); ++it)
        if ((*it)->name == image)
            return true;
    return false;
}

QStringList BackgroundManager::imageNames()
{
    QStringList list;
    for (BackgroundsList::iterator it = m_backgroundsList.begin(); it != m_backgroundsList.end(); ++it)
        list.append((*it)->name);
    return list;
}

QPixmap *BackgroundManager::preview(const QString &image)
{
    static const int MAX_WIDTH = 100;
    static const int MAX_HEIGHT = 75;
    static const QColor PREVIEW_BG = Qt::white;

    BackgroundEntry *entry = backgroundEntryFor(image);

    if (!entry) {
        ///     qDebug() << "BackgroundManager: Requested the preview of an unexisting image: " << image;
        return nullptr;
    }

    // The easiest way: already computed:
    if (entry->preview)
        return entry->preview;

    // Then, try to load the preview from file:
    QString previewPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("basket/backgrounds/previews/") + entry->name);
    auto *previewPixmap = new QPixmap(previewPath);
    // Success:
    if (!previewPixmap->isNull()) {
        ///     qDebug() << "BackgroundManager: Loaded image preview for " << entry->location << " from file " << previewPath;
        entry->preview = previewPixmap;
        return entry->preview;
    }

    // We failed? Then construct it:
    // Note: if a preview is requested, it's because the user is currently choosing an image.
    // Since we need that image to create the preview, we keep the image in memory.
    // Then, it will already be loaded when user press [OK] in the background image chooser.
    // BUT we also delay a garbage because we don't want EVERY images to be loaded if the user use only a few of them, of course:

    // Already used? Good: we don't have to load it...
    if (!entry->pixmap) {
        // Note: it's a code duplication from BackgroundManager::subscribe(const QString &image),
        // Because, as we are loading the pixmap we ALSO need to know if it's a tile or not, in case that image will soon be used (and not destroyed by the
        // garbager):
        entry->pixmap = new QPixmap(entry->location);
        // Try to figure out if it's a tiled background image or not (default to NO):
        KConfig config(entry->location + QStringLiteral(".config"));
        KConfigGroup configGroup = config.group(QStringLiteral("BasKet Background Image Configuration"));
        entry->tiled = configGroup.readEntry("tiled", false);
    }

    // The image cannot be loaded, we failed:
    if (entry->pixmap->isNull())
        return nullptr;

    // Good that we are still alive: entry->pixmap contains the pixmap to rescale down for the preview:
    // Compute new size:
    int width = entry->pixmap->width();
    int height = entry->pixmap->height();
    if (width > MAX_WIDTH) {
        height = height * MAX_WIDTH / width;
        width = MAX_WIDTH;
    }
    if (height > MAX_HEIGHT) {
        width = width * MAX_HEIGHT / height;
        height = MAX_HEIGHT;
    }
    // And create the resulting pixmap:
    auto *result = new QPixmap(width, height);
    result->fill(PREVIEW_BG);
    QImage imageToScale = entry->pixmap->toImage();
    QPixmap pmScaled = QPixmap::fromImage(imageToScale.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    QPainter painter(result);
    painter.drawPixmap(0, 0, pmScaled);
    painter.end();

    // Saving it to file for later:
    QString folder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/basket/backgrounds/previews/");
    result->save(folder + entry->name, "PNG");

    // Ouf! That's done:
    entry->preview = result;
    requestDelayedGarbage();
    return entry->preview;
}

QString BackgroundManager::pathForImageName(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);
    if (entry == nullptr) {
        return {};
    } else
        return entry->location;
}

QString BackgroundManager::previewPathForImageName(const QString &image)
{
    BackgroundEntry *entry = backgroundEntryFor(image);
    if (entry == nullptr) {
        return {};
    } else {
        QString previewPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("basket/backgrounds/previews/") + entry->name);
        QDir dir;
        if (!dir.exists(previewPath))
            return {};
        else
            return previewPath;
    }
}

void BackgroundManager::requestDelayedGarbage()
{
    static const int DELAY = 60 /*seconds*/;

    if (!m_garbageTimer.isActive()) {
        m_garbageTimer.setSingleShot(true);
        m_garbageTimer.start(DELAY * 1000 /*ms*/);
    }
}

void BackgroundManager::doGarbage()
{
    /// qDebug() << "BackgroundManager: Doing garbage...";

    /// qDebug() << "BackgroundManager: Images:";
    for (BackgroundsList::Iterator it = m_backgroundsList.begin(); it != m_backgroundsList.end(); ++it) {
        BackgroundEntry *entry = *it;
        ///     qDebug() << "* " << entry->name << ": used " << entry->customersCount << " times";
        if (entry->customersCount <= 0 && entry->pixmap) {
            ///         qDebug() << " [Deleted cached pixmap]";
            delete entry->pixmap;
            entry->pixmap = nullptr;
        }
        ///     qDebug();
    }

    /// qDebug() << "BackgroundManager: Opaque Cached Images:";
    for (OpaqueBackgroundsList::Iterator it = m_opaqueBackgroundsList.begin(); it != m_opaqueBackgroundsList.end();) {
        OpaqueBackgroundEntry *entry = *it;
        ///     qDebug() << "* " << entry->name << "," << entry->color.name() << ": used " << entry->customersCount << " times";
        if (entry->customersCount <= 0) {
            ///         qDebug() << " [Deleted entry]";
            delete entry->pixmap;
            entry->pixmap = nullptr;
            it = m_opaqueBackgroundsList.erase(it);
        } else
            ++it;
        ///     qDebug();
    }
}

#include "moc_backgroundmanager.cpp"
