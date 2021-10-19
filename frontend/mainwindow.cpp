// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QCheckBox>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QWidgetAction>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    initialiseElements();
}

MainWindow::~MainWindow() { delete ui_; }

// Configure initial application state
void MainWindow::initialiseElements()
{
    fillInstruments();

    // First Iteration variable for set-up commands
    init_ = true;

    // View menu for column toggles
    viewMenu_ = ui_->menubar->addMenu("View");

    // Allows re-arranging of table columns
    ui_->runDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_->runDataTable->horizontalHeader()->setDragEnabled(true);
    ui_->runDataTable->setAlternatingRowColors(true);
    ui_->runDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");

    // Sets instrument to last used
    QSettings settings;
    QString recentInstrument = settings.value("recentInstrument").toString();
    auto instrumentIndex = ui_->instrumentsBox->findText(recentInstrument);
    if (instrumentIndex != -1)
        ui_->instrumentsBox->setCurrentIndex(instrumentIndex);
    else
        ui_->instrumentsBox->setCurrentIndex(ui_->instrumentsBox->count() - 1);
    // Sets cycle to most recently viewed
    recentCycle();
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    QSettings settings;
    QString recentCycle = settings.value("recentCycle").toString();
    auto cycleIndex = ui_->cyclesBox->findText(recentCycle);

    // Sets cycle to last used/ most recent if unavailable
    if (ui_->instrumentsBox->currentText() != "")
    {
        if (cycleIndex != -1)
            ui_->cyclesBox->setCurrentIndex(cycleIndex);
        else if (ui_->cyclesBox->currentText() != "")
            ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
    }
    else
        ui_->cyclesBox->setEnabled(false);
    if (cycleIndex != -1)
        ui_->cyclesBox->setCurrentIndex(cycleIndex);
    else
        ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
}

// Fill instrument list
void MainWindow::fillInstruments()
{
    QList<QString> instruments = {"merlin", "nimrod", "sandals", "iris"};

    // Only allow calls after initial population
    ui_->instrumentsBox->blockSignals(true);
    ui_->instrumentsBox->clear();
    foreach (const QString instrument, instruments)
    {
        ui_->instrumentsBox->addItem(instrument);
    }
    ui_->instrumentsBox->blockSignals(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings;
    settings.setValue("recentInstrument", ui_->instrumentsBox->currentText());
    settings.setValue("recentCycle", ui_->cyclesBox->currentText());
    event->accept();
}
