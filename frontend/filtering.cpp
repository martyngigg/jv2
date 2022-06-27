// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>

// Hide column on view menu change
void MainWindow::columnHider(int state)
{
    auto *action = qobject_cast<QCheckBox *>(sender());

    for (auto i = 0; i < model_->columnCount(); ++i)
    {
        if (action->text() == headersMap_[model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString()])
        {
            switch (state)
            {
                case Qt::Unchecked:
                    ui_->runDataTable->setColumnHidden(i, true);
                    break;
                case Qt::Checked:
                    ui_->runDataTable->setColumnHidden(i, false);
                    break;
                default:
                    action->setCheckState(Qt::Checked);
            }
            break;
        }
    }
}

// Filter table data
void MainWindow::on_filterBox_textChanged(const QString &arg1)
{
    proxyModel_->setFilterString(arg1.trimmed());
    proxyModel_->setFilterKeyColumn(-1);
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);

    // Update search to new data
    if (searchString_ != "")
        updateSearch(searchString_);
}

// Groups table data
void MainWindow::on_groupButton_clicked(bool checked)
{
    if (checked)
    {
        model_->groupData();
        for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
            ui_->runDataTable->setColumnHidden(i, false);
        ui_->runDataTable->resizeColumnsToContents();
        // Make view match desired order
        ui_->runDataTable->horizontalHeader()->swapSections(ui_->runDataTable->horizontalHeader()->visualIndex(0), 0);
        ui_->runDataTable->horizontalHeader()->swapSections(ui_->runDataTable->horizontalHeader()->visualIndex(1), 1);
    }
    else
    {
        model_->unGroupData();
        for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
        {
            auto index = model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString();
            auto it = std::find_if(desiredHeader_.begin(), desiredHeader_.end(),
                                   [index](const auto &data) { return data.first == index; });
            if (it == desiredHeader_.end())
                ui_->runDataTable->setColumnHidden(i, true);
        }
        // Re-sort columns on change
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_->runDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(j);
                if (desiredHeader_[i].first == model_->headerData(logIndex, Qt::Horizontal, Qt::UserRole).toString())
                    ui_->runDataTable->horizontalHeader()->swapSections(j, i);
            }
        }
        ui_->runDataTable->resizeColumnsToContents();
    }
    updateSearch(searchString_);
}

// Clears filter parameters
void MainWindow::on_clearSearchButton_clicked() { ui_->filterBox->clear(); }

void MainWindow::on_actionMassSearchRB_No_triggered() { massSearch("RB No.", "experiment_identifier"); }

void MainWindow::on_actionMassSearchTitle_triggered() { massSearch("Title", "title"); }

void MainWindow::on_actionMassSearchUser_triggered() { massSearch("User name", "user_name"); }

void MainWindow::on_actionMassSearchRunRange_triggered() { massSearch("Run Range", "run_number"); }

void MainWindow::on_actionMassSearchDateRange_triggered() { massSearch("Date Range", "start_date"); }

void MainWindow::on_actionClear_cached_searches_triggered()
{
    cachedMassSearch_.clear();
    for (auto i = cyclesMenu_->actions().count() - 1; i >= 0; i--)
    {
        if (cyclesMenu_->actions()[i]->text()[0] == '[')
        {
            cyclesMenu_->removeAction(cyclesMenu_->actions()[i]);
        }
    }
    if (ui_->cycleButton->text()[0] == '[')
        cyclesMenu_->actions()[0]->trigger();
}

void MainWindow::goTo(HttpRequestWorker *worker, QString runNumber)
{
    setLoadScreen(false);
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        if (worker->response == "Not Found")
        {
            statusBar()->showMessage("Search query not found", 5000);
            return;
        }
        if (cyclesMap_[ui_->cycleButton->text()] == worker->response)
        {
            selectIndex(runNumber);
            return;
        }
        connect(this, &MainWindow::tableFilled, [=]() { selectIndex(runNumber); });
        for (auto i = 0; i < cyclesMenu_->actions().count(); i++)
        {
            if (cyclesMap_[cyclesMenu_->actions()[i]->text()] == worker->response)
                changeCycle(cyclesMenu_->actions()[i]->text());
        }
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

// Go-To run number
void MainWindow::on_actionRun_Number_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Find"), tr("Run No: "), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QString url_str = "http://127.0.0.1:5000/getGoToCycle/" + instName_ + "/" + textInput;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { goTo(workerProxy, textInput); });
    worker->execute(input);
    setLoadScreen(true);
}