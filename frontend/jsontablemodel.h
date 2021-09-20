// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#ifndef JSONTABLEMODEL_H
#define JSONTABLEMODEL_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QAbstractTableModel>
#include <QJsonDocument>
#include <QJsonArray>

// Model for json usage in table view
class JsonTableModel : public QAbstractTableModel
{
public:
    // Assigning custom data types for table headings
    typedef QMap<QString,QString> Heading;
    typedef QVector<Heading> Header;
    JsonTableModel( const Header& header, QObject * parent = 0);

    bool setJson( const QJsonArray& array );

    virtual QJsonObject getJsonObject( const QModelIndex &index ) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    Header m_header;
    QJsonArray m_json;
};

#endif // JSONTABLEMODEL_H
