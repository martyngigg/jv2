// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "mysortfilterproxymodel.h"
#include "jsontablemodel.h"
#include <QModelIndex>
#include <QObject>
#include <QSortFilterProxyModel>

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    filterString_ = "";
    caseSensitive_ = false;
}

void MySortFilterProxyModel::setFilterString(QString filterString) { filterString_ = filterString; }

QString MySortFilterProxyModel::filterString() const { return filterString_; }

void MySortFilterProxyModel::toggleCaseSensitivity(bool caseSensitive)
{
    caseSensitive_ = caseSensitive;
    emit updateFilter();
}

bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto filterString = filterString_;
    if (!caseSensitive_)
        filterString = filterString.toLower();
    auto accept = false;
    for (auto i = 0; i < sourceModel()->columnCount(); i++)
    {
        auto index = sourceModel()->index(sourceRow, i, sourceParent);
        auto tableData = sourceModel()->data(index).toString();
        if (!caseSensitive_)
            tableData = tableData.toLower();

        if (tableData.contains(filterString))
            accept = true;
    }

    return (accept);
}