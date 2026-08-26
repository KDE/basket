/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "variouswidgets.h"
#include "debugwindow.h"

#include <QDialogButtonBox>
#include <QDrag>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWhatsThis>

#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KOpenWithDialog>

/** class ServiceLaunchRequester: */

ServiceLaunchRequester::ServiceLaunchRequester(const QString serviceLauncher, const QString message, QWidget *parent)
    : QWidget(parent)
    , m_serviceLauncher(serviceLauncher)
    , m_message(message)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_serviceChooser = new QPushButton(this);
    m_serviceChooser->setIconSize(QSize(24, 24));
    setServiceLauncher(serviceLauncher);
    layout->addWidget(m_serviceChooser);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    connect(m_serviceChooser, &QPushButton::clicked, this, &ServiceLaunchRequester::slotSelCommand);
}

ServiceLaunchRequester::~ServiceLaunchRequester() = default;

void ServiceLaunchRequester::slotSelCommand()
{
    KService::Ptr service = KService::serviceByStorageId(m_serviceLauncher);
    QString service_name;
    if (service && service->isApplication()) {
        QString exec = service->exec();
        service_name = exec.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(0);
    }
    QPointer<KOpenWithDialog> dlg = new KOpenWithDialog(QList<QUrl>(), m_message, service_name, this);
    dlg->exec();

    if (!dlg->text().isEmpty()) {
        KService::Ptr selectedService = dlg->service();
        if (selectedService) {
            m_serviceLauncher = selectedService->storageId(); // e.g., "firefox"
            setServiceLauncher(m_serviceLauncher);
        }
    }
}

QString ServiceLaunchRequester::serviceLauncher()
{
    return m_serviceLauncher;
}

void ServiceLaunchRequester::setServiceLauncher(const QString &serviceLauncher)
{
    m_serviceLauncher = serviceLauncher;

    QString displayName;
    QString comment;
    QIcon buttonIcon;

    KService::Ptr service = KService::serviceByStorageId(serviceLauncher);

    if (service && service->isApplication()) {
        KIconLoader *iconLoader = KIconLoader::global();

        const QString serviceIcon = service->icon();
        QString iconPath;
        if (!serviceIcon.isEmpty()) {
            iconPath = iconLoader->iconPath(serviceIcon, KIconLoader::Desktop, true);
        }
        if (!iconPath.isEmpty()) {
            if (serviceIcon.contains(QLatin1Char('/'))) {
                // Path-like, so use it as-is
                buttonIcon = QIcon(iconPath);
            } else {
                // Name only, so assume it is in the icon theme
                buttonIcon = QIcon::fromTheme(serviceIcon);
            }
        } else {
            buttonIcon = QIcon::fromTheme(QStringLiteral("kde-symbolic"));
        }

        const QString name = service->name();
        const QString genericName = service->genericName();
        if (!genericName.isEmpty())
            displayName = i18nc("'<name> (<generic name>)' of application, e.g. 'KWrite (Text Editor)'", "%1 (%2)", name, genericName);
        else
            displayName = name;
        comment = service->comment();
    } else {
        buttonIcon = QIcon::fromTheme(QStringLiteral("kde-symbolic"));
        displayName = i18n("Choose an Application Launcher ...");
        comment = i18n("Use KDE Plasma Application Launchers to open your Basket Notes");
    }

    m_serviceChooser->setIcon(buttonIcon);
    m_serviceChooser->setText(displayName);
    m_serviceChooser->setToolTip(comment);

    Q_EMIT launcherChanged();
}

void ServiceLaunchRequester::setFocus()
{
    m_serviceChooser->setFocus();
}

/** class IconSizeCombo: */

IconSizeCombo::IconSizeCombo(QWidget *parent)
    : KComboBox(parent)
{
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeSmall));
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeSmallMedium));
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeMedium));
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeLarge));
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeHuge));
    addItem(i18n("%1 by %1 pixels", KIconLoader::SizeEnormous));
    setCurrentIndex(2);
}

IconSizeCombo::~IconSizeCombo() = default;

int IconSizeCombo::iconSize()
{
    switch (currentIndex()) {
    default:
    case 0:
        return KIconLoader::SizeSmall;
    case 1:
        return KIconLoader::SizeSmallMedium;
    case 2:
        return KIconLoader::SizeMedium;
    case 3:
        return KIconLoader::SizeLarge;
    case 4:
        return KIconLoader::SizeHuge;
    case 5:
        return KIconLoader::SizeEnormous;
    }
}

void IconSizeCombo::setSize(int size)
{
    switch (size) {
    default:
    case KIconLoader::SizeSmall:
        setCurrentIndex(0);
        break;
    case KIconLoader::SizeSmallMedium:
        setCurrentIndex(1);
        break;
    case KIconLoader::SizeMedium:
        setCurrentIndex(2);
        break;
    case KIconLoader::SizeLarge:
        setCurrentIndex(3);
        break;
    case KIconLoader::SizeHuge:
        setCurrentIndex(4);
        break;
    case KIconLoader::SizeEnormous:
        setCurrentIndex(5);
        break;
    }
}

/** class ViewSizeDialog: */

ViewSizeDialog::ViewSizeDialog(QWidget *parent, int w, int h)
    : QDialog(parent)
{
    auto *label = new QLabel(i18n("Resize the window to select the image size\n"
                                  "and close it or press Escape to accept changes."),
                             this);
    label->move(8, 8);
    label->setFixedSize(label->sizeHint());

    // setSizeGripEnabled(true) doesn't work (the grip stay at the same place), so we emulate it:
    m_sizeGrip = new QSizeGrip(this);
    m_sizeGrip->setFixedSize(m_sizeGrip->sizeHint());

    setGeometry(x(), y(), w, h);
}

ViewSizeDialog::~ViewSizeDialog() = default;

void ViewSizeDialog::resizeEvent(QResizeEvent *)
{
    setWindowTitle(i18n("%1 by %2 pixels", QString::number(width()), QString::number(height())));
    m_sizeGrip->move(width() - m_sizeGrip->width(), height() - m_sizeGrip->height());
}

/** class HelpLabel: */

HelpLabel::HelpLabel(const QString &text, const QString &message, QWidget *parent)
    : KUrlLabel(parent)
    , m_message(message)
{
    setText(text);
    setWhatsThis(m_message);
    connect(this, &HelpLabel::leftClickedUrl, this, &HelpLabel::display);
}

HelpLabel::~HelpLabel() = default;

void HelpLabel::display()
{
    QWhatsThis::showText(mapToGlobal(QPoint(width() / 2, height())), m_message);
}

/** class IconSizeDialog: */

IconSizeDialog::IconSizeDialog(const QString &caption, const QString &message, const QString &icon, int iconSize, QWidget *parent)
    : QDialog(parent)
{
    // QDialog options
    setWindowTitle(caption);

    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    setModal(true);

    auto *page = new QWidget(this);
    auto *topLayout = new QVBoxLayout(page);

    auto *label = new QLabel(message, page);
    topLayout->addWidget(label);

    QListWidget *iconView = new QListWidget(page);
    iconView->setViewMode(QListView::IconMode);
    iconView->setMovement(QListView::Static);
    iconView->setSelectionMode(QAbstractItemView::SingleSelection);
    iconView->setWrapping(false);
    iconView->setDragEnabled(false);

    QIcon desktopIcon = QIcon::fromTheme(icon);
    m_size16 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeSmall), i18n("%1 by %1 pixels", KIconLoader::SizeSmall), iconView);
    m_size22 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeSmallMedium), i18n("%1 by %1 pixels", KIconLoader::SizeSmallMedium), iconView);
    m_size32 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeMedium), i18n("%1 by %1 pixels", KIconLoader::SizeMedium), iconView);
    m_size48 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeLarge), i18n("%1 by %1 pixels", KIconLoader::SizeLarge), iconView);
    m_size64 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeHuge), i18n("%1 by %1 pixels", KIconLoader::SizeHuge), iconView);
    m_size128 = new QListWidgetItem(desktopIcon.pixmap(KIconLoader::SizeEnormous), i18n("%1 by %1 pixels", KIconLoader::SizeEnormous), iconView);
    iconView->setIconSize(QSize(KIconLoader::SizeEnormous, KIconLoader::SizeEnormous)); // 128x128
    iconView->setMinimumSize(QSize(128 * 6 + (6 + 2) * iconView->spacing() + 20, m_size128->sizeHint().height() + 2 * iconView->spacing() + 20));
    topLayout->addWidget(iconView);
    switch (iconSize) {
    case KIconLoader::SizeSmall:
        m_size16->setSelected(true);
        m_iconSize = KIconLoader::SizeSmall;
        break;
    case KIconLoader::SizeSmallMedium:
        m_size22->setSelected(true);
        m_iconSize = KIconLoader::SizeSmallMedium;
        break;
    default:
    case KIconLoader::SizeMedium:
        m_size32->setSelected(true);
        m_iconSize = KIconLoader::SizeMedium;
        break;
    case KIconLoader::SizeLarge:
        m_size48->setSelected(true);
        m_iconSize = KIconLoader::SizeLarge;
        break;
    case KIconLoader::SizeHuge:
        m_size64->setSelected(true);
        m_iconSize = KIconLoader::SizeHuge;
        break;
    case KIconLoader::SizeEnormous:
        m_size128->setSelected(true);
        m_iconSize = KIconLoader::SizeEnormous;
        break;
    }

    connect(iconView, &QListWidget::itemActivated, this, &IconSizeDialog::choose);
    connect(iconView, &QListWidget::itemSelectionChanged, this, &IconSizeDialog::slotSelectionChanged);

    mainLayout->addWidget(page);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IconSizeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IconSizeDialog::reject);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &IconSizeDialog::slotCancel);
}

IconSizeDialog::~IconSizeDialog() = default;

void IconSizeDialog::slotSelectionChanged()
{
    // Change m_iconSize to the new selected one:
    if (m_size16->isSelected()) {
        m_iconSize = KIconLoader::SizeSmall;
        return;
    }
    if (m_size22->isSelected()) {
        m_iconSize = KIconLoader::SizeSmallMedium;
        return;
    }
    if (m_size32->isSelected()) {
        m_iconSize = KIconLoader::SizeMedium;
        return;
    }
    if (m_size48->isSelected()) {
        m_iconSize = KIconLoader::SizeLarge;
        return;
    }
    if (m_size64->isSelected()) {
        m_iconSize = KIconLoader::SizeHuge;
        return;
    }
    if (m_size128->isSelected()) {
        m_iconSize = KIconLoader::SizeEnormous;
        return;
    }

    // But if user unselected the item (by eg. right clicking a free space), reselect the last one:
    switch (m_iconSize) {
    case KIconLoader::SizeSmall:
        m_size16->setSelected(true);
        m_iconSize = KIconLoader::SizeSmall;
        break;
    case KIconLoader::SizeSmallMedium:
        m_size22->setSelected(true);
        m_iconSize = KIconLoader::SizeSmallMedium;
        break;
    default:
    case KIconLoader::SizeMedium:
        m_size32->setSelected(true);
        m_iconSize = KIconLoader::SizeMedium;
        break;
    case KIconLoader::SizeLarge:
        m_size48->setSelected(true);
        m_iconSize = KIconLoader::SizeLarge;
        break;
    case KIconLoader::SizeHuge:
        m_size64->setSelected(true);
        m_iconSize = KIconLoader::SizeHuge;
        break;
    case KIconLoader::SizeEnormous:
        m_size128->setSelected(true);
        m_iconSize = KIconLoader::SizeEnormous;
        break;
    }
}

void IconSizeDialog::choose(QListWidgetItem *)
{
    okButton->animateClick();
}

void IconSizeDialog::slotCancel()
{
    m_iconSize = -1;
}

/** class FontSizeCombo: */

FontSizeCombo::FontSizeCombo(bool rw, bool withDefault, QWidget *parent)
    : KComboBox(rw, parent)
    , m_withDefault(withDefault)
{
    if (m_withDefault)
        addItem(i18n("(Default)"));

    QFontDatabase fontDB;
    QList<int> sizes = fontDB.standardSizes();
    for (QList<int>::Iterator it = sizes.begin(); it != sizes.end(); ++it)
        addItem(QString::number(*it));

    // connect(this, &FontSizeCombo::activated, this, &FontSizeCombo::textChangedInCombo);
    connect(this, &FontSizeCombo::editTextChanged, this, &FontSizeCombo::textChangedInCombo);

    // TODO: 01617 void KFontSizeAction::setFontSize( int size )
}

FontSizeCombo::~FontSizeCombo() = default;

void FontSizeCombo::textChangedInCombo(const QString &text)
{
    bool ok = false;
    int size = text.toInt(&ok);
    if (ok)
        Q_EMIT sizeChanged(size);
}

void FontSizeCombo::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        Q_EMIT escapePressed();
    else if (event->key() == Qt::Key_Return)
        Q_EMIT returnPressed2();
    else
        KComboBox::keyPressEvent(event);
}

void FontSizeCombo::setFontSize(qreal size)
{
    setItemText(currentIndex(), QString::number(size));

    // TODO: SEE KFontSizeAction::setFontSize( int size ) !!! for a more complete method!
}

qreal FontSizeCombo::fontSize()
{
    bool ok = false;
    int size = currentText().toInt(&ok);
    if (ok)
        return size;

    size = currentText().toInt(&ok);
    if (ok)
        return size;

    return font().pointSize();
}

#include "moc_variouswidgets.cpp"
