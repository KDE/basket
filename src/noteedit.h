/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NOTEEDIT_H
#define NOTEEDIT_H

#include <QClipboard>
#include <QDialog>
#include <QLineEdit>

#include "notecontent.h"

class QGraphicsProxyWidget;
class QWidget;
class QPushButton;
class QFontComboBox;
class QTextCharFormat;
class QAction;

class KIconButton;
class KUrlRequester;
class KTextEdit;
class KToggleAction;
class KToolBar;
class KActionCollection;
class KComboBox;
class KColorCombo;

class FontSizeCombo;
class Note;
class ServiceLaunchRequester;
class FocusWidgetFilter;
class BasketListViewItem;

/** The base class for every note editor.
 * Scenario:
 *  The Basket class calls NoteEditor::editNoteContent() with the NoteContent to edit.
 *  This method create the good child NoteEditor depending
 *  on the note content type and return it to the Basket.
 *  This custom NoteEditor have two choices regarding what to do in its constructor:
 *   - Display a dialog and then call cancel() if the user canceled the dialog;
 *   - Create an inline editor and call setInlineEditor() with that editor as parameter.
 *     When the user exit the edition, validate() is called by the Basket.
 *     You should then call setEmpty() is the user cleared the content.
 *  The custom editor SHOULD call the NoteEditor constructor.
 *  If the user cleared the content OR if the user canceled the dialog whereas he/she
 *  JUST ADDED the note, then the note will be deleted by the Basket.
 */
class NoteEditor : public QObject
{
    Q_OBJECT
public:
    explicit NoteEditor(NoteContent *noteContent);
    ~NoteEditor() override;

    bool isEmpty()
    {
        return m_isEmpty;
    }
    bool canceled()
    {
        return m_canceled;
    }
    bool isInline()
    {
        return m_widget != nullptr;
    }
    QGraphicsProxyWidget *graphicsWidget()
    {
        return m_widget;
    }
    KTextEdit *textEdit()
    {
        return m_textEdit;
    }
    QLineEdit *lineEdit()
    {
        return m_lineEdit;
    }

    void setCursorTo(const QPointF &pos);
    void connectActions(BasketScene *scene);
    void startSelection(const QPointF &pos);
    void updateSelection(const QPointF &pos);
    void endSelection(const QPointF &pos);
    void paste(const QPointF &pos, QClipboard::Mode mode);

private:
    bool m_isEmpty;
    bool m_canceled;
    QGraphicsProxyWidget *m_widget;
    KTextEdit *m_textEdit;
    QLineEdit *m_lineEdit;
    NoteContent *m_noteContent;

public:
    NoteContent *noteContent()
    {
        return m_noteContent;
    }
    Note *note();

protected:
    void setEmpty()
    {
        m_isEmpty = true;
    }
    void cancel()
    {
        m_canceled = true;
    }
    void setInlineEditor(QWidget *inlineEditor);

public:
    virtual void validate()
    {
    }
    virtual void autoSave(bool /*toFileToo*/)
    {
    } // Same as validate(), but does not precede editor close and is triggered either while the editor widget changed size or after 3 seconds of inactivity.

Q_SIGNALS:
    void askValidation();
    void mouseEnteredEditorWidget();

public:
    static NoteEditor *editNoteContent(NoteContent *noteContent, QWidget *parent);
};

class TextEditor : public NoteEditor
{
    Q_OBJECT
public:
    TextEditor(TextContent *textContent, QWidget *parent);
    ~TextEditor() override;
    void validate() override;
    void autoSave(bool toFileToo) override;

protected:
    TextContent *m_textContent;
};

class HtmlEditor : public NoteEditor
{
    Q_OBJECT
public:
    HtmlEditor(HtmlContent *htmlContent, QWidget *parent);
    ~HtmlEditor() override;
    void validate() override;
    void autoSave(bool toFileToo) override;

protected:
    HtmlContent *m_htmlContent;
public Q_SLOTS:
    void cursorPositionChanged();
    void editTextChanged();
    void charFormatChanged(const QTextCharFormat &format);
protected Q_SLOTS:
    void setBold(bool isChecked);
    void setLeft();
    void setCentered();
    void setRight();
    void setBlock();
    void onFontSelectionChanged(const QFont &font); //!< When a font is selected from combo box
    void detectTags();
};

class ImageEditor : public NoteEditor
{
    Q_OBJECT
public:
    ImageEditor(ImageContent *imageContent, QWidget *parent);
};

class AnimationEditor : public NoteEditor
{
    Q_OBJECT
public:
    AnimationEditor(AnimationContent *animationContent, QWidget *parent);
};

class FileEditor : public NoteEditor
{
    Q_OBJECT
public:
    FileEditor(FileContent *fileContent, QWidget *parent);
    ~FileEditor() override;
    void validate() override;
    void autoSave(bool toFileToo) override;

protected:
    FileContent *m_fileContent;
};

class LinkEditor : public NoteEditor
{
    Q_OBJECT
public:
    LinkEditor(LinkContent *linkContent, QWidget *parent);
};

class CrossReferenceEditor : public NoteEditor
{
    Q_OBJECT
public:
    CrossReferenceEditor(CrossReferenceContent *crossReferenceContent, QWidget *parent);
};

class LauncherEditor : public NoteEditor
{
    Q_OBJECT
public:
    LauncherEditor(LauncherContent *launcherContent, QWidget *parent);
};

class ColorEditor : public NoteEditor
{
    Q_OBJECT
public:
    ColorEditor(ColorContent *colorContent, QWidget *parent);
};

class UnknownEditor : public NoteEditor
{
    Q_OBJECT
public:
    UnknownEditor(UnknownContent *unknownContent, QWidget *parent);
};

/** The dialog to edit Link Note content.
 * @author Sébastien Laoût
 */
class LinkEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LinkEditDialog(LinkContent *contentNote, QWidget *parent = nullptr);
    ~LinkEditDialog() override;
    void ensurePolished();

protected Q_SLOTS:
    void slotOk();
    void urlChanged(const QString &);
    void doNotAutoTitle(const QString &);
    void doNotAutoIcon(QString);
    void guessTitle();
    void guessIcon();

private:
    LinkContent *m_noteContent;
    bool m_isAutoModified;
    KUrlRequester *m_url;
    QLineEdit *m_title;
    KIconButton *m_icon;
    QPushButton *m_autoTitle;
    QPushButton *m_autoIcon;
};

/** The dialog to edit cross reference content.
 * @author Brian C. Milco
 */
class CrossReferenceEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CrossReferenceEditDialog(CrossReferenceContent *contentNote, QWidget *parent = nullptr);
    ~CrossReferenceEditDialog() override;

protected Q_SLOTS:
    void slotOk();
    void urlChanged(const int index);

protected:
    void generateBasketList(KComboBox *targetList, BasketListViewItem *item = nullptr, int indent = 0);

private:
    CrossReferenceContent *m_noteContent;
    KComboBox *m_targetBasket;
};

/** The dialog to edit Launcher Note content.
 * @author Sébastien Laoût
 */
class LauncherEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LauncherEditDialog(LauncherContent *contentNote, QWidget *parent = nullptr);
    ~LauncherEditDialog() override;
    void ensurePolished();
protected Q_SLOTS:
    void slotOk();
    void guessIcon();

private:
    LauncherContent *m_noteContent;
    ServiceLaunchRequester *m_command;
    QLineEdit *m_name;
    KIconButton *m_icon;
};

/** This class manage toolbars for the inline editors.
 * The toolbars should be created once at the application startup,
 * then KXMLGUI can manage them and save their state and position...
 * @author Sébastien Laoût
 */
class InlineEditors : public QObject
{
    Q_OBJECT
public:
    InlineEditors();
    ~InlineEditors() override;
    void initToolBars(KActionCollection *ac);
    static InlineEditors *instance();

public:
    // Rich Text ToolBar:
    KToolBar *richTextToolBar();
    void enableRichTextToolBar();
    void disableRichTextToolBar();
    QPalette palette() const;
    QFontComboBox *richTextFont;
    FontSizeCombo *richTextFontSize;
    KColorCombo *richTextColor;
    KToggleAction *richTextBold;
    KToggleAction *richTextItalic;
    KToggleAction *richTextUnderline;
    //  KToggleAction     *richTextSuper;
    //  KToggleAction     *richTextSub;
    KToggleAction *richTextLeft;
    KToggleAction *richTextCenter;
    KToggleAction *richTextRight;
    KToggleAction *richTextJustified;
    QAction *richTextUndo;
    QAction *richTextRedo;
    FocusWidgetFilter *focusWidgetFilter;
};

#endif // NOTEEDIT_H
