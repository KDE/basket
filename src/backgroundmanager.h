/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#include <QList>
#include <QObject>
#include <QTimer>
#include <QColor>

class QPixmap;
class QString;

/** A node in the list of background images of BackgroundManager.
 * It can only be used by BackgroundManager because it is an internal structure of this manager.
 * @author Sébastien Laoût
 */
class BackgroundEntry
{
    friend class BackgroundManager;

public:
    ~BackgroundEntry();

protected:
    BackgroundEntry(const QString &location);

    QString name;
    QString location;
    bool tiled;       /// << Only valid after some object subscribed to this image! Because it's only read at this time.
    QPixmap *pixmap;  /// << Only valid (non-null) after some object subscribed to this image! Because it's only read at this time.
    QPixmap *preview; /// << Only valid (non-null) after some object requested the preview.
    int customersCount;
};

/** A node in the list of opaque background images (with a background color applied to an image) of BackgroundManager.
 * It can only be used by BackgroundManager because it is an internal structure of this manager.
 * @author Sébastien Laoût
 */
class OpaqueBackgroundEntry
{
    friend class BackgroundManager;

public:
    ~OpaqueBackgroundEntry();

protected:
    OpaqueBackgroundEntry(const QString &name, const QColor &color);

    QString name;
    QColor color;
    QPixmap *pixmap;
    int customersCount;
};

/** Manage the list of background images.
 * BASIC FUNCTIONNING OF A BACKGROUND CHOOSER:
 *   It get all image names with imageNames() to put them in eg. a QComboBox and then,
 *   when it's time to get the preview of an image it call preview() with the image name to get it.
 *   Preview are only computed on demand and then cached to fast the next demands (only the pointer will have to be returned).
 *   Previews are scaled to fit in a rectangle of 100 by 75 pixels, and with a white background color.
 *   They are also saved to files, so that the scaling/opaquification has not to be done later (they will be directly loaded from file).
 *   Previews are saved in Global::backgroundsFolder()+"previews/", so that emptying the folder is sufficient to remove them.
 * BASIC FUNCTIONING OF AN IMAGE REQUESTER:
 *   When eg. a basket is assigned an image name, it register it with subscribe().
 *   The full pixmap is then loaded from file and cached (if it was not already loaded) and the "tiled" property is read from the image configuration file.
 *   If this object want to have the pixmap applied on a background color (for no transparency => really faster drawing),
 *   it should register for the couple (imageName,color) with subscribe(): the pixmap will be created in the cache.
 *   Then, the object can get the subscribed images with pixmap() or opaquePixmap() and know if it's tiled with tiled().
 *   When the user removed the object background image (or when the object/basket/... is removed), the object should call unsubscribe() for
 *   EVERY subscribed image and image couples. Usage count is decreased for those images and a garbage collector will remove the cached images
 *   if nothing is subscribed to them (to free memory).
 * @author Sébastien Laoût
 */
class BackgroundManager : private QObject
{
    Q_OBJECT
private:
    /// LIST OF IMAGES:
    typedef QList<BackgroundEntry *> BackgroundsList;
    typedef QList<OpaqueBackgroundEntry *> OpaqueBackgroundsList;

public:
    /// CONTRUCTOR AND DESTRUCTOR:
    BackgroundManager();
    ~BackgroundManager() override;
    /// SUBSCRIPTION TO IMAGES:
    bool subscribe(const QString &image);                      /// << @Return true if the loading is a success. In the counter-case, calling methods below is unsafe with this @p image name.
    bool subscribe(const QString &image, const QColor &color); /// << Idem.
    void unsubscribe(const QString &image);
    void unsubscribe(const QString &image, const QColor &color);
    /// GETTING THE IMAGES AND PROPERTIES:
    QPixmap *pixmap(const QString &image);
    QPixmap *opaquePixmap(const QString &image, const QColor &color);
    bool tiled(const QString &image);
    /// LIST OF IMAGES AND PREVIEWS:
    bool exists(const QString &image);
    QStringList imageNames();
    QPixmap *preview(const QString &image);
    /// USED FOR EXPORTATION:
    QString pathForImageName(const QString &image); /// << It is STRONGLY advised to not use those two methods unless it's to copy (export) the images or something like that...
    QString previewPathForImageName(const QString &image);
    /// USED FOR IMPORTATION:
    void addImage(const QString &fullPath);

private:
    BackgroundEntry *backgroundEntryFor(const QString &image);
    OpaqueBackgroundEntry *opaqueBackgroundEntryFor(const QString &image, const QColor &color);

private:
    BackgroundsList m_backgroundsList;
    OpaqueBackgroundsList m_opaqueBackgroundsList;
    QTimer m_garbageTimer;
private Q_SLOTS:
    void requestDelayedGarbage();
    void doGarbage();
};

#endif // BACKGROUNDMANAGER_H
