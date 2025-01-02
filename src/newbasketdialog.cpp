/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "newbasketdialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include <KComboBox>
#include <KConfigGroup>
#include <KGuiItem>
#include <KIconButton>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMainWindow> //For Global::mainWindow()
#include <KMessageBox>

#include "basketfactory.h"
#include "basketlistview.h"
#include "basketscene.h"
#include "bnpview.h"
#include "global.h"
#include "kcolorcombo2.h"
#include "tools.h"
#include "variouswidgets.h" //For HelpLabel

/** class SingleSelectionKIconView: */

SingleSelectionKIconView::SingleSelectionKIconView(QWidget *parent)
    : QListWidget(parent)
    , m_lastSelected(nullptr)
{
    setViewMode(QListView::IconMode);
    connect(this, &SingleSelectionKIconView::currentItemChanged, this, &SingleSelectionKIconView::slotSelectionChanged);
}

QMimeData *SingleSelectionKIconView::dragObject()
{
    return nullptr;
}

void SingleSelectionKIconView::slotSelectionChanged(QListWidgetItem *cur)
{
    if (cur)
        m_lastSelected = cur;
}

/** class NewBasketDefaultProperties: */

NewBasketDefaultProperties::NewBasketDefaultProperties()
    : icon(QStringLiteral("org.kde.basket"))
    , backgroundImage(QString())
    , backgroundColor()
    , textColor()
    , freeLayout(false)
    , columnCount(1)
{
}

/** class NewBasketDialog: */

NewBasketDialog::NewBasketDialog(BasketScene *parentBasket, const NewBasketDefaultProperties &defaultProperties, QWidget *parent)
    : QDialog(parent)
    , m_defaultProperties(defaultProperties)
{
    // QDialog options
    setWindowTitle(i18n("New Basket"));

    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    setObjectName("NewBasket");
    setModal(true);

    auto *page = new QWidget(this);
    auto *topLayout = new QVBoxLayout(page);

    // Icon, Name and Background Color:
    auto *nameLayout = new QHBoxLayout;
    // QHBoxLayout *nameLayout = new QHBoxLayout(this);
    m_icon = new KIconButton(page);
    m_icon->setIconType(KIconLoader::NoGroup, KIconLoader::Action);
    m_icon->setIconSize(16);
    m_icon->setIcon(m_defaultProperties.icon);

    int size = qMax(m_icon->sizeHint().width(), m_icon->sizeHint().height());
    m_icon->setFixedSize(size, size); // Make it square!

    m_icon->setToolTip(i18n("Icon"));
    m_name = new QLineEdit(/*i18n("Basket"), */ page);
    m_name->setMinimumWidth(m_name->fontMetrics().maxWidth() * 20);
    connect(m_name, &QLineEdit::textChanged, this, &NewBasketDialog::nameChanged);

    m_name->setToolTip(i18n("Name"));
    m_backgroundColor = new KColorCombo2(QColor(), palette().color(QPalette::Base), page);
    m_backgroundColor->setColor(QColor());
    m_backgroundColor->setFixedSize(m_backgroundColor->sizeHint());
    m_backgroundColor->setColor(m_defaultProperties.backgroundColor);
    m_backgroundColor->setToolTip(i18n("Background color"));
    nameLayout->addWidget(m_icon);
    nameLayout->addWidget(m_name);
    nameLayout->addWidget(m_backgroundColor);
    topLayout->addLayout(nameLayout);

    auto *layout = new QHBoxLayout;
    auto *button = new QPushButton(page);
    KGuiItem::assign(button, KGuiItem(i18n("&Manage Templates..."), QStringLiteral("configure")));
    connect(button, &QPushButton::clicked, this, &NewBasketDialog::manageTemplates);
    button->hide();

    // Compute the right template to use as the default:
    QString defaultTemplate = QStringLiteral("free");
    if (!m_defaultProperties.freeLayout) {
        if (m_defaultProperties.columnCount == 1)
            defaultTemplate = QStringLiteral("1column");
        else if (m_defaultProperties.columnCount == 2)
            defaultTemplate = QStringLiteral("2columns");
        else
            defaultTemplate = QStringLiteral("3columns");
    }

    // Empty:
    // * * * * *
    // Personal:
    // *To Do
    // Professional:
    // *Meeting Summary
    // Hobbies:
    // *
    m_templates = new SingleSelectionKIconView(page);
    m_templates->setSelectionMode(QAbstractItemView::SingleSelection);
    QListWidgetItem *lastTemplate = nullptr;
    QPixmap icon(40, 53);

    QPainter painter(&icon);
    painter.fillRect(0, 0, icon.width(), icon.height(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(0, 0, icon.width(), icon.height());
    painter.end();
    lastTemplate = new QListWidgetItem(icon, i18n("One column"), m_templates);

    if (defaultTemplate == QStringLiteral("1column"))
        m_templates->setCurrentItem(lastTemplate);

    painter.begin(&icon);
    painter.fillRect(0, 0, icon.width(), icon.height(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(0, 0, icon.width(), icon.height());
    painter.drawLine(icon.width() / 2, 0, icon.width() / 2, icon.height());
    painter.end();
    lastTemplate = new QListWidgetItem(icon, i18n("Two columns"), m_templates);

    if (defaultTemplate == QStringLiteral("2columns"))
        m_templates->setCurrentItem(lastTemplate);

    painter.begin(&icon);
    painter.fillRect(0, 0, icon.width(), icon.height(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(0, 0, icon.width(), icon.height());
    painter.drawLine(icon.width() / 3, 0, icon.width() / 3, icon.height());
    painter.drawLine(icon.width() * 2 / 3, 0, icon.width() * 2 / 3, icon.height());
    painter.end();
    lastTemplate = new QListWidgetItem(icon, i18n("Three columns"), m_templates);

    if (defaultTemplate == QStringLiteral("3columns"))
        m_templates->setCurrentItem(lastTemplate);

    painter.begin(&icon);
    painter.fillRect(0, 0, icon.width(), icon.height(), palette().color(QPalette::Base));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawRect(0, 0, icon.width(), icon.height());
    painter.drawRect(icon.width() / 5, icon.width() / 5, icon.width() / 4, icon.height() / 8);
    painter.drawRect(icon.width() * 2 / 5, icon.width() * 2 / 5, icon.width() / 4, icon.height() / 8);
    painter.end();
    lastTemplate = new QListWidgetItem(icon, i18n("Free"), m_templates);

    if (defaultTemplate == QStringLiteral("free"))
        m_templates->setCurrentItem(lastTemplate);

    m_templates->setMinimumHeight(topLayout->minimumSize().width() * 9 / 16);

    auto *label = new QLabel(page);
    label->setText(i18n("&Template:"));
    label->setBuddy(m_templates);
    layout->addWidget(label, /*stretch=*/0, Qt::AlignBottom);
    layout->addStretch();
    layout->addWidget(button, /*stretch=*/0, Qt::AlignBottom);
    topLayout->addLayout(layout);
    topLayout->addWidget(m_templates);

    layout = new QHBoxLayout;
    m_createIn = new KComboBox(page);
    m_createIn->addItem(i18n("(Baskets)"));
    label = new QLabel(page);
    label->setText(i18n("C&reate in:"));
    label->setBuddy(m_createIn);
    auto *helpLabel = new HelpLabel(i18n("How is it useful?"),
                                    i18n("<p>Creating baskets inside of other baskets to form a hierarchy allows you to be more organized by eg.:</p><ul>"
                                         "<li>Grouping baskets by themes or topics;</li>"
                                         "<li>Grouping baskets in folders for different projects;</li>"
                                         "<li>Making sections with sub-baskets representing chapters or pages;</li>"
                                         "<li>Making a group of baskets to export together (to eg. email them to people).</li></ul>"),
                                    page);
    layout->addWidget(label);
    layout->addWidget(m_createIn);
    layout->addWidget(helpLabel);
    layout->addStretch();
    topLayout->addLayout(layout);

    m_basketsMap.clear();
    int index;
    m_basketsMap.insert(/*index=*/0, /*basket=*/nullptr);
    index = 1;
    for (int i = 0; i < Global::bnpView->topLevelItemCount(); i++) {
        index = populateBasketsList(Global::bnpView->topLevelItem(i), /*indent=*/1, /*index=*/index);
    }

    connect(m_templates, &QListWidget::itemDoubleClicked, this, &NewBasketDialog::slotOk);
    connect(m_templates, &QListWidget::itemActivated, this, &NewBasketDialog::returnPressed);

    mainLayout->addWidget(page);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(okButton, &QPushButton::clicked, this, &NewBasketDialog::slotOk);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);
    okButton->setEnabled(false);

    if (parentBasket) {
        int index = 0;

        for (QMap<int, BasketScene *>::Iterator it = m_basketsMap.begin(); it != m_basketsMap.end(); ++it) {
            if (it.value() == parentBasket) {
                index = it.key();
                break;
            }
        }

        if (index <= 0)
            return;

        if (m_createIn->currentIndex() != index)
            m_createIn->setCurrentIndex(index);
    }

    m_name->setFocus();
}

void NewBasketDialog::returnPressed()
{
    okButton->animateClick();
}

int NewBasketDialog::populateBasketsList(QTreeWidgetItem *item, int indent, int index)
{
    static const int ICON_SIZE = 16;
    // Get the basket data:
    BasketScene *basket = ((BasketListViewItem *)item)->basket();
    QPixmap icon = KIconLoader::global()->loadIcon(basket->icon(),
                                                   KIconLoader::NoGroup,
                                                   ICON_SIZE,
                                                   KIconLoader::DefaultState,
                                                   QStringList(),
                                                   nullptr,
                                                   /*canReturnNull=*/false);
    icon = Tools::indentPixmap(icon, indent, 2 * ICON_SIZE / 3);
    m_createIn->addItem(icon, basket->basketName());
    m_basketsMap.insert(index, basket);
    ++index;

    for (int i = 0; i < item->childCount(); i++) {
        // Append children of item to the list:
        index = populateBasketsList(item->child(i), indent + 1, index);
    }

    return index;
}

NewBasketDialog::~NewBasketDialog() = default;

void NewBasketDialog::ensurePolished()
{
    QDialog::ensurePolished();
    m_name->setFocus();
}

void NewBasketDialog::nameChanged(const QString &newName)
{
    okButton->setEnabled(!newName.isEmpty());
}

void NewBasketDialog::slotOk()
{
    QListWidgetItem *item = ((SingleSelectionKIconView *)m_templates)->selectedItem();
    QString templateName;
    if (!item)
        return;
    if (item->text() == i18n("One column"))
        templateName = QStringLiteral("1column");
    if (item->text() == i18n("Two columns"))
        templateName = QStringLiteral("2columns");
    if (item->text() == i18n("Three columns"))
        templateName = QStringLiteral("3columns");
    if (item->text() == i18n("Free-form"))
        templateName = QStringLiteral("free");
    if (item->text() == i18n("Mind map"))
        templateName = QStringLiteral("mindmap");

    Global::bnpView->closeAllEditors();

    QString backgroundImage;
    QColor textColor;
    if (m_backgroundColor->color() == m_defaultProperties.backgroundColor) {
        backgroundImage = m_defaultProperties.backgroundImage;
        textColor = m_defaultProperties.textColor;
    }

    BasketFactory::newBasket(m_icon->icon(),
                             m_name->text(),
                             m_basketsMap[m_createIn->currentIndex()],
                             backgroundImage,
                             m_backgroundColor->color(),
                             textColor,
                             templateName);

    if (Global::activeMainWindow())
        Global::activeMainWindow()->show();
}

void NewBasketDialog::manageTemplates()
{
    KMessageBox::information(this, QStringLiteral("Wait a minute! There is no template for now: they will come with time... :-D"));
}

#include "moc_newbasketdialog.cpp"
