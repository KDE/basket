/**
 * SPDX-FileCopyrightText: (C) 2003 by Sébastien Laoût <slaout@linux62.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FILTER_H
#define FILTER_H

#include <QWidget>
#include <QtCore/QMap>

class QToolButton;

class QLineEdit;
class KComboBox;

class Tag;
class State;

/** The structure that contain all filter terms
 * @author Sébastien Laoût
 */
struct FilterData {
public:
    // Useful Enum for tagFilterType:
    enum TagFilterType { DontCareTagsFilter = 0, NotTaggedFilter, TaggedFilter, TagFilter, StateFilter };
    // Constructor and Destructor:
    FilterData()
    {
        isFiltering = false;
        tagFilterType = DontCareTagsFilter;
        tag = 0;
        state = 0;
    }
    ~FilterData()
    {
    }
    // Filter data:
    QString string;
    int tagFilterType;
    Tag *tag;
    State *state;
    bool isFiltering;
};

/** A QWidget that allow user to enter terms to filter in a Basket.
 * @author Sébastien Laoût
 */
class FilterBar : public QWidget
{
    Q_OBJECT
public:
    explicit FilterBar(QWidget *parent = nullptr);
    ~FilterBar() override;
    const FilterData &filterData();
signals:
    void newFilter(const FilterData &data);
public slots:
    void repopulateTagsCombo();
    void reset();
    void inAllBaskets();
    void setEditFocus();
    void filterTag(Tag *tag);
    void filterState(State *state);
    void setFilterData(const FilterData &data);

public:
    bool hasEditFocus();
    QLineEdit *lineEdit()
    {
        return m_lineEdit;
    }
private slots:
    void changeFilter();
    void tagChanged(int index);

private:
    FilterData m_data;
    QLineEdit *m_lineEdit;
    QToolButton *m_resetButton;
    KComboBox *m_tagsBox;
    QToolButton *m_inAllBasketsButton;

    QMap<int, Tag *> m_tagsMap;
    QMap<int, State *> m_statesMap;
};

#endif // FILTER_H
