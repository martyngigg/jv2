// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef JSONTABLEMODEL_H
#define JSONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QVector>

// Model for json usage in table view
class JsonTableModel : public QAbstractTableModel
{
    public:
    // Assigning custom data types for table headings
    typedef QMap<QString, QString> Heading;
    typedef QVector<Heading> Header;
    JsonTableModel(const Header &header_, QObject *parent = 0);

    bool setJson(const QJsonArray &array);
    QJsonArray getJson();
    bool setHeader(const Header &array);
    Header getHeader();

    virtual QJsonObject getJsonObject(const QModelIndex &index) const; // get row data

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual void groupData();
    virtual void unGroupData();
    void setColumnTitle(int section, QString title);
    bool setData(const QModelIndex &index, QJsonObject rowData, int role = Qt::EditRole);
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());

    private:
    Header tableHeader_;
    Header tableHoldHeader_;
    Header tableGroupedHeader_;
    QJsonArray tableJsonData_;
    QJsonArray tableHoldJsonData_;
};

#endif // JSONTABLEMODEL_H
