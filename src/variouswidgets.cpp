/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "variouswidgets.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QSizeGrip>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QDrag>
#include <QtGui/QFontDatabase>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

#include <KConfigGroup>
#include <KIconLoader>
#include <KLocalizedString>
#include <KOpenWithDialog>

/** class RunCommandRequester: */

RunCommandRequester::RunCommandRequester(const QString &runCommand, const QString &message, QWidget *parent)
    : QWidget(parent)
{
    m_message = message;

    QHBoxLayout *layout = new QHBoxLayout(this);
    m_runCommand = new QLineEdit(runCommand, this);
    QPushButton *pb = new QPushButton(/*"C&hoose..."*/ i18n("..."), this);

    pb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    layout->addWidget(m_runCommand);
    layout->addWidget(pb);

    connect(pb, &QPushButton::clicked, this, &RunCommandRequester::slotSelCommand);
}

RunCommandRequester::~RunCommandRequester()
{
}

void RunCommandRequester::slotSelCommand()
{
    QPointer<KOpenWithDialog> dlg = new KOpenWithDialog(QList<QUrl>(), m_message, m_runCommand->text(), this);
    dlg->exec();
    if (!dlg->text().isEmpty())
        m_runCommand->setText(dlg->text());
}

QString RunCommandRequester::runCommand()
{
    return m_runCommand->text();
}

void RunCommandRequester::setRunCommand(const QString &runCommand)
{
    m_runCommand->setText(runCommand);
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

IconSizeCombo::~IconSizeCombo()
{
}

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
    QLabel *label = new QLabel(i18n("Resize the window to select the image size\n"
                                    "and close it or press Escape to accept changes."),
                               this);
    label->move(8, 8);
    label->setFixedSize(label->sizeHint());

    // setSizeGripEnabled(true) doesn't work (the grip stay at the same place), so we emulate it:
    m_sizeGrip = new QSizeGrip(this);
    m_sizeGrip->setFixedSize(m_sizeGrip->sizeHint());

    setGeometry(x(), y(), w, h);
}

ViewSizeDialog::~ViewSizeDialog()
{
}

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
    connect(this, SIGNAL(leftClickedUrl()), this, SLOT(display()));
}

HelpLabel::~HelpLabel()
{
}

void HelpLabel::display()
{
    QWhatsThis::showText(mapToGlobal(QPoint(width() / 2, height())), m_message);
}

/** class IconSizeDialog: */

class UndraggableKIconView : public QListWidget
{
public:
    UndraggableKIconView(QWidget *parent = nullptr)
        : QListWidget(parent)
    {
        this->setViewMode(QListView::IconMode);
        this->setMovement(QListView::Static);
        this->setSelectionMode(QAbstractItemView::SingleSelection);
        this->setWrapping(false);
    }
    QDrag *dragObject()
    {
        return nullptr;
    }
};

IconSizeDialog::IconSizeDialog(const QString &caption, const QString &message, const QString &icon, int iconSize, QWidget *parent)
    : QDialog(parent)
{
    // QDialog options
    setWindowTitle(caption);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    setModal(true);

    QWidget *page = new QWidget(this);
    QVBoxLayout *topLayout = new QVBoxLayout(page);

    QLabel *label = new QLabel(message, page);
    topLayout->addWidget(label);

    QListWidget *iconView = new UndraggableKIconView(page);

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

    connect(iconView, SIGNAL(executed(QListWidgetItem *)), this, SLOT(choose(QListWidgetItem *)));
    connect(iconView, &QListWidget::itemActivated, this, &IconSizeDialog::choose);
    connect(iconView, &QListWidget::itemSelectionChanged, this, &IconSizeDialog::slotSelectionChanged);

    mainLayout->addWidget(page);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &IconSizeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &IconSizeDialog::reject);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &IconSizeDialog::slotCancel);
}

IconSizeDialog::~IconSizeDialog()
{
}

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

    //connect(this, &FontSizeCombo::activated, this, &FontSizeCombo::textChangedInCombo);
    connect(this, &FontSizeCombo::editTextChanged, this, &FontSizeCombo::textChangedInCombo);

    // TODO: 01617 void KFontSizeAction::setFontSize( int size )
}

FontSizeCombo::~FontSizeCombo()
{
}

void FontSizeCombo::textChangedInCombo(const QString &text)
{
    bool ok = false;
    int size = text.toInt(&ok);
    if (ok)
        emit sizeChanged(size);
}

void FontSizeCombo::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        emit escapePressed();
    else if (event->key() == Qt::Key_Return)
        emit returnPressed2();
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
