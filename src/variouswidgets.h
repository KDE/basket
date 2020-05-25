/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VARIOUSWIDGETS_H
#define VARIOUSWIDGETS_H

#include <QDialog>
#include <QWidget>

#include <KComboBox>
#include <KUrlLabel>

class QLineEdit;
class QListWidgetItem;
class QResizeEvent;
class QString;
class QKeyEvent;

/** A widget to select a command to run,
 * with a QLineEdit and a QPushButton.
 * @author Sébastien Laoût
 */
class RunCommandRequester : public QWidget
{
    Q_OBJECT
public:
    RunCommandRequester(const QString &runCommand, const QString &message, QWidget *parent = nullptr);
    ~RunCommandRequester() override;
    QString runCommand();
    void setRunCommand(const QString &runCommand);
    QLineEdit *lineEdit()
    {
        return m_runCommand;
    }
private Q_SLOTS:
    void slotSelCommand();

private:
    QLineEdit *m_runCommand;
    QString m_message;
};

/** KComboBox to ask icon size
 * @author Sébastien Laoût
 */
class IconSizeCombo : public KComboBox
{
    Q_OBJECT
public:
    explicit IconSizeCombo(QWidget *parent = nullptr);
    ~IconSizeCombo() override;
    int iconSize();
    void setSize(int size);
};

/** A window that the user resize to graphically choose a new image size
 * TODO: Create a SizePushButton or even SizeWidget
 * @author Sébastien Laoût
 */
class ViewSizeDialog : public QDialog
{
    Q_OBJECT
public:
    ViewSizeDialog(QWidget *parent, int w, int h);
    ~ViewSizeDialog() override;

private:
    void resizeEvent(QResizeEvent *) override;
    QWidget *m_sizeGrip;
};

/** A label displaying a link that, once clicked, offer a What's This messageBox to help users.
 * @author Sébastien Laoût
 */
class HelpLabel : public KUrlLabel
{
    Q_OBJECT
public:
    HelpLabel(const QString &text, const QString &message, QWidget *parent);
    ~HelpLabel() override;
    QString message()
    {
        return m_message;
    }
public Q_SLOTS:
    void setMessage(const QString &message)
    {
        m_message = message;
    }
    void display();

private:
    QString m_message;
};

/** A dialog to choose the size of an icon.
 * @author Sébastien Laoût
 */
class IconSizeDialog : public QDialog
{
    Q_OBJECT
public:
    IconSizeDialog(const QString &caption, const QString &message, const QString &icon, int iconSize, QWidget *parent);
    ~IconSizeDialog() override;
    int iconSize()
    {
        return m_iconSize;
    } /// << @return the chosen icon size (16, 32, ...) or -1 if canceled!
protected Q_SLOTS:
    void slotCancel();
    void slotSelectionChanged();
    void choose(QListWidgetItem *);

private:
    QListWidgetItem *m_size16;
    QListWidgetItem *m_size22;
    QListWidgetItem *m_size32;
    QListWidgetItem *m_size48;
    QListWidgetItem *m_size64;
    QListWidgetItem *m_size128;
    int m_iconSize;
    QPushButton *okButton;
};

/**
 * A missing class from Frameworks (and Qt): a combobox to select a font size!
 */
class FontSizeCombo : public KComboBox
{
    Q_OBJECT
public:
    FontSizeCombo(bool rw, bool withDefault, QWidget *parent = nullptr);
    ~FontSizeCombo() override;
    void setFontSize(qreal size);
    qreal fontSize();

protected:
    void keyPressEvent(QKeyEvent *event) override;
Q_SIGNALS:
    void sizeChanged(qreal size);
    void escapePressed();
    void returnPressed2();
private Q_SLOTS:
    void textChangedInCombo(const QString &text);

private:
    bool m_withDefault;
};

#endif // VARIOUSWIDGETS_H
