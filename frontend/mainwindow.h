// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "httprequestworker.h"
#include "jsontablemodel.h"
#include <QCheckBox>
#include <QMainWindow>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void fillInstruments();
  void initialiseElements();
private slots:
  void on_filterBox_textChanged(const QString &arg1);
  void handle_result_instruments(HttpRequestWorker *worker);
  void handle_result_cycles(HttpRequestWorker *worker);
  void on_instrumentsBox_currentIndexChanged(const QString &arg1);

  void on_cyclesBox_currentIndexChanged(const QString &arg1);

  void columnHider(int state);

private:
  Ui::MainWindow *ui;
  JsonTableModel *model;
  QSortFilterProxyModel *proxyModel;
  QMenu *viewMenu;
  JsonTableModel::Header header;
};
#endif // MAINWINDOW_H
