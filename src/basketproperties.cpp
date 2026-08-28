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

#include <algorithm>

#include "backgroundmanager.h"
#include "basketscene.h"
#include "gitwrapper.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "variouswidgets.h"

#include "ui_basketproperties.h"

BasketPropertiesDialog::BasketPropertiesDialog(BasketScene *basket, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::BasketPropertiesUi)
    , m_basket(basket)
{
    // Set up dialog options
    m_ui->setupUi(this);
    QPushButton *okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    okButton->setDefault(true);
    setObjectName("BasketProperties");

    m_ui->icon->setIconType(KIconLoader::NoGroup, KIconLoader::Application);
    m_ui->icon->setIcon(m_basket->icon());

    int size = std::max(m_ui->icon->sizeHint().width(), m_ui->icon->sizeHint().height());
    m_ui->icon->setFixedSize(size, size); // Make it square!
    m_ui->name->setText(m_basket->basketName());
    m_ui->name->setMinimumWidth(m_ui->name->fontMetrics().maxWidth() * 20);

    // Appearance:
    m_backgroundColor = new KColorCombo2(m_basket->backgroundColorSetting(), palette().color(QPalette::Base), m_ui->appearanceGroup);
    m_textColor = new KColorCombo2(m_basket->textColorSetting(), palette().color(QPalette::Text), m_ui->appearanceGroup);

    m_ui->bgColorLbl->setBuddy(m_backgroundColor);
    m_ui->txtColorLbl->setBuddy(m_textColor);

    m_ui->appearanceLayout->addWidget(m_backgroundColor, 1, 2);
    m_ui->appearanceLayout->addWidget(m_textColor, 2, 2);

    setTabOrder(m_ui->backgroundImage, m_backgroundColor);
    setTabOrder(m_backgroundColor, m_textColor);
    setTabOrder(m_textColor, m_ui->columnForm);

    m_backgroundImagesMap.insert(0, QString());
    QStringList backgrounds = Global::backgroundManager->imageNames();
    int index = 1;
    for (QStringList::Iterator it = backgrounds.begin(); it != backgrounds.end(); ++it) {
        QPixmap *preview = Global::backgroundManager->preview(*it);
        if (preview) {
            m_backgroundImagesMap.insert(index, *it);
            m_ui->backgroundImage->insertItem(index, *it);
            m_ui->backgroundImage->setItemData(index, *preview, Qt::DecorationRole);
            if (m_basket->backgroundImageName() == *it)
                m_ui->backgroundImage->setCurrentIndex(index);
            index++;
        }
    }
    //  m_backgroundImage->insertItem(i18n("Other..."), -1);
    int BUTTON_MARGIN = qApp->style()->pixelMetric(QStyle::PM_ButtonMargin);
    m_ui->backgroundImage->setMaxVisibleItems(50 /*75 * 6 / m_backgroundImage->sizeHint().height()*/);
    m_ui->backgroundImage->setMinimumHeight(75 + 2 * BUTTON_MARGIN);

    // Disposition:

    m_ui->columnCount->setValue(m_basket->columnsCount());
    connect(m_ui->columnCount, &QSpinBox::valueChanged, this, &BasketPropertiesDialog::selectColumnsLayout);

    int height = std::max(m_ui->mindMap->sizeHint().height(), m_ui->columnCount->sizeHint().height()); // Make all radioButtons vertically equally-spaced!
    m_ui->mindMap->setMinimumSize(m_ui->mindMap->sizeHint().width(),
                                  height); // Because the m_columnCount can be higher, and make radio1 and radio2 more spaced than radio2 and radio3.

    if (!m_basket->isFreeLayout())
        m_ui->columnForm->setChecked(true);
    else if (m_basket->isMindMap())
        m_ui->mindMap->setChecked(true);
    else
        m_ui->freeForm->setChecked(true);

    m_ui->mindMap->hide();

    // Keyboard Shortcut:
    QList<QKeySequence> shortcuts{m_basket->shortcut()};
    m_ui->shortcut->setShortcut(shortcuts);

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

    m_ui->shortcutLayout->addWidget(helpLabel);
    connect(m_ui->shortcut, &KShortcutWidget::shortcutChanged, this, &BasketPropertiesDialog::capturedShortcut);

    setTabOrder(m_ui->columnCount, m_ui->shortcut);
    setTabOrder(m_ui->shortcut, helpLabel);
    setTabOrder(helpLabel, m_ui->showBasket);

    switch (m_basket->shortcutAction()) {
    default:
    case 0:
        m_ui->showBasket->setChecked(true);
        break;
    case 1:
        m_ui->globalButton->setChecked(true);
        break;
    case 2:
        m_ui->switchButton->setChecked(true);
        break;
    }

    // Connect the Ok and Apply buttons to actually apply the changes
    connect(okButton, &QPushButton::clicked, this, &BasketPropertiesDialog::applyChanges);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &BasketPropertiesDialog::applyChanges);
}

BasketPropertiesDialog::~BasketPropertiesDialog()
{
    delete m_ui;
}

bool BasketPropertiesDialog::event(QEvent *event)
{
    const bool result = QDialog::event(event);
    if (event->type() == QEvent::Polish) {
        m_ui->name->setFocus();
    }
    return result;
}

void BasketPropertiesDialog::applyChanges()
{
    if (m_ui->columnForm->isChecked()) {
        m_basket->setDisposition(0, m_ui->columnCount->value());
    } else if (m_ui->freeForm->isChecked()) {
        m_basket->setDisposition(1, m_ui->columnCount->value());
    } else {
        m_basket->setDisposition(2, m_ui->columnCount->value());
    }

    if (m_ui->showBasket->isChecked()) {
        m_basket->setShortcut(m_ui->shortcut->shortcut()[0], 0);
    } else if (m_ui->globalButton->isChecked()) {
        m_basket->setShortcut(m_ui->shortcut->shortcut()[0], 1);
    } else if (m_ui->switchButton->isChecked()) {
        m_basket->setShortcut(m_ui->shortcut->shortcut()[0], 2);
    }

    // Should be called LAST, because it will emit the propertiesChanged() signal and the tree will be able to show the newly set Alt+Letter shortcut:
    m_basket->setAppearance(m_ui->icon->icon(),
                            m_ui->name->text(),
                            m_backgroundImagesMap[m_ui->backgroundImage->currentIndex()],
                            m_backgroundColor->color(),
                            m_textColor->color());
    GitWrapper::commitBasket(m_basket);
    m_basket->save();
}

void BasketPropertiesDialog::capturedShortcut(const QList<QKeySequence> &sc)
{
    // TODO: Validate it!
    m_ui->shortcut->setShortcut(sc);
}

void BasketPropertiesDialog::selectColumnsLayout()
{
    m_ui->columnForm->setChecked(true);
}

#include "moc_basketproperties.cpp"
