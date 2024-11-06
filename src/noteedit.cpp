/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "noteedit.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QGraphicsProxyWidget>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QTextCharFormat>

#include <KActionCollection>
#include <KColorCombo>
#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KIconButton>
#include <KLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KService>
#include <KToggleAction>
#include <KToolBar>
#include <KUrlRequester>

#include "basketlistview.h"
#include "basketscene.h"
#include "focusedwidgets.h"
#include "icon_names.h"
#include "note.h"
#include "notecontent.h"
#include "notefactory.h"
#include "settings.h"
#include "tools.h"
#include "variouswidgets.h"

/** class NoteEditor: */

NoteEditor::NoteEditor(NoteContent *noteContent)
{
    m_isEmpty = false;
    m_canceled = false;
    m_widget = nullptr;
    m_textEdit = nullptr;
    m_lineEdit = nullptr;
    m_noteContent = noteContent;
}

NoteEditor::~NoteEditor()
{
    delete m_widget;
}

Note *NoteEditor::note()
{
    return m_noteContent->note();
}

void NoteEditor::setCursorTo(const QPointF &pos)
{
    // clicked comes from the QMouseEvent, which is in item's coordinate system.
    if (m_textEdit) {
        QPointF currentPos = note()->mapFromScene(pos);
        QPointF deltaPos = m_textEdit->pos() - note()->pos();
        m_textEdit->setTextCursor(m_textEdit->cursorForPosition((currentPos - deltaPos).toPoint()));
    }
}

void NoteEditor::startSelection(const QPointF &pos)
{
    if (m_textEdit) {
        QPointF currentPos = note()->mapFromScene(pos);
        QPointF deltaPos = m_textEdit->pos() - note()->pos();
        m_textEdit->setTextCursor(m_textEdit->cursorForPosition((currentPos - deltaPos).toPoint()));
    }
}

void NoteEditor::updateSelection(const QPointF &pos)
{
    if (m_textEdit) {
        QPointF currentPos = note()->mapFromScene(pos);
        QPointF deltaPos = m_textEdit->pos() - note()->pos();

        QTextCursor cursor = m_textEdit->cursorForPosition((currentPos - deltaPos).toPoint());
        QTextCursor currentCursor = m_textEdit->textCursor();
        // select the text
        currentCursor.setPosition(cursor.position(), QTextCursor::KeepAnchor);
        // update the cursor
        m_textEdit->setTextCursor(currentCursor);
    }
}

void NoteEditor::endSelection(const QPointF & /*pos*/)
{
    // For TextEdit inside GraphicsScene selectionChanged() is only generated for the first selected char -
    // thus we need to call it manually after selection is finished
    if (FocusedTextEdit *textEdit = dynamic_cast<FocusedTextEdit *>(m_textEdit))
        textEdit->onSelectionChanged();
}

void NoteEditor::paste(const QPointF &pos, QClipboard::Mode mode)
{
    if (FocusedTextEdit *textEdit = dynamic_cast<FocusedTextEdit *>(m_textEdit)) {
        setCursorTo(pos);
        textEdit->paste(mode);
    }
}

void NoteEditor::connectActions(BasketScene *scene)
{
    if (m_textEdit) {
        connect(m_textEdit, SIGNAL(textChanged()), scene, SLOT(selectionChangedInEditor()));
        connect(m_textEdit, SIGNAL(textChanged()), scene, SLOT(contentChangedInEditor()));
        connect(m_textEdit, &KTextEdit::textChanged, scene, &BasketScene::placeEditorAndEnsureVisible);
        connect(m_textEdit, SIGNAL(selectionChanged()), scene, SLOT(selectionChangedInEditor()));

    } else if (m_lineEdit) {
        connect(m_lineEdit, SIGNAL(textChanged(const QString &)), scene, SLOT(selectionChangedInEditor()));
        connect(m_lineEdit, SIGNAL(textChanged(const QString &)), scene, SLOT(contentChangedInEditor()));
        connect(m_lineEdit, SIGNAL(selectionChanged()), scene, SLOT(selectionChangedInEditor()));
    }
}

NoteEditor *NoteEditor::editNoteContent(NoteContent *noteContent, QWidget *parent)
{
    TextContent *textContent = dynamic_cast<TextContent *>(noteContent);
    if (textContent)
        return new TextEditor(textContent, parent);

    HtmlContent *htmlContent = dynamic_cast<HtmlContent *>(noteContent);
    if (htmlContent)
        return new HtmlEditor(htmlContent, parent);

    ImageContent *imageContent = dynamic_cast<ImageContent *>(noteContent);
    if (imageContent)
        return new ImageEditor(imageContent, parent);

    AnimationContent *animationContent = dynamic_cast<AnimationContent *>(noteContent);
    if (animationContent)
        return new AnimationEditor(animationContent, parent);

    FileContent *fileContent = dynamic_cast<FileContent *>(noteContent); // Same for SoundContent
    if (fileContent)
        return new FileEditor(fileContent, parent);

    LinkContent *linkContent = dynamic_cast<LinkContent *>(noteContent);
    if (linkContent)
        return new LinkEditor(linkContent, parent);

    CrossReferenceContent *crossReferenceContent = dynamic_cast<CrossReferenceContent *>(noteContent);
    if (crossReferenceContent)
        return new CrossReferenceEditor(crossReferenceContent, parent);

    LauncherContent *launcherContent = dynamic_cast<LauncherContent *>(noteContent);
    if (launcherContent)
        return new LauncherEditor(launcherContent, parent);

    ColorContent *colorContent = dynamic_cast<ColorContent *>(noteContent);
    if (colorContent)
        return new ColorEditor(colorContent, parent);

    UnknownContent *unknownContent = dynamic_cast<UnknownContent *>(noteContent);
    if (unknownContent)
        return new UnknownEditor(unknownContent, parent);

    return nullptr;
}

void NoteEditor::setInlineEditor(QWidget *inlineEditor)
{
    if (!m_widget) {
        m_widget = new QGraphicsProxyWidget();
    }
    m_widget->setWidget(inlineEditor);
    m_widget->setZValue(500);
    // m_widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_textEdit = nullptr;
    m_lineEdit = nullptr;
    KTextEdit *textEdit = dynamic_cast<KTextEdit *>(inlineEditor);
    if (textEdit) {
        m_textEdit = textEdit;
    } else {
        QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(inlineEditor);
        if (lineEdit) {
            m_lineEdit = lineEdit;
        }
    }
}

/** class TextEditor: */

TextEditor::TextEditor(TextContent *textContent, QWidget *parent)
    : NoteEditor(textContent)
    , m_textContent(textContent)
{
    FocusedTextEdit *textEdit = new FocusedTextEdit(/*disableUpdatesOnKeyPress=*/true, parent);
    textEdit->setLineWidth(0);
    textEdit->setMidLineWidth(0);
    textEdit->setFrameStyle(QFrame::Box);
    QPalette palette;
    palette.setColor(textEdit->backgroundRole(), note()->backgroundColor());
    palette.setColor(textEdit->foregroundRole(), note()->textColor());
    textEdit->setPalette(palette);

    textEdit->setFont(note()->font());
    textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (Settings::spellCheckTextNotes())
        textEdit->setCheckSpellingEnabled(true);
    textEdit->setPlainText(m_textContent->text());

    // Not sure if the following comment is still true
    // FIXME: Sometimes, the cursor flicker at ends before being positioned where clicked (because qApp->processEvents() I think)
    textEdit->moveCursor(QTextCursor::End);
    textEdit->verticalScrollBar()->setCursor(Qt::ArrowCursor);
    setInlineEditor(textEdit);
    connect(textEdit, &FocusedTextEdit::escapePressed, this, &TextEditor::askValidation);
    connect(textEdit, &FocusedTextEdit::mouseEntered, this, &TextEditor::mouseEnteredEditorWidget);

    connect(textEdit, &FocusedTextEdit::cursorPositionChanged, textContent->note()->basket(), &BasketScene::editorCursorPositionChanged);
    // In case it is a very big note, the top is displayed and Enter is pressed: the cursor is on bottom, we should enure it visible:
    QTimer::singleShot(0, textContent->note()->basket(), SLOT(editorCursorPositionChanged()));
}

TextEditor::~TextEditor()
{
    delete graphicsWidget()->widget(); // TODO: delete that in validate(), so we can remove one method
}

void TextEditor::autoSave(bool toFileToo)
{
    bool autoSpellCheck = true;
    if (toFileToo) {
        if (Settings::spellCheckTextNotes() != textEdit()->checkSpellingEnabled()) {
            Settings::setSpellCheckTextNotes(textEdit()->checkSpellingEnabled());
            Settings::saveConfig();
        }

        autoSpellCheck = textEdit()->checkSpellingEnabled();
        textEdit()->setCheckSpellingEnabled(false);
    }

    m_textContent->setText(textEdit()->toPlainText());

    if (toFileToo) {
        m_textContent->saveToFile();
        m_textContent->setEdited();
        textEdit()->setCheckSpellingEnabled(autoSpellCheck);
    }
}

void TextEditor::validate()
{
    if (Settings::spellCheckTextNotes() != textEdit()->checkSpellingEnabled()) {
        Settings::setSpellCheckTextNotes(textEdit()->checkSpellingEnabled());
        Settings::saveConfig();
    }

    textEdit()->setCheckSpellingEnabled(false);
    if (textEdit()->document()->isEmpty())
        setEmpty();
    m_textContent->setText(textEdit()->toPlainText());
    m_textContent->saveToFile();
    m_textContent->setEdited();

    note()->setWidth(0);
}

/** class HtmlEditor: */

HtmlEditor::HtmlEditor(HtmlContent *htmlContent, QWidget *parent)
    : NoteEditor(htmlContent)
    , m_htmlContent(htmlContent)
{
    FocusedTextEdit *textEdit = new FocusedTextEdit(/*disableUpdatesOnKeyPress=*/true, parent);
    textEdit->setLineWidth(0);
    textEdit->setMidLineWidth(0);
    textEdit->setFrameStyle(QFrame::Box);
    textEdit->setAutoFormatting(Settings::autoBullet() ? QTextEdit::AutoAll : QTextEdit::AutoNone);

    QPalette palette;
    palette.setColor(textEdit->backgroundRole(), note()->backgroundColor());
    palette.setColor(textEdit->foregroundRole(), note()->textColor());
    textEdit->setPalette(palette);

    textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEdit->setHtml(Tools::detectCrossReferences(m_htmlContent->html(), /*userLink=*/true));
    textEdit->moveCursor(QTextCursor::End);
    textEdit->verticalScrollBar()->setCursor(Qt::ArrowCursor);
    setInlineEditor(textEdit);

    connect(textEdit, &FocusedTextEdit::mouseEntered, this, &HtmlEditor::mouseEnteredEditorWidget);
    connect(textEdit, &FocusedTextEdit::escapePressed, this, &HtmlEditor::askValidation);

    connect(InlineEditors::instance()->richTextFont, &QFontComboBox::currentFontChanged, this, &HtmlEditor::onFontSelectionChanged);
    connect(InlineEditors::instance()->richTextFontSize, &FontSizeCombo::sizeChanged, textEdit, &FocusedTextEdit::setFontPointSize);
    connect(InlineEditors::instance()->richTextColor, &KColorCombo::activated, textEdit, &FocusedTextEdit::setTextColor);

    connect(InlineEditors::instance()->focusWidgetFilter, &FocusWidgetFilter::escapePressed, textEdit, [&textEdit]() { textEdit->setFocus(); });
    connect(InlineEditors::instance()->focusWidgetFilter, &FocusWidgetFilter::returnPressed, textEdit, [&textEdit]() { textEdit->setFocus(); });
    connect(InlineEditors::instance()->richTextFont, SIGNAL(activated(int)), textEdit, SLOT(setFocus()));

    connect(InlineEditors::instance()->richTextFontSize, SIGNAL(activated(int)), textEdit, SLOT(setFocus()));

    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    connect(textEdit, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(charFormatChanged(const QTextCharFormat &)));
    //  connect( textEdit,  SIGNAL(currentVerticalAlignmentChanged(VerticalAlignment)), this, SLOT(slotVerticalAlignmentChanged()) );

    connect(InlineEditors::instance()->richTextBold, SIGNAL(triggered(bool)), this, SLOT(setBold(bool)));
    connect(InlineEditors::instance()->richTextItalic, SIGNAL(triggered(bool)), textEdit, SLOT(setFontItalic(bool)));
    connect(InlineEditors::instance()->richTextUnderline, SIGNAL(triggered(bool)), textEdit, SLOT(setFontUnderline(bool)));
    connect(InlineEditors::instance()->richTextLeft, SIGNAL(triggered()), this, SLOT(setLeft()));
    connect(InlineEditors::instance()->richTextCenter, SIGNAL(triggered()), this, SLOT(setCentered()));
    connect(InlineEditors::instance()->richTextRight, SIGNAL(triggered()), this, SLOT(setRight()));
    connect(InlineEditors::instance()->richTextJustified, SIGNAL(triggered()), this, SLOT(setBlock()));

    //  InlineEditors::instance()->richTextToolBar()->show();
    cursorPositionChanged();
    charFormatChanged(textEdit->currentCharFormat());
    // QTimer::singleShot( 0, this, SLOT(cursorPositionChanged()) );
    InlineEditors::instance()->enableRichTextToolBar();

    connect(InlineEditors::instance()->richTextUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));
    connect(InlineEditors::instance()->richTextRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));
    connect(textEdit, SIGNAL(undoAvailable(bool)), InlineEditors::instance()->richTextUndo, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(redoAvailable(bool)), InlineEditors::instance()->richTextRedo, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(textChanged()), this, SLOT(editTextChanged()));
    InlineEditors::instance()->richTextUndo->setEnabled(false);
    InlineEditors::instance()->richTextRedo->setEnabled(false);

    connect(textEdit, SIGNAL(cursorPositionChanged()), htmlContent->note()->basket(), SLOT(editorCursorPositionChanged()));
    // In case it is a very big note, the top is displayed and Enter is pressed: the cursor is on bottom, we should enure it visible:
    QTimer::singleShot(0, htmlContent->note()->basket(), SLOT(editorCursorPositionChanged()));
}

void HtmlEditor::cursorPositionChanged()
{
    InlineEditors::instance()->richTextFont->setCurrentFont(textEdit()->currentFont().family());
    if (InlineEditors::instance()->richTextColor->color() != textEdit()->textColor())
        InlineEditors::instance()->richTextColor->setColor(textEdit()->textColor());
    InlineEditors::instance()->richTextBold->setChecked((textEdit()->fontWeight() >= QFont::Bold));
    InlineEditors::instance()->richTextItalic->setChecked(textEdit()->fontItalic());
    InlineEditors::instance()->richTextUnderline->setChecked(textEdit()->fontUnderline());

    switch (textEdit()->alignment()) {
    default:
    case 1 /*Qt::AlignLeft*/:
        InlineEditors::instance()->richTextLeft->setChecked(true);
        break;
    case 2 /*Qt::AlignRight*/:
        InlineEditors::instance()->richTextRight->setChecked(true);
        break;
    case 4 /*Qt::AlignHCenter*/:
        InlineEditors::instance()->richTextCenter->setChecked(true);
        break;
    case 8 /*Qt::AlignJustify*/:
        InlineEditors::instance()->richTextJustified->setChecked(true);
        break;
    }
}

void HtmlEditor::editTextChanged()
{
    // The following is a workaround for an apparent Qt bug.
    // When I start typing in a textEdit, the undo&redo actions are not enabled until I click
    // or move the cursor - probably, the signal undoAvailable() is not emitted.
    // So, I had to intervene and do that manually.
    InlineEditors::instance()->richTextUndo->setEnabled(textEdit()->document()->isUndoAvailable());
    InlineEditors::instance()->richTextRedo->setEnabled(textEdit()->document()->isRedoAvailable());
}

void HtmlEditor::charFormatChanged(const QTextCharFormat &format)
{
    InlineEditors::instance()->richTextFontSize->setFontSize(format.font().pointSize());
}

/*void HtmlEditor::slotVerticalAlignmentChanged(QTextEdit::VerticalAlignment align)
{
    QTextEdit::VerticalAlignment align = textEdit()
    switch (align) {
        case KTextEdit::AlignSuperScript:
            InlineEditors::instance()->richTextSuper->setChecked(true);
            InlineEditors::instance()->richTextSub->setChecked(false);
            break;
        case KTextEdit::AlignSubScript:
            InlineEditors::instance()->richTextSuper->setChecked(false);
            InlineEditors::instance()->richTextSub->setChecked(true);
            break;
        default:
            InlineEditors::instance()->richTextSuper->setChecked(false);
            InlineEditors::instance()->richTextSub->setChecked(false);
    }

    NoteHtmlEditor::buttonToggled(int id) :
        case 106:
            if (isChecked && m_toolbar->isButtonOn(107))
                m_toolbar->setButton(107, false);
            m_text->setVerticalAlignment(isChecked ? KTextEdit::AlignSuperScript : KTextEdit::AlignNormal);
            break;
        case 107:
            if (isChecked && m_toolbar->isButtonOn(106))
                m_toolbar->setButton(106, false);
            m_text->setVerticalAlignment(isChecked ? KTextEdit::AlignSubScript   : KTextEdit::AlignNormal);
            break;
}*/

void HtmlEditor::setLeft()
{
    textEdit()->setAlignment(Qt::AlignLeft);
}
void HtmlEditor::setRight()
{
    textEdit()->setAlignment(Qt::AlignRight);
}
void HtmlEditor::setCentered()
{
    textEdit()->setAlignment(Qt::AlignHCenter);
}
void HtmlEditor::setBlock()
{
    textEdit()->setAlignment(Qt::AlignJustify);
}

void HtmlEditor::onFontSelectionChanged(const QFont &font)
{
    // Change font family only
    textEdit()->setFontFamily(font.family());
    InlineEditors::instance()->richTextFont->clearFocus();
    // textEdit()->setFocus();
}

void HtmlEditor::setBold(bool isChecked)
{
    qWarning() << "setBold " << isChecked;
    textEdit()->setFontWeight(isChecked ? QFont::Bold : QFont::Normal);
}

HtmlEditor::~HtmlEditor()
{
    // delete graphicsWidget()->widget();
}

void HtmlEditor::autoSave(bool toFileToo)
{
    m_htmlContent->setHtml(Tools::textDocumentToMinimalHTML(textEdit()->document()));
    if (toFileToo) {
        m_htmlContent->saveToFile();
        m_htmlContent->setEdited();
    }
}

void HtmlEditor::validate()
{
    if (Tools::htmlToText(textEdit()->toHtml()).isEmpty())
        setEmpty();
    if (Settings::detectTextTags())
        detectTags();
    QString convert = Tools::textDocumentToMinimalHTML(textEdit()->document());
    QString textEquivalent = Tools::htmlToText(convert);

    if (note()->allowCrossReferences())
        convert = Tools::detectCrossReferences(convert, /*userLink=*/true);

    m_htmlContent->setHtml(convert);
    m_htmlContent->saveToFile();
    m_htmlContent->setEdited();

    disconnect();
    graphicsWidget()->disconnect();
    if (InlineEditors::instance()) {
        InlineEditors::instance()->disableRichTextToolBar();
        //      if (InlineEditors::instance()->richTextToolBar())
        //          InlineEditors::instance()->richTextToolBar()->hide();
    }

    if (graphicsWidget()) {
        note()->setZValue(1);
        delete graphicsWidget()->widget();
        setInlineEditor(nullptr);
    }
}

void HtmlEditor::detectTags()
{
    QTextDocument * doc = textEdit()->document();
    const QTextBlock& block = doc->firstBlock();
    if (!block.isValid() || block.begin() == block.end()) return;

    const QTextFragment& fragment = block.begin().fragment();
    if (!fragment.isValid() || fragment.length() == 0) return;
    //Process unstyled text only
    const QTextCharFormat& charFmt = fragment.charFormat();
    if (charFmt.propertyCount() > 0) return;

    QString newText = fragment.text();
    int prefixLength;
    QTextCursor cursor(block);
    const QList<State*>& states = Tools::detectTags(fragment.text(), prefixLength);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, prefixLength);
    cursor.removeSelectedText();

    for (State* state: states)
    {
        note()->addState(state, true);
    }
}
/** class ImageEditor: */

ImageEditor::ImageEditor(ImageContent *imageContent, QWidget *parent)
    : NoteEditor(imageContent)
{
    int choice = KMessageBox::questionTwoActionsCancel(parent,
                                                  i18n("Images can not be edited here at the moment (the next version of BasKet Note Pads will include an image editor).\n"
                                                       "Do you want to open it with an application that understand it?"),
                                                  i18n("Edit Image Note"),
                                                  KStandardGuiItem::open(),
                                                  KGuiItem(i18n("Load From &File..."), QString::fromUtf8(IconNames::DOCUMENT_IMPORT)),
                                                  KStandardGuiItem::cancel());

    switch (choice) {
    case (KMessageBox::Ok):
        note()->basket()->noteOpen(note());
        break;
    case (KMessageBox::SecondaryAction): // Load from file
        cancel();
        Global::bnpView->insertWizard(3); // 3 maps to m_actLoadFile
        break;
    case (KMessageBox::Cancel):
        cancel();
        break;
    }
}

/** class AnimationEditor: */

AnimationEditor::AnimationEditor(AnimationContent *animationContent, QWidget *parent)
    : NoteEditor(animationContent)
{
    int choice = KMessageBox::questionTwoActions(parent,
                                            i18n("This animated image can not be edited here.\n"
                                                 "Do you want to open it with an application that understands it?"),
                                            i18n("Edit Animation Note"),
                                            KStandardGuiItem::open(),
                                            KStandardGuiItem::cancel());

    if (choice == KMessageBox::PrimaryAction)
        note()->basket()->noteOpen(note());
}

/** class FileEditor: */

FileEditor::FileEditor(FileContent *fileContent, QWidget *parent)
    : NoteEditor(fileContent)
    , m_fileContent(fileContent)
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    FocusWidgetFilter *filter = new FocusWidgetFilter(lineEdit);

    QPalette palette;
    palette.setColor(lineEdit->backgroundRole(), note()->backgroundColor());
    palette.setColor(lineEdit->foregroundRole(), note()->textColor());
    lineEdit->setPalette(palette);

    lineEdit->setFont(note()->font());
    lineEdit->setText(m_fileContent->fileName());
    lineEdit->selectAll();
    setInlineEditor(lineEdit);
    connect(filter, SIGNAL(returnPressed()), this, SIGNAL(askValidation()));
    connect(filter, SIGNAL(escapePressed()), this, SIGNAL(askValidation()));
    connect(filter, SIGNAL(mouseEntered()), this, SIGNAL(mouseEnteredEditorWidget()));
}

FileEditor::~FileEditor()
{
    delete graphicsWidget()->widget();
}

void FileEditor::autoSave(bool toFileToo)
{
    // FIXME: How to detect cancel?
    if (toFileToo && !lineEdit()->text().isEmpty() && m_fileContent->trySetFileName(lineEdit()->text())) {
        m_fileContent->setFileName(lineEdit()->text());
        m_fileContent->setEdited();
    }
}

void FileEditor::validate()
{
    autoSave(/*toFileToo=*/true);
}

/** class LinkEditor: */

LinkEditor::LinkEditor(LinkContent *linkContent, QWidget *parent)
    : NoteEditor(linkContent)
{
    QPointer<LinkEditDialog> dialog = new LinkEditDialog(linkContent, parent);
    if (dialog->exec() == QDialog::Rejected)
        cancel();
    if (linkContent->url().isEmpty() && linkContent->title().isEmpty())
        setEmpty();
}

/** class CrossReferenceEditor: */

CrossReferenceEditor::CrossReferenceEditor(CrossReferenceContent *crossReferenceContent, QWidget *parent)
    : NoteEditor(crossReferenceContent)
{
    QPointer<CrossReferenceEditDialog> dialog = new CrossReferenceEditDialog(crossReferenceContent, parent);
    if (dialog->exec() == QDialog::Rejected)
        cancel();
    if (crossReferenceContent->url().isEmpty() && crossReferenceContent->title().isEmpty())
        setEmpty();
}

/** class LauncherEditor: */

LauncherEditor::LauncherEditor(LauncherContent *launcherContent, QWidget *parent)
    : NoteEditor(launcherContent)
{
    QPointer<LauncherEditDialog> dialog = new LauncherEditDialog(launcherContent, parent);
    if (dialog->exec() == QDialog::Rejected)
        cancel();
    if (launcherContent->name().isEmpty() && launcherContent->exec().isEmpty())
        setEmpty();
}

/** class ColorEditor: */

ColorEditor::ColorEditor(ColorContent *colorContent, QWidget *parent)
    : NoteEditor(colorContent)
{
    QPointer<QColorDialog> dialog = new QColorDialog(parent);
    dialog->setCurrentColor(colorContent->color());
    dialog->setWindowTitle(i18n("Edit Color Note"));
    // dialog->setButtons(QDialog::Ok | QDialog::Cancel);
    if (dialog->exec() == QDialog::Accepted) {
        if (dialog->currentColor() != colorContent->color()) {
            colorContent->setColor(dialog->currentColor());
            colorContent->setEdited();
        }
    } else
        cancel();

    /* This code don't allow to set a caption to the dialog:
    QColor color = colorContent()->color();
    color = QColorDialog::getColor(parent)==QDialog::Accepted&&color!=m_color);
    if ( color.isValid() ) {
        colorContent()->setColor(color);
        setEdited();
    }*/
}

/** class UnknownEditor: */

UnknownEditor::UnknownEditor(UnknownContent *unknownContent, QWidget *parent)
    : NoteEditor(unknownContent)
{
    KMessageBox::information(parent,
                             i18n("The type of this note is unknown and can not be edited here.\n"
                                  "You however can drag or copy the note into an application that understands it."),
                             i18n("Edit Unknown Note"));
}

/*********************************************************************/

/** class LinkEditDialog: */

LinkEditDialog::LinkEditDialog(LinkContent *contentNote, QWidget *parent /*, QKeyEvent *ke*/)
    : QDialog(parent)
    , m_noteContent(contentNote)
{
    // QDialog options
    setWindowTitle(i18n("Edit Link Note"));

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);

    setObjectName("EditLink");
    setModal(true);

    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);
    // QGridLayout *layout = new QGridLayout(page, /*nRows=*/4, /*nCols=*/2, /*margin=*/0, spacingHint());
    QGridLayout *layout = new QGridLayout(page);
    mainLayout->addLayout(layout);

    QWidget *wid1 = new QWidget(page);
    mainLayout->addWidget(wid1);
    QHBoxLayout *titleLay = new QHBoxLayout(wid1);
    m_title = new QLineEdit(m_noteContent->title(), wid1);
    m_autoTitle = new QPushButton(i18n("Auto"), wid1);
    m_autoTitle->setCheckable(true);
    m_autoTitle->setChecked(m_noteContent->autoTitle());
    titleLay->addWidget(m_title);
    titleLay->addWidget(m_autoTitle);

    QWidget *wid = new QWidget(page);
    mainLayout->addWidget(wid);
    QHBoxLayout *hLay = new QHBoxLayout(wid);
    m_icon = new KIconButton(wid);
    QLabel *label3 = new QLabel(page);
    mainLayout->addWidget(label3);
    label3->setText(i18n("&Icon:"));
    label3->setBuddy(m_icon);

    if (m_noteContent->url().isEmpty()) {
        m_url = new KUrlRequester(QUrl(QString()), wid);
        m_url->setMode(KFile::File | KFile::ExistingOnly);
    } else {
        m_url = new KUrlRequester(QUrl(m_noteContent->url().toDisplayString()), wid);
        m_url->setMode(KFile::File | KFile::ExistingOnly);
    }

    if (m_noteContent->title().isEmpty()) {
        m_title->setText(QString());
    } else {
        m_title->setText(m_noteContent->title());
    }

    QUrl filteredURL = NoteFactory::filteredURL(QUrl::fromUserInput(m_url->lineEdit()->text())); // KURIFilter::self()->filteredURI(KUrl(m_url->lineEdit()->text()));
    m_icon->setIconType(KIconLoader::NoGroup, KIconLoader::MimeType);
    m_icon->setIconSize(LinkLook::lookForURL(filteredURL)->iconSize());
    m_autoIcon = new QPushButton(i18n("Auto"), wid); // Create before to know size here:
    /* Icon button: */
    m_icon->setIcon(m_noteContent->icon());
    int minSize = m_autoIcon->sizeHint().height();
    // Make the icon button at least the same height than the other buttons for a better alignment (nicer to the eyes):
    if (m_icon->sizeHint().height() < minSize)
        m_icon->setFixedSize(minSize, minSize);
    else
        m_icon->setFixedSize(m_icon->sizeHint().height(), m_icon->sizeHint().height()); // Make it square
    /* Auto button: */
    m_autoIcon->setCheckable(true);
    m_autoIcon->setChecked(m_noteContent->autoIcon());
    hLay->addWidget(m_icon);
    hLay->addWidget(m_autoIcon);
    hLay->addStretch();

    m_url->lineEdit()->setMinimumWidth(m_url->lineEdit()->fontMetrics().maxWidth() * 20);
    m_title->setMinimumWidth(m_title->fontMetrics().maxWidth() * 20);

    // m_url->setShowLocalProtocol(true);
    QLabel *label1 = new QLabel(page);
    mainLayout->addWidget(label1);
    label1->setText(i18n("Ta&rget:"));
    label1->setBuddy(m_url);

    QLabel *label2 = new QLabel(page);
    mainLayout->addWidget(label2);
    label2->setText(i18n("&Title:"));
    label2->setBuddy(m_title);

    layout->addWidget(label1, 0, 0, Qt::AlignVCenter);
    layout->addWidget(label2, 1, 0, Qt::AlignVCenter);
    layout->addWidget(label3, 2, 0, Qt::AlignVCenter);
    layout->addWidget(m_url, 0, 1, Qt::AlignVCenter);
    layout->addWidget(wid1, 1, 1, Qt::AlignVCenter);
    layout->addWidget(wid, 2, 1, Qt::AlignVCenter);

    m_isAutoModified = false;
    connect(m_url, &KUrlRequester::textChanged, this, &LinkEditDialog::urlChanged);
    connect(m_title, &QLineEdit::textChanged, this, &LinkEditDialog::doNotAutoTitle);
    connect(m_icon, &KIconButton::iconChanged, this, &LinkEditDialog::doNotAutoIcon);
    connect(m_autoTitle, &QPushButton::clicked, this, [this](bool) { guessTitle(); });
    connect(m_autoIcon, &QPushButton::clicked, this, [this](bool) { guessIcon(); });

    QWidget *stretchWidget = new QWidget(page);
    mainLayout->addWidget(stretchWidget);
    QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    policy.setHorizontalStretch(1);
    policy.setVerticalStretch(255);
    stretchWidget->setSizePolicy(policy); // Make it fill ALL vertical space
    layout->addWidget(stretchWidget, 3, 1, Qt::AlignVCenter);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(okButton, SIGNAL(clicked()), SLOT(slotOk()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    // urlChanged(QString());

    //  if (ke)
    //      qApp->postEvent(m_url->lineEdit(), ke);
}

LinkEditDialog::~LinkEditDialog()
{
}

void LinkEditDialog::ensurePolished()
{
    QDialog::ensurePolished();
    if (m_url->lineEdit()->text().isEmpty()) {
        m_url->setFocus();
        m_url->lineEdit()->end(false);
    } else {
        m_title->setFocus();
        m_title->end(false);
    }
}

void LinkEditDialog::urlChanged(const QString &)
{
    m_isAutoModified = true;
    //  guessTitle();
    //  guessIcon();
    // Optimization (filter only once):
    QUrl filteredURL = NoteFactory::filteredURL(m_url->url()); // KURIFilter::self()->filteredURI(KUrl(m_url->url()));
    if (m_autoIcon->isChecked())
        m_icon->setIcon(NoteFactory::iconForURL(filteredURL));
    if (m_autoTitle->isChecked()) {
        m_title->setText(NoteFactory::titleForURL(filteredURL));
        m_autoTitle->setChecked(true); // Because the setText() will disable it!
    }
}

void LinkEditDialog::doNotAutoTitle(const QString &)
{
    if (m_isAutoModified)
        m_isAutoModified = false;
    else
        m_autoTitle->setChecked(false);
}

void LinkEditDialog::doNotAutoIcon(QString)
{
    m_autoIcon->setChecked(false);
}

void LinkEditDialog::guessIcon()
{
    if (m_autoIcon->isChecked()) {
        QUrl filteredURL = NoteFactory::filteredURL(m_url->url()); // KURIFilter::self()->filteredURI(KUrl(m_url->url()));
        m_icon->setIcon(NoteFactory::iconForURL(filteredURL));
    }
}

void LinkEditDialog::guessTitle()
{
    if (m_autoTitle->isChecked()) {
        QUrl filteredURL = NoteFactory::filteredURL(m_url->url()); // KURIFilter::self()->filteredURI(KUrl(m_url->url()));
        m_title->setText(NoteFactory::titleForURL(filteredURL));
        m_autoTitle->setChecked(true); // Because the setText() will disable it!
    }
}

void LinkEditDialog::slotOk()
{
    QUrl filteredURL = NoteFactory::filteredURL(m_url->url()); // KURIFilter::self()->filteredURI(KUrl(m_url->url()));
    m_noteContent->setLink(filteredURL, m_title->text(), m_icon->icon(), m_autoTitle->isChecked(), m_autoIcon->isChecked());
    m_noteContent->setEdited();

    /* Change icon size if link look have changed */
    LinkLook *linkLook = LinkLook::lookForURL(filteredURL);
    QString icon = m_icon->icon();                                     // When we change size, icon isn't changed and keep it's old size
    m_icon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum); // Reset size policy
    m_icon->setIconSize(linkLook->iconSize());                         //  So I store it's name and reload it after size change !
    m_icon->setIcon(icon);
    int minSize = m_autoIcon->sizeHint().height();
    // Make the icon button at least the same height than the other buttons for a better alignment (nicer to the eyes):
    if (m_icon->sizeHint().height() < minSize)
        m_icon->setFixedSize(minSize, minSize);
    else
        m_icon->setFixedSize(m_icon->sizeHint().height(), m_icon->sizeHint().height()); // Make it square
}

/** class CrossReferenceEditDialog: */

CrossReferenceEditDialog::CrossReferenceEditDialog(CrossReferenceContent *contentNote, QWidget *parent /*, QKeyEvent *ke*/)
    : QDialog(parent)
    , m_noteContent(contentNote)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // QDialog options
    setWindowTitle(i18n("Edit Cross Reference"));

    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);
    QWidget *wid = new QWidget(page);
    mainLayout->addWidget(wid);

    QGridLayout *layout = new QGridLayout(page);
    mainLayout->addLayout(layout);

    m_targetBasket = new KComboBox(wid);
    this->generateBasketList(m_targetBasket);

    if (m_noteContent->url().isEmpty()) {
        BasketListViewItem *item = Global::bnpView->topLevelItem(0);
        m_noteContent->setCrossReference(QUrl::fromUserInput(item->data(0, Qt::UserRole).toString()), m_targetBasket->currentText(), QStringLiteral("edit-copy"));
        this->urlChanged(0);
    } else {
        QString url = m_noteContent->url().url();
        // cannot use findData because I'm using a StringList and I don't have the second
        // piece of data to make find work.
        for (int i = 0; i < m_targetBasket->count(); ++i) {
            if (url == m_targetBasket->itemData(i, Qt::UserRole).toStringList().first()) {
                m_targetBasket->setCurrentIndex(i);
                break;
            }
        }
    }

    QLabel *label1 = new QLabel(page);
    mainLayout->addWidget(label1);
    label1->setText(i18n("Ta&rget:"));
    label1->setBuddy(m_targetBasket);

    layout->addWidget(label1, 0, 0, Qt::AlignVCenter);
    layout->addWidget(m_targetBasket, 0, 1, Qt::AlignVCenter);

    connect(m_targetBasket, SIGNAL(activated(int)), this, SLOT(urlChanged(int)));

    QWidget *stretchWidget = new QWidget(page);
    mainLayout->addWidget(stretchWidget);
    QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    policy.setHorizontalStretch(1);
    policy.setVerticalStretch(255);
    stretchWidget->setSizePolicy(policy); // Make it fill ALL vertical space
    layout->addWidget(stretchWidget, 3, 1, Qt::AlignVCenter);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);
    setObjectName("EditCrossReference");
    setModal(true);
    connect(okButton, SIGNAL(clicked()), SLOT(slotOk()));
}

CrossReferenceEditDialog::~CrossReferenceEditDialog()
{
}

void CrossReferenceEditDialog::urlChanged(const int index)
{
    if (m_targetBasket)
        m_noteContent->setCrossReference(
            QUrl::fromUserInput(m_targetBasket->itemData(index, Qt::UserRole).toStringList().first()), m_targetBasket->currentText().trimmed(), m_targetBasket->itemData(index, Qt::UserRole).toStringList().last());
}

void CrossReferenceEditDialog::slotOk()
{
    m_noteContent->setEdited();
}

void CrossReferenceEditDialog::generateBasketList(KComboBox *targetList, BasketListViewItem *item, int indent)
{
    if (!item) { // include ALL top level items and their children.
        for (int i = 0; i < Global::bnpView->topLevelItemCount(); ++i)
            this->generateBasketList(targetList, Global::bnpView->topLevelItem(i));
    } else {
        BasketScene *bv = item->basket();

        // TODO: add some fancy deco stuff to make it look like a tree list.
        QString pad;
        QString text = item->text(0); // user text

        text.prepend(pad.fill(QLatin1Char(' '), indent * 2));

        // create the link text
        QString link = QStringLiteral("basket://");
        link.append(bv->folderName().toLower()); // unique ref.
        QStringList data;
        data.append(link);
        data.append(bv->icon());

        targetList->addItem(item->icon(0), text, QVariant(data));

        int subBasketCount = item->childCount();
        if (subBasketCount > 0) {
            indent++;
            for (int i = 0; i < subBasketCount; ++i) {
                this->generateBasketList(targetList, (BasketListViewItem *)item->child(i), indent);
            }
        }
    }
}

/** class LauncherEditDialog: */

LauncherEditDialog::LauncherEditDialog(LauncherContent *contentNote, QWidget *parent)
    : QDialog(parent)
    , m_noteContent(contentNote)
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    // QDialog options
    setWindowTitle(i18n("Edit Launcher Note"));

    setObjectName("EditLauncher");
    setModal(true);

    QWidget *page = new QWidget(this);
    mainLayout->addWidget(page);

    // QGridLayout *layout = new QGridLayout(page, /*nRows=*/4, /*nCols=*/2, /*margin=*/0, spacingHint());
    QGridLayout *layout = new QGridLayout(page);
    mainLayout->addLayout(layout);

    KService service(contentNote->fullPath());

    m_command = new ServiceLaunchRequester(service.storageId(), i18n("Choose a command to run:"), page);
    mainLayout->addWidget(m_command);
    m_name = new QLineEdit(service.name(), page);
    mainLayout->addWidget(m_name);

    QWidget *wid = new QWidget(page);
    mainLayout->addWidget(wid);
    QHBoxLayout *hLay = new QHBoxLayout(wid);
    m_icon = new KIconButton(wid);

    QLabel *label = new QLabel(page);
    mainLayout->addWidget(label);
    label->setText(i18n("&Icon:"));
    label->setBuddy(m_icon);

    m_icon->setIconType(KIconLoader::NoGroup, KIconLoader::Application);
    m_icon->setIconSize(LinkLook::launcherLook->iconSize());
    QPushButton *guessButton = new QPushButton(i18n("&Guess"), wid);
    /* Icon button: */
    m_icon->setIcon(service.icon());
    int minSize = guessButton->sizeHint().height();
    // Make the icon button at least the same height than the other buttons for a better alignment (nicer to the eyes):
    if (m_icon->sizeHint().height() < minSize)
        m_icon->setFixedSize(minSize, minSize);
    else
        m_icon->setFixedSize(m_icon->sizeHint().height(), m_icon->sizeHint().height()); // Make it square
    /* Guess button: */
    hLay->addWidget(m_icon);
    hLay->addWidget(guessButton);
    hLay->addStretch();

    // m_command->lineEdit()->setMinimumWidth(m_command->lineEdit()->fontMetrics().maxWidth() * 20);

    QLabel *label1 = new QLabel(page);
    mainLayout->addWidget(label1);
    label1->setText(i18n("Comman&d:"));
    // label1->setBuddy(m_command->lineEdit());

    QLabel *label2 = new QLabel(page);
    mainLayout->addWidget(label2);
    label2->setText(i18n("&Name:"));
    label2->setBuddy(m_name);

    layout->addWidget(label1, 0, 0, Qt::AlignVCenter);
    layout->addWidget(label2, 1, 0, Qt::AlignVCenter);
    layout->addWidget(label, 2, 0, Qt::AlignVCenter);
    layout->addWidget(m_command, 0, 1, Qt::AlignVCenter);
    layout->addWidget(m_name, 1, 1, Qt::AlignVCenter);
    layout->addWidget(wid, 2, 1, Qt::AlignVCenter);

    QWidget *stretchWidget = new QWidget(page);
    mainLayout->addWidget(stretchWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(okButton, SIGNAL(clicked()), SLOT(slotOk()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    policy.setHorizontalStretch(1);
    policy.setVerticalStretch(255);
    stretchWidget->setSizePolicy(policy); // Make it fill ALL vertical space

    layout->addWidget(stretchWidget, 3, 1, Qt::AlignVCenter);

    connect(guessButton, SIGNAL(clicked()), this, SLOT(guessIcon()));
}

LauncherEditDialog::~LauncherEditDialog()
{
}

void LauncherEditDialog::ensurePolished()
{
    QDialog::ensurePolished();
    if (m_command->serviceLauncher().isEmpty()) {
        m_command->setFocus();
    } else {
        m_name->setFocus();
        m_name->end(false);
    }
}

void LauncherEditDialog::slotOk()
{
    // TODO: Remember if a string has been modified AND IS DIFFERENT FROM THE
    // ORIGINAL!

    KDesktopFile dtFile(m_noteContent->fullPath());
    KConfigGroup grp = dtFile.desktopGroup();
    grp.writeEntry("Exec", m_command->serviceLauncher());
    grp.writeEntry("Name", m_name->text());
    grp.writeEntry("Icon", m_icon->icon());

    // Just for faster feedback: conf object will save to disk (and then
    // m_note->loadContent() called)
    m_noteContent->setLauncher(m_name->text(), m_icon->icon(), m_command->serviceLauncher());
    m_noteContent->setEdited();
}

void LauncherEditDialog::guessIcon()
{
    m_icon->setIcon(NoteFactory::iconForCommand(m_command->serviceLauncher()));
}

/** class InlineEditors: */

InlineEditors::InlineEditors()
{
}

InlineEditors::~InlineEditors()
{
}

InlineEditors *InlineEditors::instance()
{
    static InlineEditors *instance = nullptr;
    if (!instance)
        instance = new InlineEditors();
    return instance;
}

void InlineEditors::initToolBars(KActionCollection *ac)
{
    QFont defaultFont;
    QColor textColor = (Global::bnpView && Global::bnpView->currentBasket() ? Global::bnpView->currentBasket()->textColor() : palette().color(QPalette::Text));

    // NOTE: currently it is NULL since initToolBars is called early. Could use different way to get MainWindow pointer from main
    KMainWindow *parent = Global::activeMainWindow();

    // Init the RichTextEditor Toolbar:
    richTextFont = new QFontComboBox(Global::activeMainWindow());
    focusWidgetFilter = new FocusWidgetFilter(richTextFont);
    richTextFont->setFixedWidth(richTextFont->sizeHint().width() * 2 / 3);
    richTextFont->setCurrentFont(defaultFont.family());

    QWidgetAction *action = new QWidgetAction(parent);
    ac->addAction(QStringLiteral("richtext_font"), action);
    action->setDefaultWidget(richTextFont);
    action->setText(i18n("Font"));
    ac->setDefaultShortcut(action, Qt::Key_F6);

    richTextFontSize = new FontSizeCombo(/*rw=*/true, Global::activeMainWindow());
    richTextFontSize->setFontSize(defaultFont.pointSize());
    action = new QWidgetAction(parent);
    ac->addAction(QStringLiteral("richtext_font_size"), action);
    action->setDefaultWidget(richTextFontSize);
    action->setText(i18n("Font Size"));
    ac->setDefaultShortcut(action, Qt::Key_F7);

    richTextColor = new KColorCombo(Global::activeMainWindow());
    richTextColor->installEventFilter(focusWidgetFilter);
    richTextColor->setFixedWidth(richTextColor->sizeHint().height() * 2);
    richTextColor->setColor(textColor);
    action = new QWidgetAction(parent);
    ac->addAction(QStringLiteral("richtext_color"), action);
    action->setDefaultWidget(richTextColor);
    action->setText(i18n("Color"));

    KToggleAction *ta = nullptr;
    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_bold"), ta);
    ta->setText(i18n("Bold"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-text-bold")));
    ac->setDefaultShortcut(ta, QKeySequence(QStringLiteral("Ctrl+B")));
    richTextBold = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_italic"), ta);
    ta->setText(i18n("Italic"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-text-italic")));
    ac->setDefaultShortcut(ta, QKeySequence(QStringLiteral("Ctrl+I")));
    richTextItalic = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_underline"), ta);
    ta->setText(i18n("Underline"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-text-underline")));
    ac->setDefaultShortcut(ta, QKeySequence(QStringLiteral("Ctrl+U")));
    richTextUnderline = ta;

#if 0
    ta = new KToggleAction(ac);
    ac->addAction("richtext_super", ta);
    ta->setText(i18n("Superscript"));
    ta->setIcon(QIcon::fromTheme("text_super"));
    richTextSuper = ta;

    ta = new KToggleAction(ac);
    ac->addAction("richtext_sub", ta);
    ta->setText(i18n("Subscript"));
    ta->setIcon(QIcon::fromTheme("text_sub"));
    richTextSub = ta;
#endif

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_left"), ta);
    ta->setText(i18n("Align Left"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-justify-left")));
    richTextLeft = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_center"), ta);
    ta->setText(i18n("Centered"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-justify-center")));
    richTextCenter = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_right"), ta);
    ta->setText(i18n("Align Right"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-justify-right")));
    richTextRight = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_block"), ta);
    ta->setText(i18n("Justified"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("format-justify-fill")));
    richTextJustified = ta;

    QActionGroup *alignmentGroup = new QActionGroup(this);
    alignmentGroup->addAction(richTextLeft);
    alignmentGroup->addAction(richTextCenter);
    alignmentGroup->addAction(richTextRight);
    alignmentGroup->addAction(richTextJustified);

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_undo"), ta);
    ta->setText(i18n("Undo"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    richTextUndo = ta;

    ta = new KToggleAction(ac);
    ac->addAction(QStringLiteral("richtext_redo"), ta);
    ta->setText(i18n("Redo"));
    ta->setIcon(QIcon::fromTheme(QStringLiteral("edit-redo")));
    richTextRedo = ta;

    disableRichTextToolBar();
}

KToolBar *InlineEditors::richTextToolBar()
{
    if (Global::activeMainWindow()) {
        Global::activeMainWindow()->toolBar(); // Make sure we create the main toolbar FIRST, so it will be on top of the edit toolbar!
        return Global::activeMainWindow()->toolBar(QStringLiteral("richTextEditToolBar"));
    } else
        return nullptr;
}

void InlineEditors::enableRichTextToolBar()
{
    richTextFont->setEnabled(true);
    richTextFontSize->setEnabled(true);
    richTextColor->setEnabled(true);
    richTextBold->setEnabled(true);
    richTextItalic->setEnabled(true);
    richTextUnderline->setEnabled(true);
    richTextLeft->setEnabled(true);
    richTextCenter->setEnabled(true);
    richTextRight->setEnabled(true);
    richTextJustified->setEnabled(true);
    richTextUndo->setEnabled(true);
    richTextRedo->setEnabled(true);
}

void InlineEditors::disableRichTextToolBar()
{
    disconnect(richTextFont);
    disconnect(richTextFontSize);
    disconnect(richTextColor);
    disconnect(richTextBold);
    disconnect(richTextItalic);
    disconnect(richTextUnderline);
    disconnect(richTextLeft);
    disconnect(richTextCenter);
    disconnect(richTextRight);
    disconnect(richTextJustified);
    disconnect(richTextUndo);
    disconnect(richTextRedo);

    richTextFont->setEnabled(false);
    richTextFontSize->setEnabled(false);
    richTextColor->setEnabled(false);
    richTextBold->setEnabled(false);
    richTextItalic->setEnabled(false);
    richTextUnderline->setEnabled(false);
    richTextLeft->setEnabled(false);
    richTextCenter->setEnabled(false);
    richTextRight->setEnabled(false);
    richTextJustified->setEnabled(false);
    richTextUndo->setEnabled(false);
    richTextRedo->setEnabled(false);

    // Return to a "proper" state:
    QFont defaultFont;
    QColor textColor = (Global::bnpView && Global::bnpView->currentBasket() ? Global::bnpView->currentBasket()->textColor() : palette().color(QPalette::Text));
    richTextFont->setCurrentFont(defaultFont.family());
    richTextFontSize->setFontSize(defaultFont.pointSize());
    richTextColor->setColor(textColor);
    richTextBold->setChecked(false);
    richTextItalic->setChecked(false);
    richTextUnderline->setChecked(false);
    richTextLeft->setChecked(false);
    richTextCenter->setChecked(false);
    richTextRight->setChecked(false);
    richTextJustified->setChecked(false);
}

QPalette InlineEditors::palette() const
{
    return qApp->palette();
}

#include "moc_noteedit.cpp"
