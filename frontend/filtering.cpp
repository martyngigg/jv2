// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"

// Hide column on view menu change
void MainWindow::columnHider(int state)
{
    auto *action = qobject_cast<QCheckBox *>(sender());

    for (auto i = 0; i < model_->columnCount(); ++i)
    {
        if (action->text() == model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString())
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
    proxyModel_->setFilterFixedString(arg1.trimmed());
    proxyModel_->setFilterKeyColumn(-1);
    proxyModel_->setFilterCaseSensitivity(Qt::CaseInsensitive);

    // Update search to new data
    on_searchBox_textChanged(ui_->searchBox->text());
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
        int logIndex;
        for (auto i = 0; i < desiredHeader_.size(); ++i)
        {
            for (auto j = 0; j < ui_->runDataTable->horizontalHeader()->count(); ++j)
            {
                logIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(j);
                if (desiredHeader_[i].first ==
                    ui_->runDataTable->horizontalHeader()->model()->headerData(logIndex, Qt::Horizontal).toString())
                    ui_->runDataTable->horizontalHeader()->swapSections(j, i);
            }
        }
        ui_->runDataTable->resizeColumnsToContents();
    }
}

// Clears filter parameters
void MainWindow::on_clearSearchButton_clicked() { ui_->filterBox->clear(); }