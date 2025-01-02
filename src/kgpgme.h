/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KGPGME_H
#define KGPGME_H

#include <config.h>

#ifdef HAVE_LIBGPGME

#include <gpgme.h>

#include <QList>
#include <QString>

/**
    @author Petri Damsten <damu@iki.fi>
*/

class KGpgKey
{
public:
    QString id;
    QString name;
    QString email;
};

using KGpgKeyList = QList<KGpgKey>;

class KGpgMe
{
public:
    KGpgMe();
    ~KGpgMe();

    QString selectKey(QString previous = QString());
    KGpgKeyList keys(bool privateKeys = false) const;
    void setText(QString text, bool saving)
    {
        m_text = text;
        m_saving = saving;
    };
    void setUseGnuPGAgent(bool use)
    {
        m_useGnuPGAgent = use;
        setPassphraseCb();
    };
    QString text() const
    {
        return m_text;
    };
    bool saving() const
    {
        return m_saving;
    };
    void clearCache();

    bool encrypt(const QByteArray &inBuffer, unsigned long length, QByteArray *outBuffer, QString keyid = QString());
    bool decrypt(const QByteArray &inBuffer, QByteArray *outBuffer);

    static QString checkForUtf8(QString txt);
    static bool isGnuPGAgentAvailable();

private:
    gpgme_ctx_t m_ctx;
    QString m_text;
    bool m_saving;
    bool m_useGnuPGAgent;
    QString m_cache;

    void init(gpgme_protocol_t proto);
    gpgme_error_t readToBuffer(gpgme_data_t in, QByteArray *outBuffer) const;
    void setPassphraseCb();
    static gpgme_error_t passphraseCb(void *hook, const char *uid_hint, const char *passphrase_info, int last_was_bad, int fd);
    gpgme_error_t passphrase(const char *uid_hint, const char *passphrase_info, int last_was_bad, int fd);
};
#endif // HAVE_LIBGPGME
#endif // KGPGME_H
