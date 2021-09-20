// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "jsontablemodel.h"
#include <QJsonObject>

// Model to handle json data in table view
JsonTableModel::JsonTableModel(const JsonTableModel::Header &header,
                               QObject *parent)
    : QAbstractTableModel(parent), m_header(header) {}

// Sets json data to populate table
bool JsonTableModel::setJson(const QJsonArray &array) {
  beginResetModel();
  m_json = array;
  endResetModel();
  return true;
}

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  switch (orientation) {
  case Qt::Horizontal:
    return m_header[section]["title"];
  case Qt::Vertical:
    // return section + 1;
    return QVariant();
  default:
    return QVariant();
  }
}

int JsonTableModel::rowCount(const QModelIndex &parent) const {
  return m_json.size();
}

int JsonTableModel::columnCount(const QModelIndex &parent) const {
  return m_header.size();
}

QJsonObject JsonTableModel::getJsonObject(const QModelIndex &index) const {
  const QJsonValue &value = m_json[index.row()];
  return value.toObject();
}

// Fills table view
QVariant JsonTableModel::data(const QModelIndex &index, int role) const {
  switch (role) {
  case Qt::DisplayRole: {
    QJsonObject obj = getJsonObject(index);
    const QString &key = m_header[index.column()]["index"];
    if (obj.contains(key)) {
      QJsonValue v = obj[key];

      if (v.isString()) {
        return v.toString();
      } else if (v.isDouble()) {
        return QString::number(v.toDouble());
      } else {
        return QVariant();
      }
    } else {
      return QVariant();
    }
  }
  case Qt::ToolTipRole:
    return QVariant();
  default:
    return QVariant();
  }
}
