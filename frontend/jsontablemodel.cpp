// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "jsontablemodel.h"
#include <QJsonObject>

// Model to handle json data in table view
JsonTableModel::JsonTableModel(const JsonTableModel::Header &header_, QObject *parent)
    : QAbstractTableModel(parent), m_header(header_)
{
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
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (orientation)
    {
        case Qt::Horizontal:
            return m_header[section]["title"];
        case Qt::Vertical:
            // return section + 1;
            return QVariant();
        default:
            return QVariant();
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
    switch (role)
    {
        case Qt::DisplayRole:
        {
            QJsonObject obj = getJsonObject(index);
            const QString &key = m_header[index.column()]["index"];
            if (obj.contains(key))
            {
                QJsonValue v = obj[key];

                if (v.isString())
                    return v.toString();
                else if (v.isDouble())
                    return QString::number(v.toDouble());
                else
                    return QVariant();
            }
            else
                return QVariant();
        }
        case Qt::ToolTipRole:
            return QVariant();
        default:
            return QVariant();
    }
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
                auto totalRunTime = std::get<1>(data).toInt() + valueObj["duration"].toString().toInt();
                std::get<1>(data) = QString::number(totalRunTime);
                std::get<2>(data) += "-" + valueObj["run_number"].toString();
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
    Header header_;
    foreach (const QString &key, groupedJson.at(0).toObject().keys())
    {
        header_.push_back(Heading({{"title", key}, {"index", key}}));
    }
    setHeader(header_);
    setJson(groupedJson);
}

// Apply held (ungrouped) values to table
void JsonTableModel::unGroupData()
{
    setHeader(m_holdHeader);
    setJson(m_holdJson);
}