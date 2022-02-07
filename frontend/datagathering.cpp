// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QWidgetAction>

// Fills cycles box on request completion
void MainWindow::handle_result_instruments(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        auto response = worker->response;

        // Prevents unwanted firing
        ui_->cyclesBox->blockSignals(true);
        QString cycleText = ui_->cyclesBox->currentText();
        ui_->cyclesBox->clear();
        foreach (const QJsonValue &value, worker->json_array)
        {
            // removes header_ file
            if (value.toString() != "journal.xml")
            {
                auto displayName =
                    "Cycle " + value.toString().split("_")[1] + "/" + value.toString().split("_")[2].remove(".xml");
                ui_->cyclesBox->addItem(displayName, value.toString());
            }
        }
        ui_->cyclesBox->blockSignals(false);

        // Keep cycle over instruments
        auto cycleIndex = ui_->cyclesBox->findText(cycleText);
        if (!init_)
        {
            if (cycleIndex != -1)
                ui_->cyclesBox->setCurrentIndex(cycleIndex);
            else
                ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
        }
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
    if (init_)
    {
        // Sets cycle to most recently viewed
        recentCycle();
        init_ = false;
    }
}

// Fills table view with run
void MainWindow::handle_result_cycles(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        // Get desired fields and titles from config files
        desiredHeader_ = getFields(instName_, instType_);
        auto jsonArray = worker->json_array;
        auto jsonObject = jsonArray.at(0).toObject();
        // Add columns to header and give titles where applicable
        header_.clear();
        foreach (const QString &key, jsonObject.keys())
        {
            // Find matching indices
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [key](const auto &data) { return data.first == key; });
            if (it != desiredHeader_.end())
                header_.push_back(JsonTableModel::Heading({{"title", it->second}, {"index", key}}));
            else
                header_.push_back(JsonTableModel::Heading({{"title", key}, {"index", key}}));
        }

        // Sets and fills table data
        model_ = new JsonTableModel(header_, this);
        proxyModel_ = new QSortFilterProxyModel;
        proxyModel_->setSourceModel(model_);
        ui_->runDataTable->setModel(proxyModel_);
        model_->setJson(jsonArray);
        ui_->runDataTable->show();

        // Fills viewMenu_ with all columns
        viewMenu_->clear();
        viewMenu_->addAction("savePref", this, SLOT(savePref()));
        viewMenu_->addSeparator();
        foreach (const QString &key, jsonObject.keys())
        {

            QCheckBox *checkBox = new QCheckBox(viewMenu_);
            QWidgetAction *checkableAction = new QWidgetAction(viewMenu_);
            checkableAction->setDefaultWidget(checkBox);
            checkBox->setText(key);
            checkBox->setCheckState(Qt::PartiallyChecked);
            viewMenu_->addAction(checkableAction);
            connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(columnHider(int)));

            // Filter table based on desired headers
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [key](const auto &data) { return data.first == key; });
            // If match found
            if (it != desiredHeader_.end())
                checkBox->setCheckState(Qt::Checked);
            else
                checkBox->setCheckState(Qt::Unchecked);
        }
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_->runDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(j);
                // If index matches model data, swap columns in view
                if (desiredHeader_[i].first == model_->headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
                {
                    ui_->runDataTable->horizontalHeader()->swapSections(j, i);
                }
            }
        }
        ui_->runDataTable->resizeColumnsToContents();
        updateSearch(searchString_);
        ui_->filterBox->clear();
        emit tableFilled();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

// Update cycles list when Instrument changed
void MainWindow::currentInstrumentChanged(const QString &arg1)
{
    cachedMassSearch_.clear();

    // Configure api call
    QString url_str = "http://127.0.0.1:5000/getCycles/" + arg1;
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);

    // Call result handler when request completed
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_instruments(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Populate table with cycle data
void MainWindow::on_cyclesBox_currentIndexChanged(int index)
{
    // Handle possible undesired calls
    auto currentText = ui_->cyclesBox->itemText(index);
    ui_->cyclesBox->setDisabled(currentText.isEmpty());
    ui_->filterBox->setDisabled(currentText.isEmpty());
    if (currentText.isEmpty())
        return;

    if (currentText[0] == '[')
    {
        auto it = std::find_if(cachedMassSearch_.begin(), cachedMassSearch_.end(), [currentText](const auto &tuple) {
            return std::get<1>(tuple) == currentText.mid(1, currentText.length() - 2);
        });
        if (it != cachedMassSearch_.end())
        {
            setLoadScreen(true);
            handle_result_cycles(std::get<0>(*it));
        }
        return;
    }

    QString url_str = "http://127.0.0.1:5000/getJournal/" + instName_ + "/" + ui_->cyclesBox->itemData(index).toString();
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);

    // Call result handler when request completed
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handle_result_cycles(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}
