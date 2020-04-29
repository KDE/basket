/**
 * SPDX-FileCopyrightText: (C) 2005 Sébastien Laoût <slaout@linux62.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TAG_H
#define TAG_H

#include <QtCore/QList>

#include <KToggleAction>

class QColor;
class QFont;
class QPainter;
class QString;

class QKeySequence;

class Tag;

/**
 * @author Sébastien Laoût
 */
class State
{
public:
    /// LIST OF STATES:
    typedef QList<State *> List;

public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    explicit State(const QString &id = QString(), Tag *tag = nullptr);
    ~State();
    /// SET PROPERTIES:
    void setId(const QString &id)
    {
        m_id = id;
    }
    void setName(const QString &name)
    {
        m_name = name;
    }
    void setEmblem(const QString &emblem)
    {
        m_emblem = emblem;
    }
    void setBold(bool bold)
    {
        m_bold = bold;
    }
    void setItalic(bool italic)
    {
        m_italic = italic;
    }
    void setUnderline(bool underline)
    {
        m_underline = underline;
    }
    void setStrikeOut(bool strikeOut)
    {
        m_strikeOut = strikeOut;
    }
    void setTextColor(const QColor &color)
    {
        m_textColor = color;
    }
    void setFontName(const QString &font)
    {
        m_fontName = font;
    }
    void setFontSize(int size)
    {
        m_fontSize = size;
    }
    void setBackgroundColor(const QColor &color)
    {
        m_backgroundColor = color;
    }
    void setTextEquivalent(const QString &text)
    {
        m_textEquivalent = text;
    }
    void setOnAllTextLines(bool yes)
    {
        m_onAllTextLines = yes;
    }
    void setAllowCrossReferences(bool yes)
    {
        m_allowCrossReferences = yes;
    }
    void setParentTag(Tag *tag)
    {
        m_parentTag = tag;
    }
    /// GET PROPERTIES:
    QString id() const
    {
        return m_id;
    }
    QString name() const
    {
        return m_name;
    }
    QString emblem() const
    {
        return m_emblem;
    }
    bool bold() const
    {
        return m_bold;
    }
    bool italic() const
    {
        return m_italic;
    }
    bool underline() const
    {
        return m_underline;
    }
    bool strikeOut() const
    {
        return m_strikeOut;
    }
    QColor textColor() const
    {
        return m_textColor;
    }
    QString fontName() const
    {
        return m_fontName;
    }
    int fontSize() const
    {
        return m_fontSize;
    }
    QColor backgroundColor() const
    {
        return m_backgroundColor;
    }
    QString textEquivalent() const
    {
        return m_textEquivalent;
    }
    bool onAllTextLines() const
    {
        return m_onAllTextLines;
    }
    bool allowCrossReferences() const
    {
        return m_allowCrossReferences;
    }
    Tag *parentTag() const
    {
        return m_parentTag;
    }
    /// HELPING FUNCTIONS:
    State *nextState(bool cycle = true);
    QString fullName();
    QFont font(QFont base);
    QString toCSS(const QString &gradientFolderPath, const QString &gradientFolderName, const QFont &baseFont);
    static void merge(const List &states, State *result, int *emblemsCount, bool *haveInvisibleTags, const QColor &backgroundColor);
    void copyTo(State *other);

private:
    /// PROPERTIES:
    QString m_id;
    QString m_name;
    QString m_emblem;
    bool m_bold;
    bool m_italic;
    bool m_underline;
    bool m_strikeOut;
    QColor m_textColor;
    QString m_fontName;
    int m_fontSize;
    QColor m_backgroundColor;
    QString m_textEquivalent;
    bool m_onAllTextLines;
    bool m_allowCrossReferences;
    Tag *m_parentTag;
};

/** A Tag is a category of Notes.
 * A Note can have 0, 1 or more Tags.
 * A Tag can have a unique State or several States.
 * @author Sébastien Laoût
 */
class Tag
{
public:
    /// LIST OF ALL TAGS IN THE APPLICATION:
    typedef QList<Tag *> List;
    static Tag::List all;
    static State *stateForId(const QString &id);
    static Tag *tagForKAction(QAction *action);
    static Tag *tagSimilarTo(Tag *tagToTest);
    static QMap<QString, QString> loadTags(const QString &path = QString() /*, bool merge = false*/); /// << Load the tags contained in the XML file @p path or those in the application settings if @p path isEmpty(). If @p merge is true and
                                                                                                      /// a tag with the id of a tag that should be loaded already exist, the tag will get a new id. Otherwise, the tag will be dismissed.
    static void saveTags();
    static void saveTagsTo(QList<Tag *> &list, const QString &fullPath);
    static void createDefaultTagsSet(const QString &file);
    static long getNextStateUid();

private:
    static long nextStateUid;

public:
    /// CONSTRUCTOR AND DESTRUCTOR:
    Tag(/*State *firstState, const QString &name, bool inheritedBySiblings*/);
    ~Tag();
    /// SET PROPERTIES:
    void setName(const QString &name);
    void setShortcut(const QKeySequence &shortcut)
    {
        m_action->setShortcut(shortcut);
    }
    void setInheritedBySiblings(bool inherited)
    {
        m_inheritedBySiblings = inherited;
    }
    void appendState(State *state)
    {
        m_states.append(state);
        state->setParentTag(this);
    }
    void removeState(State *state)
    {
        m_states.removeOne(state);
        state->setParentTag(nullptr);
    }
    /// GET PROPERTIES:
    QString name() const
    {
        return m_name;
    }
    QKeySequence shortcut() const
    {
        return m_action->shortcut();
    }
    bool inheritedBySiblings() const
    {
        return m_inheritedBySiblings;
    }
    State::List &states() const
    {
        return (State::List &)m_states;
    }
    int countStates() const
    {
        return m_states.count();
    }
    void copyTo(Tag *other);

private:
    /// PROPERTIES:
    QString m_name;
    QAction *m_action;
    bool m_inheritedBySiblings;
    State::List m_states;
};

#include <qicon.h>
#include <qstring.h>

/** An action that represents a State or a Tag
 * @author Kelvie Wong
 * Based off of StateMenuItem by Sébastien Laoût
 */
class StateAction : public KToggleAction
{
    Q_OBJECT
    Q_DISABLE_COPY(StateAction);

public:
    StateAction(State *state, const QKeySequence &shortcut, QWidget *parent, bool withTagName = false);

    ~StateAction() override;

private:
    State *m_state;
    QString m_name;
    QString m_shortcut;
};

#endif // TAG_H
