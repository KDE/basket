/**
 * SPDX-FileCopyrightText: (C) 2003 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LINKLABEL_H
#define LINKLABEL_H

#include <QFrame>

class QPixmap;
class QString;
class QUrl;
class QColor;
class QLabel;
class QBoxLayout;
class QSpacerItem;
class QCheckBox;
class QEvent;

class KComboBox;
class KColorCombo2;
class IconSizeCombo;

class HTMLExporter;
class HelpLabel;
class KCModule;

/** Store the style of links
 * @author Sébastien Laoût
 */
class LinkLook
{
public:
    enum Underlining {
        Always = 0,
        Never,
        OnMouseHover,
        OnMouseOutside
    };
    enum Preview {
        None = 0,
        IconSize,
        TwiceIconSize,
        ThreeIconSize
    };
    explicit LinkLook(bool useLinkColor = true, bool canPreview = true);
    LinkLook(const LinkLook &other);
    void setLook(bool italic, bool bold, int underlining, QColor color, QColor hoverColor, int iconSize, int preview /*= None*/);
    inline bool italic() const
    {
        return m_italic;
    }
    inline bool bold() const
    {
        return m_bold;
    }
    inline int underlining() const
    {
        return m_underlining;
    }
    inline QColor color() const
    {
        return m_color;
    }
    inline QColor hoverColor() const
    {
        return m_hoverColor;
    }
    inline int iconSize() const
    {
        return m_iconSize;
    }
    inline int preview() const
    {
        return m_preview;
    }
    inline bool useLinkColor() const
    {
        return m_useLinkColor;
    }
    inline bool canPreview() const
    {
        return m_canPreview;
    }
    /* Helping Functions */
    bool underlineOutside() const
    {
        return underlining() == Always || underlining() == OnMouseOutside;
    }
    bool underlineInside() const
    {
        return underlining() == Always || underlining() == OnMouseHover;
    }
    bool previewEnabled() const
    {
        return canPreview() && preview() > None;
    }
    int previewSize() const;
    QColor effectiveColor() const;
    QColor effectiveHoverColor() const;
    QColor defaultColor() const;
    QColor defaultHoverColor() const;
    QString toCSS(const QString &cssClass, const QColor &defaultTextColor) const;

private:
    bool m_italic;
    bool m_bold;
    int m_underlining;
    QColor m_color;
    QColor m_hoverColor;
    int m_iconSize;
    int m_preview;
    bool m_useLinkColor;
    bool m_canPreview;

public:
    /* Global Looks */
    static LinkLook *soundLook;
    static LinkLook *fileLook;
    static LinkLook *localLinkLook;
    static LinkLook *networkLinkLook;
    static LinkLook *launcherLook;
    static LinkLook *crossReferenceLook;
    /* Static method to get a LinkLook from an URL */
    static LinkLook *lookForURL(const QUrl &url);
};

/** Used to represent links with icon and specific look
 * Note : This label will appear blank while LinkLook willn't be set
 * @author Sébastien Laoût
 */
class LinkLabel : public QFrame
{
    Q_OBJECT
public:
    LinkLabel(int hAlign, int vAlign, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    LinkLabel(const QString &title,
              const QString &icon,
              LinkLook *look,
              int hAlign,
              int vAlign,
              QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());
    ~LinkLabel() override;

public:
    void setLink(const QString &title, const QString &icon, LinkLook *look = nullptr);
    void setLook(LinkLook *look);
    void setAlign(int hAlign, int vAlign);
    void setSelected(bool selected);
    void setPaletteBackgroundColor(const QColor &color);
    int heightForWidth(int w = -1) const override;

protected:
    void initLabel(int hAlign, int vAlign);
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    QBoxLayout *m_layout;
    QLabel *m_icon;
    QLabel *m_title;
    QSpacerItem *m_spacer1;
    QSpacerItem *m_spacer2;

    bool m_isSelected;
    bool m_isHovered;

    LinkLook *m_look;
    int m_hAlign;
    int m_vAlign;
};

/** THE NEW CLASS TO DISPLAY Links FOR THE NEW BASKET ENGINE.
 * We should get ride of class LinkLabel soon.
 * And LinkLabel will be entirely rewritten to use this LinkDisplay as the drawing primitives.
 * @author Sébastien Laoût
 */
class LinkDisplay
{
public:
    LinkDisplay(); /// << Create a new empty unselected LinkDisplay. Please then call setLink() to make sense.
    // Configure the link displayer:
    void setLink(const QString &title,
                 const QString &icon,
                 LinkLook *look,
                 const QFont &font); /// << Change the content and disposition. minWidth(), width() & height() can have changed. Keep the old preview (if any)
    void setLink(const QString &title,
                 const QString &icon,
                 const QPixmap &preview,
                 LinkLook *look,
                 const QFont &font); /// << Idem but change the preview too (or remove it if it is invalid)
    void setWidth(qreal width); /// << Set a new width. @see height() that will be computed.
    // Get its properties:
    qreal minWidth() const
    {
        return m_minWidth;
    } /// << @return the minimum width to display this link.
    qreal maxWidth() const
    {
        return m_maxWidth;
    } /// << @return the maximum width to display this link.
    qreal width() const
    {
        return m_width;
    } /// << @return the width of the link. It is never less than minWidth()!
    qreal height() const
    {
        return m_height;
    } /// << @return the height if the link after having set it a width.
    // And finally, use it:
    void paint(QPainter *painter,
               qreal x,
               qreal y,
               qreal width,
               qreal height,
               const QPalette &palette,
               bool isDefaultColor,
               bool isSelected,
               bool isHovered,
               bool isIconButtonHovered)
        const; /// << Draw the link on a painter. Set textColor to be !isValid() to use the LinkLook color. Otherwise it will use this color!
    QPixmap feedbackPixmap(qreal width,
                           qreal height,
                           const QPalette &palette,
                           bool isDefaultColor); /// << @return the pixmap to put under the cursor while dragging this object.
    // Eventually get some information about the link display:
    bool iconButtonAt(const QPointF &pos) const; /// << @return true if the icon button is under point @p pos.
    QRectF iconButtonRect() const; /// << @return the rectangle of the icon button.
    // Utility function:
    QFont labelFont(QFont font, bool isIconButtonHovered) const; /// << @return the font for this link, according to parent font AND LinkLook!
    qreal heightForWidth(qreal width) const; /// << @return the needed height to display the link in function of a width.
    QString toHtml(const QString &imageName) const; /// << Convert the link to HTML code, using the LinkLook to style it.
    QString toHtml(HTMLExporter *exporter, const QUrl &url, const QString &title = QString());

private:
    QString m_title;
    QString m_icon;
    QPixmap m_preview;
    LinkLook *m_look;
    QFont m_font;
    qreal m_minWidth;
    qreal m_maxWidth;
    qreal m_width;
    qreal m_height;
};

/** A widget to edit a LinkLook, showing a live example to the user.
 * @author Sébastien Laoût
 */
class LinkLookEditWidget : public QWidget
{
    Q_OBJECT
public:
    LinkLookEditWidget(KCModule *module, const QString exTitle, const QString exIcon, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~LinkLookEditWidget() override;
    void saveChanges();
    void saveToLook(LinkLook *look);
    void set(LinkLook *look);
private Q_SLOTS:
    void slotChangeLook();

protected:
    LinkLook *m_look;
    QCheckBox *m_italic;
    QCheckBox *m_bold;
    KComboBox *m_underlining;
    KColorCombo2 *m_color;
    KColorCombo2 *m_hoverColor;
    IconSizeCombo *m_iconSize;
    KComboBox *m_preview;
    LinkLook *m_exLook;
    LinkLabel *m_example;
    QString m_exTitle;
    QString m_exIcon;
    HelpLabel *m_hLabel;
    QLabel *m_label;
};

#endif // LINKLABEL_H
