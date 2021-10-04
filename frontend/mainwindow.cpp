// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSortFilterProxyModel>
#include <QWidgetAction>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  initialiseElements();
}

MainWindow::~MainWindow() { delete ui; }

// Hide column on view menu change
void MainWindow::columnHider(int state) {
  QCheckBox *action = qobject_cast<QCheckBox *>(sender());

  for (int i = 0; i < ui->runDataTable->horizontalHeader()->count(); i++) {
    if (action->text() ==
        ui->runDataTable->horizontalHeader()->model()->headerData(
            i, Qt::Horizontal)) {
      switch (action->checkState()) {
      case Qt::Unchecked:
        ui->runDataTable->setColumnHidden(i, true);
        break;
      case Qt::Checked:
        ui->runDataTable->setColumnHidden(i, false);
        break;
      default:
        ui->runDataTable->setColumnHidden(i, false);
      }
      break;
    }
  }
}

void MainWindow::initialiseElements() {
  fillInstruments();
  viewMenu = ui->menubar->addMenu("View");
  ui->runDataTable->horizontalHeader()->setSectionsMovable(true);
  ui->runDataTable->horizontalHeader()->setDragEnabled(true);
}

// Fill instrument list
void MainWindow::fillInstruments() {
  QList<QString> instruments = {"default", "merlin", "nimrod", "sandals",
                                "iris"};
  ui->instrumentsBox->clear();
  foreach (const QString instrument, instruments) {
    ui->instrumentsBox->addItem(instrument);
  }
}

void MainWindow::on_instrumentsBox_currentIndexChanged(const QString &arg1) {
  // Handle possible undesired calls
  if (arg1 == "default" || arg1 == "") {
    ui->cyclesBox->clear();
    ui->cyclesBox->addItem("default");
    return;
  }
  QString url_str = "http://127.0.0.1:5000/getCycles/" + arg1;
  HttpRequestInput input(url_str);
  HttpRequestWorker *worker = new HttpRequestWorker(this);
  // Call result handler when request completed
  connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
          SLOT(handle_result_instruments(HttpRequestWorker *)));
  worker->execute(&input);
}

void MainWindow::on_cyclesBox_currentIndexChanged(const QString &arg1) {
  // Handle possible undesired calls
  if (arg1 == "default" || arg1 == "") {
    return;
  }
  QString url_str = "http://127.0.0.1:5000/getJournal/" +
                    ui->instrumentsBox->currentText() + "/" + arg1;
  HttpRequestInput input(url_str);
  HttpRequestWorker *worker = new HttpRequestWorker(this);
  // Call result handler when request completed
  connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
          SLOT(handle_result_cycles(HttpRequestWorker *)));
  worker->execute(&input);
}

// Filter table data
void MainWindow::on_filterBox_textChanged(const QString &arg1) {
  proxyModel->setFilterFixedString(arg1.trimmed());
  proxyModel->setFilterKeyColumn(-1);
  proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
}

// Fills cycles box
void MainWindow::handle_result_instruments(HttpRequestWorker *worker) {
  QString msg;

  if (worker->error_type == QNetworkReply::NoError) {
    auto response = worker->response;
    ui->cyclesBox->clear();
    ui->cyclesBox->addItem("default");
    foreach (const QJsonValue &value, worker->json_array) {
      // removes header file
      if (value.toString() != "journal.xml")
        ui->cyclesBox->addItem(value.toString());
    }
  } else {
    // an error occurred
    msg = "Error1: " + worker->error_str;
    QMessageBox::information(this, "", msg);
  }
}

// Fills table view with run
void MainWindow::handle_result_cycles(HttpRequestWorker *worker) {
  QString msg;

  if (worker->error_type == QNetworkReply::NoError) {
    // Get keys from json data
    auto jsonArray = worker->json_array;
    auto jsonObject = jsonArray.at(0).toObject();
    header.clear();
    viewMenu->clear();
    foreach (const QString &key, jsonObject.keys()) {
      header.push_back(
          JsonTableModel::Heading({{"title", key}, {"index", key}}));

      QCheckBox *checkBox = new QCheckBox(viewMenu);
      QWidgetAction *checkableAction = new QWidgetAction(viewMenu);
      checkableAction->setDefaultWidget(checkBox);
      checkBox->setText(key);
      checkBox->setCheckState(Qt::Checked);
      connect(checkBox, SIGNAL(stateChanged(int)), this,
              SLOT(columnHider(int)));
      viewMenu->addAction(checkableAction);
    }
    model = new JsonTableModel(header, this);
    proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(model);
    ui->runDataTable->setModel(proxyModel);
    model->setJson(jsonArray);
    ui->runDataTable->show();

  } else {
    // an error occurred
    msg = "Error2: " + worker->error_str;
    QMessageBox::information(this, "", msg);
  }
}