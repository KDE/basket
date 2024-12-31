/**
 * SPDX-FileCopyrightText: (C) 2006 Petri Damsten <damu@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#ifdef HAVE_LIBGPGME

#include "debugwindow.h"
#include "global.h"
#include "kgpgme.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPasswordDialog>
#include <QApplication>

#include <errno.h> //For errno
#include <locale.h> //For LC_ALL, etc.
#include <unistd.h> //For write

// KGpgSelKey class based on class in KGpg with the same name

class KGpgSelKey : public QDialog
{
private:
    QTreeWidget *keysListpr;

public:
    KGpgSelKey(QWidget *parent, const char *name, QString preselected, const KGpgMe &gpg)
        : QDialog(parent)
    {
        // Dialog options
        setObjectName(name);
        setModal(true);
        setWindowTitle(i18n("Private Key List"));

        QWidget *mainWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);

        QString keyname;
        QVBoxLayout *vbox;
        QWidget *page = new QWidget(this);
        QLabel *labeltxt;
        QPixmap keyPair = QIcon::fromTheme(QStringLiteral("kgpg_key2")).pixmap(20, 20);

        setMinimumSize(350, 100);
        keysListpr = new QTreeWidget(page);
        keysListpr->setRootIsDecorated(true);
        keysListpr->setColumnCount(3);
        QStringList headers;
        headers << i18n("Name") << i18n("Email") << i18n("ID");
        keysListpr->setHeaderLabels(headers);
        keysListpr->setAllColumnsShowFocus(true);

        labeltxt = new QLabel(i18n("Choose a secret key:"), page);
        vbox = new QVBoxLayout(page);

        KGpgKeyList list = gpg.keys(true);

        for (KGpgKeyList::iterator it = list.begin(); it != list.end(); ++it) {
            QString name = gpg.checkForUtf8((*it).name);
            QStringList values;
            values << name << (*it).email << (*it).id;
            QTreeWidgetItem *item = new QTreeWidgetItem(keysListpr, values, 3);
            item->setIcon(0, keyPair);
            if (preselected == (*it).id) {
                item->setSelected(true);
                keysListpr->setCurrentItem(item);
            }
        }
        if (!keysListpr->currentItem() && keysListpr->topLevelItemCount() > 0) {
            keysListpr->topLevelItem(0)->setSelected(true);
            keysListpr->setCurrentItem(keysListpr->topLevelItem(0));
        }
        vbox->addWidget(labeltxt);
        vbox->addWidget(keysListpr);
        mainLayout->addWidget(page);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &KGpgSelKey::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &KGpgSelKey::reject);
        mainLayout->addWidget(buttonBox);
    };

    QString key()
    {
        QTreeWidgetItem *item = keysListpr->currentItem();

        if (item)
            return item->text(2);
        return QString();
    }
};

KGpgMe::KGpgMe()
    : m_ctx(0)
    , m_useGnuPGAgent(true)
{
    init(GPGME_PROTOCOL_OpenPGP);
    if (gpgme_new(&m_ctx) == GPG_ERR_NO_ERROR) {
        gpgme_set_armor(m_ctx, 1);
        setPassphraseCb();

        // Set gpg version
        gpgme_engine_info_t info;
        gpgme_get_engine_info(&info);
        while (info != NULL && info->protocol != gpgme_get_protocol(m_ctx)) {
            info = info->next;
        }

        if (info != NULL) {
            QByteArray gpgPath = info->file_name;
            gpgPath.replace("gpg2", "gpg"); // require GnuPG v1
            gpgme_ctx_set_engine_info(m_ctx, GPGME_PROTOCOL_OpenPGP, gpgPath.data(), NULL);
        }

    } else {
        m_ctx = 0;
    }
}

KGpgMe::~KGpgMe()
{
    if (m_ctx)
        gpgme_release(m_ctx);
    clearCache();
}

void KGpgMe::clearCache()
{
    if (m_cache.size() > 0) {
        m_cache.fill(QLatin1Char('\0'));
        m_cache.truncate(0);
    }
}

void KGpgMe::init(gpgme_protocol_t proto)
{
    gpgme_error_t err;

    gpgme_check_version("1.0.0"); // require GnuPG v1
    setlocale(LC_ALL, "");
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#ifndef Q_OS_WIN
    gpgme_set_locale(NULL, LC_MESSAGES, setlocale(LC_MESSAGES, NULL));
#endif
    err = gpgme_engine_check_version(proto);
    if (err) {
        static QString lastErrorText;

        QString text = QStringLiteral("%1: %2").arg(QStringView(gpgme_strsource(err)), QStringView(gpgme_strerror(err)));
        if (text != lastErrorText) {
            KMessageBox::error(qApp->activeWindow(), text);
            lastErrorText = text;
        }
    }
}

QString KGpgMe::checkForUtf8(QString txt)
{
    // code borrowed from KGpg which borrowed it from gpa
    const char *s;

    // Make sure the encoding is UTF-8.
    // Test structure suggested by Werner Koch
    if (txt.isEmpty())
        return QString();

    for (s = txt.toStdString().c_str(); *s && !(*s & 0x80); s++)
        ;
    if (*s && !strchr(txt.toStdString().c_str(), 0xc3) && (txt.indexOf(QStringLiteral("\\x")) == -1))
        return txt;

    // The string is not in UTF-8
    // if (strchr (txt.toLatin1(), 0xc3)) return (txt+" +++");
    if (txt.indexOf(QStringLiteral("\\x")) == -1)
        return QString::fromUtf8(txt.toLatin1());
    //        if (!strchr (txt.toLatin1(), 0xc3) || (txt.indexOf("\\x")!=-1)) {
    for (int idx = 0; (idx = txt.indexOf(QStringLiteral("\\x"), idx)) >= 0; ++idx) {
        char str[2] = "x";
        str[0] = (char)QString(txt.mid(idx + 2, 2)).toShort(0, 16);
        txt.replace(idx, 4, QStringView(str));
    }
    if (!strchr(txt.toStdString().c_str(), 0xc3))
        return txt;
    else
        return QString::fromUtf8(QString::fromUtf8(txt.toLatin1()).toLatin1());
    // perform Utf8 twice, or some keys display badly
    return txt;
}

QString KGpgMe::selectKey(QString previous)
{
    QPointer<KGpgSelKey> dlg = new KGpgSelKey(qApp->activeWindow(), "", previous, *this);

    if (dlg->exec())
        return dlg->key();
    return QString();
}

// Rest of the code is mainly based in gpgme examples

KGpgKeyList KGpgMe::keys(bool privateKeys /* = false */) const
{
    KGpgKeyList keys;
    gpgme_error_t err = 0, err2 = 0;
    gpgme_key_t key = 0;
    gpgme_keylist_result_t result = 0;

    if (m_ctx) {
        err = gpgme_op_keylist_start(m_ctx, NULL, privateKeys);
        if (!err) {
            while (!(err = gpgme_op_keylist_next(m_ctx, &key))) {
                KGpgKey gpgkey;

                if (!key->subkeys)
                    continue;
                gpgkey.id = QStringView(key->subkeys->keyid);
                if (key->uids) {
                    gpgkey.name = QStringView(key->uids->name);
                    gpgkey.email = QStringView(key->uids->email);
                }
                keys.append(gpgkey);
                gpgme_key_unref(key);
            }

            if (gpg_err_code(err) == GPG_ERR_EOF)
                err = 0;
            err2 = gpgme_op_keylist_end(m_ctx);
            if (!err)
                err = err2;
        }
    }

    if (err) {
        KMessageBox::error(qApp->activeWindow(), QStringLiteral("%1: %2").arg(QStringView(gpgme_strsource(err))).arg(QStringView(gpgme_strerror(err))));
    } else {
        result = gpgme_op_keylist_result(m_ctx);
        if (result->truncated) {
            KMessageBox::error(qApp->activeWindow(), i18n("Key listing unexpectedly truncated."));
        }
    }
    return keys;
}

bool KGpgMe::encrypt(const QByteArray &inBuffer, unsigned long length, QByteArray *outBuffer, QString keyid /* = QString() */)
{
    gpgme_error_t err = 0;
    gpgme_data_t in = 0, out = 0;
    gpgme_key_t keys[2] = {NULL, NULL};
    gpgme_key_t *key = NULL;
    gpgme_encrypt_result_t result = 0;

    outBuffer->resize(0);
    if (m_ctx) {
        err = gpgme_data_new_from_mem(&in, inBuffer.data(), length, 1);
        if (!err) {
            err = gpgme_data_new(&out);
            if (!err) {
                if (keyid.isNull()) {
                    key = NULL;
                } else {
                    err = gpgme_get_key(m_ctx, keyid.toStdString().c_str(), &keys[0], 0);
                    key = keys;
                }

                if (!err) {
                    err = gpgme_op_encrypt(m_ctx, key, GPGME_ENCRYPT_ALWAYS_TRUST, in, out);
                    if (!err) {
                        result = gpgme_op_encrypt_result(m_ctx);
                        if (result->invalid_recipients) {
                            KMessageBox::error(qApp->activeWindow(),
                                               QStringLiteral("%1: %2")
                                                   .arg(i18n("That public key is not meant for encryption"))
                                                   .arg(QStringView(result->invalid_recipients->fpr)));
                        } else {
                            err = readToBuffer(out, outBuffer);
                        }
                    }
                }
            }
        }
    }
    if (err != GPG_ERR_NO_ERROR && err != GPG_ERR_CANCELED) {
        KMessageBox::error(qApp->activeWindow(), QStringLiteral("%1: %2").arg(QStringView(gpgme_strsource(err))).arg(QStringView(gpgme_strerror(err))));
    }
    if (err != GPG_ERR_NO_ERROR) {
        DEBUG_WIN << QStringLiteral("KGpgMe::encrypt error: ") + QString::number(err);
        clearCache();
    }
    if (keys[0])
        gpgme_key_unref(keys[0]);
    if (in)
        gpgme_data_release(in);
    if (out)
        gpgme_data_release(out);
    return (err == GPG_ERR_NO_ERROR);
}

bool KGpgMe::decrypt(const QByteArray &inBuffer, QByteArray *outBuffer)
{
    gpgme_error_t err = 0;
    gpgme_data_t in = 0, out = 0;
    gpgme_decrypt_result_t result = 0;

    outBuffer->resize(0);
    if (m_ctx) {
        err = gpgme_data_new_from_mem(&in, inBuffer.data(), inBuffer.size(), 1);
        if (!err) {
            err = gpgme_data_new(&out);
            if (!err) {
                err = gpgme_op_decrypt(m_ctx, in, out);
                if (!err) {
                    result = gpgme_op_decrypt_result(m_ctx);
                    if (result->unsupported_algorithm) {
                        KMessageBox::error(qApp->activeWindow(),
                                           QStringLiteral("%1: %2").arg(i18n("Unsupported algorithm")).arg(QStringView(result->unsupported_algorithm)));
                    } else {
                        err = readToBuffer(out, outBuffer);
                    }
                }
            }
        }
    }
    if (err != GPG_ERR_NO_ERROR && err != GPG_ERR_CANCELED) {
        KMessageBox::error(qApp->activeWindow(), QStringLiteral("%1: %2").arg(QStringView(gpgme_strsource(err))).arg(QStringView(gpgme_strerror(err))));
    }
    if (err != GPG_ERR_NO_ERROR)
        clearCache();
    if (in)
        gpgme_data_release(in);
    if (out)
        gpgme_data_release(out);
    return (err == GPG_ERR_NO_ERROR);
}

#define BUF_SIZE (32 * 1024)

gpgme_error_t KGpgMe::readToBuffer(gpgme_data_t in, QByteArray *outBuffer) const
{
    int ret;
    gpgme_error_t err = GPG_ERR_NO_ERROR;

    ret = gpgme_data_seek(in, 0, SEEK_SET);
    if (ret) {
        err = gpgme_err_code_from_errno(errno);
    } else {
        char *buf = new char[BUF_SIZE + 2];

        if (buf) {
            while ((ret = gpgme_data_read(in, buf, BUF_SIZE)) > 0) {
                uint size = outBuffer->size();
                outBuffer->resize(size + ret);
                memcpy(outBuffer->data() + size, buf, ret);
            }
            if (ret < 0)
                err = gpgme_err_code_from_errno(errno);
            delete[] buf;
        }
    }
    return err;
}

bool KGpgMe::isGnuPGAgentAvailable()
{
    QString agent_info = QStringView(qgetenv("GPG_AGENT_INFO"));

    if (agent_info.indexOf(QLatin1Char(':')) > 0)
        return true;
    return false;
}

void KGpgMe::setPassphraseCb()
{
    bool agent = false;
    QString agent_info;

    agent_info = QStringView(qgetenv("GPG_AGENT_INFO"));

    if (m_useGnuPGAgent) {
        if (agent_info.indexOf(QLatin1Char(':')))
            agent = true;
        if (agent_info.startsWith(QStringLiteral("disable:")))
            setenv("GPG_AGENT_INFO", agent_info.mid(8).toStdString().c_str(), 1);
    } else {
        if (!agent_info.startsWith(QStringLiteral("disable:")))
            setenv("GPG_AGENT_INFO", (std::string("disable:") + agent_info.toStdString()).c_str(), 1);
    }
    if (agent)
        gpgme_set_passphrase_cb(m_ctx, 0, 0);
    else
        gpgme_set_passphrase_cb(m_ctx, passphraseCb, this);
}

gpgme_error_t KGpgMe::passphraseCb(void *hook, const char *uid_hint, const char *passphrase_info, int last_was_bad, int fd)
{
    KGpgMe *gpg = static_cast<KGpgMe *>(hook);
    return gpg->passphrase(uid_hint, passphrase_info, last_was_bad, fd);
}

gpgme_error_t KGpgMe::passphrase(const char *uid_hint, const char * /*passphrase_info*/, int last_was_bad, int fd)
{
    QString s;
    QString gpg_hint = checkForUtf8(QStringView(uid_hint));
    bool canceled = false;

    if (last_was_bad) {
        s += QStringLiteral("<b>") + i18n("Wrong password.") + QStringLiteral("</b><br><br>\n\n");
        clearCache();
    }

    if (!m_text.isEmpty())
        s += m_text + QStringLiteral("<br>");

    if (!gpg_hint.isEmpty())
        s += gpg_hint;

    if (m_cache.isEmpty()) {
        KPasswordDialog dlg;
        dlg.setPrompt(s);

        if (m_saving)
            dlg.setWindowTitle(i18n("Please enter a new password:"));

        if (dlg.exec())
            m_cache = dlg.password();
        else
            canceled = true;
    }

    if (!canceled)
        write(fd, m_cache.data(), m_cache.length());
    write(fd, "\n", 1);
    return canceled ? GPG_ERR_CANCELED : GPG_ERR_NO_ERROR;
}
#endif // HAVE_LIBGPGME
