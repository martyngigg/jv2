// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "jsontablemodel.h"
#include <QDebug>
#include <QJsonObject>
#include <QTime>

// Model to handle json data in table view
JsonTableModel::JsonTableModel(const JsonTableModel::Header &header_, QObject *parent)
    : QAbstractTableModel(parent), m_header(header_)
{
    m_groupedHeader.push_back(Heading({{"title", "Title"}, {"index", "title"}}));
    m_groupedHeader.push_back(Heading({{"title", "Total Duration"}, {"index", "duration"}}));
    m_groupedHeader.push_back(Heading({{"title", "Run Numbers"}, {"index", "run_number"}}));
}

// Sets json data to populate table
bool JsonTableModel::setJson(const QJsonArray &array)
{
    beginResetModel();
    m_json = array;
    endResetModel();
    return true;
}

QJsonArray JsonTableModel::getJson() { return m_json; }

// Sets header_ data to define table
bool JsonTableModel::setHeader(const Header &array)
{
    beginResetModel();
    m_header = array;
    endResetModel();
    return true;
}

JsonTableModel::Header JsonTableModel::getHeader() { return m_header; }

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::UserRole)
        return m_header[section]["index"]; // Index == database name

    if (role != Qt::DisplayRole)
        return {};

    switch (orientation)
    {
        case Qt::Horizontal:
            return m_header[section]["title"]; // Title == desired display name
        case Qt::Vertical:
            // return section + 1;
            return {};
        default:
            return {};
    }
}

int JsonTableModel::rowCount(const QModelIndex &parent) const { return m_json.size(); }

int JsonTableModel::columnCount(const QModelIndex &parent) const { return m_header.size(); }

QJsonObject JsonTableModel::getJsonObject(const QModelIndex &index) const
{
    const QJsonValue &value = m_json[index.row()];
    return value.toObject();
}

// Fills table view
QVariant JsonTableModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    QJsonObject obj = getJsonObject(index);
    const QString &key = m_header[index.column()]["index"];
    if (!obj.contains(key))
        return {};
    QJsonValue v = obj[key];

    if (v.isDouble())
        return QString::number(v.toDouble());
    if (!v.isString())
        return {};

    // if title = Run Numbers then format (for grouped data)
    if (m_header[index.column()]["title"] == "Run Numbers")
    {
        // Format grouped runs display
        auto runArr = v.toString().split(";");
        if (runArr.size() == 1)
            return runArr[0];
        QString displayString = runArr[0];
        for (auto i = 1; i < runArr.size(); i++)
            if (runArr[i].toInt() == runArr[i - 1].toInt() + 1)
                displayString += "-" + runArr[i];
            else
                displayString += "," + runArr[i];
        QStringList splitDisplay;
        foreach (const auto &string, displayString.split(","))
        {
            if (string.contains("-"))
                splitDisplay.append(string.left(string.indexOf("-") + 1) +
                                    string.right(string.size() - string.lastIndexOf("-") - 1));
            else
                splitDisplay.append(string);
        }
        return splitDisplay.join(",");
    }
    return v.toString();
}

// Returns grouped table data
void JsonTableModel::groupData()
{
    QJsonArray groupedJson;

    // holds data in tuple as QJson referencing is incomplete
    std::vector<std::tuple<QString, QString, QString>> groupedData;
    for (QJsonValue value : m_json)
    {
        const QJsonObject &valueObj = value.toObject();
        bool unique = true;

        // add duplicate title data to stack
        for (std::tuple<QString, QString, QString> &data : groupedData)
        {
            if (std::get<0>(data) == valueObj["title"].toString())
            {
                auto currentTotal = QTime::fromString(std::get<1>(data), "HH:mm:ss");
                // convert duration to seconds
                auto newTime = QTime(0, 0, 0).secsTo(QTime::fromString(valueObj["duration"].toString(), "HH:mm:ss"));
                auto totalRunTime = currentTotal.addSecs(newTime).toString("HH:mm:ss");
                std::get<1>(data) = QString(totalRunTime);
                std::get<2>(data) += ";" + valueObj["run_number"].toString();
                unique = false;
                break;
            }
        }
        if (unique)
            groupedData.push_back(std::make_tuple(valueObj["title"].toString(), valueObj["duration"].toString(),
                                                  valueObj["run_number"].toString()));
    }
    for (std::tuple<QString, QString, QString> data : groupedData)
    {
        auto groupData = QJsonObject({qMakePair(QString("title"), QJsonValue(std::get<0>(data))),
                                      qMakePair(QString("duration"), QJsonValue(std::get<1>(data))),
                                      qMakePair(QString("run_number"), QJsonValue(std::get<2>(data)))});
        groupedJson.push_back(QJsonValue(groupData));
    }
    // Hold ungrouped values
    m_holdJson = m_json;
    m_holdHeader = m_header;

    // Get and assign array headers
    setHeader(m_groupedHeader);
    setJson(groupedJson);
}

// Apply held (ungrouped) values to table
void JsonTableModel::unGroupData()
{
    setHeader(m_holdHeader);
    setJson(m_holdJson);
}

void JsonTableModel::setColumnTitle(int section, QString title) { m_header[section]["index"] = title; }

bool JsonTableModel::setData(const QModelIndex &index, QJsonObject rowData, int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        const int row = index.row();
        m_json[row] = rowData;
        emit dataChanged(index, index.siblingAtColumn(rowData.count()), {Qt::DisplayRole, Qt::EditRole});
        return true;
    }

    return false;
}

bool JsonTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
    emit layoutAboutToBeChanged();
    beginInsertRows(parent, row, row + count);
    for (int position = 0; position < count; ++position)
        m_json.insert(row, QJsonValue());
    endInsertRows();
    emit layoutChanged();
    return true;
}