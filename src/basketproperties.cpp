/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "basketproperties.h"

#include <QApplication>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QStringList>
#include <QStyle>
#include <QVBoxLayout>

#include <KComboBox>
#include <KConfigGroup>
#include <KIconDialog>
#include <KIconLoader>
#include <KLocalizedString>
#include <KShortcutWidget>

#include "backgroundmanager.h"
#include "basketscene.h"
#include "gitwrapper.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "variouswidgets.h"

#include "ui_basketproperties.h"

BasketPropertiesDialog::BasketPropertiesDialog(BasketScene *basket, QWidget *parent)
    : QDialog(parent)
    , Ui::BasketPropertiesUi()
    , m_basket(basket)
{
    // Set up dialog options
    setWindowTitle(i18n("Basket Properties"));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    auto *mainWidget = new QWidget(this);
    setupUi(mainWidget);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &BasketPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &BasketPropertiesDialog::reject);
    mainLayout->addWidget(buttonBox);
    okButton->setDefault(true);
    setObjectName("BasketProperties");
    setModal(true);

    auto *propsUi = dynamic_cast<Ui::BasketPropertiesUi *>(this); // cast to remove name ambiguity
    propsUi->icon->setIconType(KIconLoader::NoGroup, KIconLoader::Application);
    propsUi->icon->setIconSize(16);
    propsUi->icon->setIcon(m_basket->icon());

    int size = qMax(propsUi->icon->sizeHint().width(), propsUi->icon->sizeHint().height());
    propsUi->icon->setFixedSize(size, size); // Make it square!
    propsUi->icon->setToolTip(i18n("Icon"));
    propsUi->name->setText(m_basket->basketName());
    propsUi->name->setMinimumWidth(propsUi->name->fontMetrics().maxWidth() * 20);
    propsUi->name->setToolTip(i18n("Name"));

    // Appearance:
    m_backgroundColor = new KColorCombo2(m_basket->backgroundColorSetting(), palette().color(QPalette::Base), appearanceGroup);
    m_textColor = new KColorCombo2(m_basket->textColorSetting(), palette().color(QPalette::Text), appearanceGroup);

    bgColorLbl->setBuddy(m_backgroundColor);
    txtColorLbl->setBuddy(m_textColor);

    appearanceLayout->addWidget(m_backgroundColor, 1, 2);
    appearanceLayout->addWidget(m_textColor, 2, 2);

    setTabOrder(backgroundImage, m_backgroundColor);
    setTabOrder(m_backgroundColor, m_textColor);
    setTabOrder(m_textColor, columnForm);

    backgroundImage->addItem(i18n("(None)"));
    m_backgroundImagesMap.insert(0, QString());
    backgroundImage->setIconSize(QSize(100, 75));
    QStringList backgrounds = Global::backgroundManager->imageNames();
    int index = 1;
    for (QStringList::Iterator it = backgrounds.begin(); it != backgrounds.end(); ++it) {
        QPixmap *preview = Global::backgroundManager->preview(*it);
        if (preview) {
            m_backgroundImagesMap.insert(index, *it);
            backgroundImage->insertItem(index, *it);
            backgroundImage->setItemData(index, *preview, Qt::DecorationRole);
            if (m_basket->backgroundImageName() == *it)
                backgroundImage->setCurrentIndex(index);
            index++;
        }
    }
    //  m_backgroundImage->insertItem(i18n("Other..."), -1);
    int BUTTON_MARGIN = qApp->style()->pixelMetric(QStyle::PM_ButtonMargin);
    backgroundImage->setMaxVisibleItems(50 /*75 * 6 / m_backgroundImage->sizeHint().height()*/);
    backgroundImage->setMinimumHeight(75 + 2 * BUTTON_MARGIN);

    // Disposition:

    columnCount->setRange(1, 20);
    columnCount->setValue(m_basket->columnsCount());
    connect(columnCount, &QSpinBox::valueChanged, this, &BasketPropertiesDialog::selectColumnsLayout);

    int height = qMax(mindMap->sizeHint().height(), columnCount->sizeHint().height()); // Make all radioButtons vertically equally-spaced!
    mindMap->setMinimumSize(mindMap->sizeHint().width(),
                            height); // Because the m_columnCount can be higher, and make radio1 and radio2 more spaced than radio2 and radio3.

    if (!m_basket->isFreeLayout())
        columnForm->setChecked(true);
    else if (m_basket->isMindMap())
        mindMap->setChecked(true);
    else
        freeForm->setChecked(true);

    mindMap->hide();

    // Keyboard Shortcut:
    QList<QKeySequence> shortcuts{m_basket->shortcut()};
    shortcut->setShortcut(shortcuts);

    auto *helpLabel = new HelpLabel(
        i18n("Learn some tips..."),
        i18n("<p><strong>Easily Remember your Shortcuts</strong>:<br>"
             "With the first option, giving the basket a shortcut of the form <strong>Alt+Letter</strong> will underline that letter in the basket tree.<br>"
             "For instance, if you are assigning the shortcut <i>Alt+T</i> to a basket named <i>Tips</i>, the basket will be displayed as <i><u>T</u>ips</i> "
             "in the tree. "
             "It helps you visualize the shortcuts to remember them more quickly.</p>"
             "<p><strong>Local vs Global</strong>:<br>"
             "The first option allows you to show the basket while the main window is active. "
             "Global shortcuts are valid from anywhere, even if the window is hidden.</p>"
             "<p><strong>Show vs Switch</strong>:<br>"
             "The last option makes this basket the current one without opening the main window. "
             "It is useful in addition to the configurable global shortcuts, eg. to paste the clipboard or the selection into the current basket from "
             "anywhere.</p>"),
        nullptr);

    shortcutLayout->addWidget(helpLabel);
    connect(shortcut, &KShortcutWidget::shortcutChanged, this, &BasketPropertiesDialog::capturedShortcut);

    setTabOrder(columnCount, shortcut);
    setTabOrder(shortcut, helpLabel);
    setTabOrder(helpLabel, showBasket);

    switch (m_basket->shortcutAction()) {
    default:
    case 0:
        showBasket->setChecked(true);
        break;
    case 1:
        globalButton->setChecked(true);
        break;
    case 2:
        switchButton->setChecked(true);
        break;
    }

    // Connect the Ok and Apply buttons to actually apply the changes
    connect(okButton, &QPushButton::clicked, this, &BasketPropertiesDialog::applyChanges);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &BasketPropertiesDialog::applyChanges);
}

BasketPropertiesDialog::~BasketPropertiesDialog() = default;

void BasketPropertiesDialog::ensurePolished()
{
    QWidget::ensurePolished();
    auto *propsUi = dynamic_cast<Ui::BasketPropertiesUi *>(this);
    propsUi->name->setFocus();
}

void BasketPropertiesDialog::applyChanges()
{
    if (columnForm->isChecked()) {
        m_basket->setDisposition(0, columnCount->value());
    } else if (freeForm->isChecked()) {
        m_basket->setDisposition(1, columnCount->value());
    } else {
        m_basket->setDisposition(2, columnCount->value());
    }

    if (showBasket->isChecked()) {
        m_basket->setShortcut(shortcut->shortcut()[0], 0);
    } else if (globalButton->isChecked()) {
        m_basket->setShortcut(shortcut->shortcut()[0], 1);
    } else if (switchButton->isChecked()) {
        m_basket->setShortcut(shortcut->shortcut()[0], 2);
    }

    auto *propsUi = dynamic_cast<Ui::BasketPropertiesUi *>(this);
    // Should be called LAST, because it will emit the propertiesChanged() signal and the tree will be able to show the newly set Alt+Letter shortcut:
    m_basket->setAppearance(propsUi->icon->icon(),
                            propsUi->name->text(),
                            m_backgroundImagesMap[backgroundImage->currentIndex()],
                            m_backgroundColor->color(),
                            m_textColor->color());
    GitWrapper::commitBasket(m_basket);
    m_basket->save();
}

void BasketPropertiesDialog::capturedShortcut(const QList<QKeySequence> &sc)
{
    // TODO: Validate it!
    shortcut->setShortcut(sc);
}

void BasketPropertiesDialog::selectColumnsLayout()
{
    columnForm->setChecked(true);
}

#include "moc_basketproperties.cpp"
