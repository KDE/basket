/**
 * SPDX-FileCopyrightText: (C) 2003 by Petri Damsten <petri.damsten@iki.fi>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _BASKETPART_H_
#define _BASKETPART_H_

#include <KParts/ReadWritePart>
#include <KPluginFactory>

class QWidget;
class QPainter;

class KAboutData;
class QUrl;

class BNPView;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Petri Damsten <petri.damsten@iki.fi>
 * @version 0.1
 */
class BasketPart : public KParts::ReadWritePart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    BasketPart(QWidget *parentWidget, QObject *parent, const QList<QVariant> &);

    /**
     * Destructor
     */
    virtual ~BasketPart();

    /**
     * This is a virtual function inherited from KParts::ReadWritePart.
     * A shell will use this to inform this Part if it should act
     * read-only
     */
    virtual void setReadWrite(bool rw);

    /**
     * Reimplemented to disable and enable Save action
     */
    virtual void setModified(bool modified);

    static KAboutData *createAboutData();

signals:
    void showPart();

protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();

    /**
     * This must be implemented by each read-write part
     */
    virtual bool saveFile();

protected slots:
    void setWindowTitle(const QString &caption);

private:
    BNPView *m_view;
};

K_PLUGIN_FACTORY_DECLARATION(BasketFactory)

#endif // _BASKETPART_H_
