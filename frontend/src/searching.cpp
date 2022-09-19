// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <QInputDialog>
#include <tuple>

// Search table data
void MainWindow::updateSearch(const QString &arg1)
{
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (arg1.isEmpty())
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }
    // Find all occurences of search string in table elements
    for (auto i = 0; i < proxyModel_->columnCount(); ++i)
    {
        auto location = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (ui_->runDataTable->isColumnHidden(location) == false)
            foundIndices_.append(
                proxyModel_->match(proxyModel_->index(0, location), Qt::DisplayRole, arg1, -1, Qt::MatchContains));
    }
    // Select first match
    if (foundIndices_.size() > 0)
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

// Select previous match
void MainWindow::findUp()
{
    // Boundary/ error handling
    if (foundIndices_.size() > 0)
    {
        if (currentFoundIndex_ >= 1)
            currentFoundIndex_ -= 1;
        else
            currentFoundIndex_ = foundIndices_.size() - 1;
        goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                                 QString::number(foundIndices_.size()) + " Results");
    }
}

// Select next match
void MainWindow::findDown()
{
    // Boundary/ error handling
    if (foundIndices_.size() > 0)
    {
        currentFoundIndex_ = ++currentFoundIndex_ % foundIndices_.size();
        goToCurrentFoundIndex(foundIndices_[currentFoundIndex_]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": " + QString::number(currentFoundIndex_ + 1) + "/" +
                                 QString::number(foundIndices_.size()) + " Results");
    }
}

// Select all matches
void MainWindow::selectAllSearches()
{
    // Error handling
    if (foundIndices_.size() > 0)
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        currentFoundIndex_ = -1;
        for (auto i = 0; i < foundIndices_.size(); ++i)
        {
            ui_->runDataTable->selectionModel()->setCurrentIndex(foundIndices_[i],
                                                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        statusBar()->showMessage("Find \"" + searchString_ + "\": Selecting " + QString::number(foundIndices_.size()) +
                                 " Results");
    }
}
void MainWindow::goToCurrentFoundIndex(QModelIndex index)
{
    ui_->runDataTable->selectionModel()->setCurrentIndex(index,
                                                         QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void MainWindow::selectIndex(QString runNumber)
{
    ui_->runDataTable->selectionModel()->clearSelection();

    updateSearch(runNumber);
    statusBar()->showMessage("Found run " + runNumber + " in " + ui_->cycleButton->text(), 5000);
    disconnect(this, &MainWindow::tableFilled, nullptr, nullptr);
}

void MainWindow::selectSimilar()
{
    int TitleColumn;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        if (model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString() == "title")
        {
            TitleColumn = i;
            break;
        }
    }
    QString title = proxyModel_->index(ui_->runDataTable->rowAt(pos_.y()), TitleColumn).data().toString();
    for (auto i = 0; i < model_->rowCount(); i++)
    {
        if (proxyModel_->index(i, TitleColumn).data().toString() == title)
            ui_->runDataTable->selectionModel()->setCurrentIndex(proxyModel_->index(i, TitleColumn),
                                                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}
void MainWindow::on_actionSearch_triggered()
{
    QString textInput =
        QInputDialog::getText(this, tr("Find"), tr("Find in current run data (RB, user, title,...):"), QLineEdit::Normal);
    searchString_ = textInput;
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (textInput.isEmpty())
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }
    // Find all occurences of search string in table elements
    for (auto i = 0; i < proxyModel_->columnCount(); ++i)
    {
        auto location = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (ui_->runDataTable->isColumnHidden(location) == false)
            foundIndices_.append(
                proxyModel_->match(proxyModel_->index(0, location), Qt::DisplayRole, textInput, -1, Qt::MatchContains));
    }
    // Select first match
    if (foundIndices_.size() > 0)
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

void MainWindow::on_actionSelectNext_triggered() { findDown(); }
void MainWindow::on_actionSelectPrevious_triggered() { findUp(); }
void MainWindow::on_actionSelectAll_triggered() { selectAllSearches(); }