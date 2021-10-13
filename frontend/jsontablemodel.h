// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#ifndef JSONTABLEMODEL_H
#define JSONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QObject>
#include <QVector>

// Model for json usage in table view
class JsonTableModel : public QAbstractTableModel {
public:
  // Assigning custom data types for table headings
  typedef QMap<QString, QString> Heading;
  typedef QVector<Heading> Header;
  JsonTableModel(const Header &header, QObject *parent = 0);

  bool setJson(const QJsonArray &array);
  QJsonArray getJson();
  bool setHeader(const Header &array);
  Header getHeader();

  virtual QJsonObject getJsonObject(const QModelIndex &index) const;

  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &index,
                        int role = Qt::DisplayRole) const;
  virtual void groupData();
  virtual void unGroupData();

private:
  Header m_header;
  Header m_holdHeader;
  QJsonArray m_json;
  QJsonArray m_holdJson;
};

#endif // JSONTABLEMODEL_H
