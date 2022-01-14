// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "chartview.h"
#include "mainwindow.h"
#include <QAction>
#include <QCategoryAxis>
#include <QChartView>
#include <QDateTimeAxis>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QValueAxis>
#include <QWidgetAction>
#include <algorithm>

void MainWindow::customMenuRequested(QPoint pos)
{
    pos_ = pos;
    auto index = ui_->runDataTable->indexAt(pos);
    auto selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColum;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        if (ui_->runDataTable->horizontalHeader()->model()->headerData(i, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = i;
            break;
        }
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;

    for (auto run : selectedRuns)
    {
        runNo = model_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");
    }
    // Removes final ";"
    runNos.chop(1);

    QString url_str = "http://127.0.0.1:5000/getNexusFields/";
    QString cycle = ui_->cyclesBox->currentText().replace("journal", "cycle").replace(".xml", "");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos;

    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextMenu(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Fills field menu
void MainWindow::handle_result_contextMenu(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        contextMenu_->clear();

        foreach (const auto &log, worker->json_array)
        {
            auto logArray = log.toArray();
            auto name = logArray.first().toString().toUpper();
            name.chop(2);
            auto formattedName = name.append("og");
            auto *subMenu = new QMenu("Plot from " + formattedName);
            logArray.removeFirst();
            if (logArray.size() > 0)
                contextMenu_->addMenu(subMenu);

            auto logArrayVar = logArray.toVariantList();
            std::sort(logArrayVar.begin(), logArrayVar.end(),
                      [](QVariant &v1, QVariant &v2) { return v1.toString() < v2.toString(); });

            foreach (const auto &block, logArrayVar)
            {
                // Fills contextMenu with all columns
                QString path = block.toString();
                auto *action = new QAction(path.right(path.size() - path.lastIndexOf("/") - 1), this);
                action->setData(path);
                connect(action, SIGNAL(triggered()), this, SLOT(contextGraph()));
                subMenu->addAction(action);
            }
        }
        contextMenu_->popup(ui_->runDataTable->viewport()->mapToGlobal(pos_));
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::contextGraph()
{
    // Gets signal object
    auto *contextAction = qobject_cast<QAction *>(sender());

    // Gathers all selected runs
    auto selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();
    // Finds run number location in table
    int runNoColum;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        if (ui_->runDataTable->horizontalHeader()->model()->headerData(i, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = i;
            break;
        }
    }
    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;
    // Concats runs
    for (auto run : selectedRuns)
    {
        runNo = model_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");
    }
    // Removes final ";"
    runNos.chop(1);
    // Error handling
    if (runNos.size() == 0)
        return;

    QString url_str = "http://127.0.0.1:5000/getNexusData/";
    QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");
    QString field = contextAction->data().toString().replace("/", ":");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos + "/" + field;

    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextGraph(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}

// Handles log data
void MainWindow::handle_result_contextGraph(HttpRequestWorker *worker)
{
    setLoadScreen(false);
    auto *window = new QWidget;
    auto *dateTimeChart = new QChart();
    auto *dateTimeChartView = new ChartView(dateTimeChart, window);
    auto *absTimeChart = new QChart();
    auto *absTimeChartView = new ChartView(absTimeChart, window);

    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        auto *timeAxis = new QDateTimeAxis();
        timeAxis->setFormat("yyyy-MM-dd<br>H:mm:ss");
        timeAxis->setTitleText("Real Time");
        dateTimeChart->addAxis(timeAxis, Qt::AlignBottom);

        auto *dateTimeYAxis = new QValueAxis();
        dateTimeYAxis->setRange(0, 0);
        dateTimeChart->addAxis(dateTimeYAxis, Qt::AlignLeft);

        auto *dateTimeStringAxis = new QCategoryAxis();
        QStringList categoryValues;
        dateTimeChart->addAxis(dateTimeStringAxis, Qt::AlignLeft);

        auto *absTimeXAxis = new QValueAxis();
        absTimeXAxis->setTitleText("Absolute Time");
        absTimeChart->addAxis(absTimeXAxis, Qt::AlignBottom);

        auto *absTimeYAxis = new QValueAxis();
        absTimeChart->addAxis(absTimeYAxis, Qt::AlignLeft);

        auto *absTimeStringAxis = new QCategoryAxis();
        absTimeChart->addAxis(absTimeStringAxis, Qt::AlignLeft);

        bool firstRun = true;
        // For each Run
        foreach (const auto &runFields, worker->json_array)
        {
            auto runFieldsArray = runFields.toArray();

            auto startTime = QDateTime::fromString(runFieldsArray.first()[0].toString(), "yyyy-MM-dd'T'HH:mm:ss");
            auto endTime = QDateTime::fromString(runFieldsArray.first()[1].toString(), "yyyy-MM-dd'T'HH:mm:ss");
            runFieldsArray.removeFirst();

            if (firstRun)
            {
                timeAxis->setRange(startTime, endTime);
                absTimeXAxis->setRange(0, 0);
                firstRun = false;
            }

            // For each field
            foreach (const auto &fieldData, runFieldsArray)
            {
                auto fieldDataArray = fieldData.toArray();

                // For each plot point
                auto *dateSeries = new QLineSeries();
                auto *absSeries = new QLineSeries();

                // Set dateSeries ID
                QString name = fieldDataArray.first()[0].toString() + " " + fieldDataArray.first()[1].toString();
                dateSeries->setName(name);
                absSeries->setName(name);
                fieldDataArray.removeFirst();

                if (fieldDataArray.first()[1].isString())
                {
                    foreach (const auto &dataPair, fieldDataArray)
                    {
                        auto dataPairArray = dataPair.toArray();
                        categoryValues.append(dataPairArray[1].toString());
                        dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                           dataPairArray[1].toString().right(2).toDouble());
                        absSeries->append(dataPairArray[0].toDouble(), dataPairArray[1].toString().right(2).toDouble());
                    }
                }
                else
                {
                    foreach (const auto &dataPair, fieldDataArray)
                    {
                        auto dataPairArray = dataPair.toArray();
                        dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                           dataPairArray[1].toDouble());
                        absSeries->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
                        if (dataPairArray[1].toDouble() < dateTimeYAxis->min())
                            dateTimeYAxis->setMin(dataPairArray[1].toDouble());
                        if (dataPairArray[1].toDouble() > dateTimeYAxis->max())
                            dateTimeYAxis->setMax(dataPairArray[1].toDouble());
                    }
                }
                if (startTime.addSecs(startTime.secsTo(QDateTime::fromMSecsSinceEpoch(dateSeries->at(0).x()))) <
                    timeAxis->min())
                    timeAxis->setMin(
                        startTime.addSecs(startTime.secsTo(QDateTime::fromMSecsSinceEpoch(dateSeries->at(0).x()))));
                if (endTime > timeAxis->max())
                    timeAxis->setMax(endTime);

                if (absSeries->at(0).x() < absTimeXAxis->min())
                    absTimeXAxis->setMin(absSeries->at(0).x());
                if (absSeries->at(absSeries->count() - 1).x() > absTimeXAxis->max())
                    absTimeXAxis->setMax(absSeries->at(absSeries->count() - 1).x());

                dateTimeChart->addSeries(dateSeries);
                dateSeries->attachAxis(timeAxis);
                absTimeChart->addSeries(absSeries);
                absSeries->attachAxis(absTimeXAxis);
                if (categoryValues.isEmpty())
                {
                    dateSeries->attachAxis(dateTimeYAxis);
                    absSeries->attachAxis(absTimeYAxis);
                }
                else
                {
                    dateSeries->attachAxis(dateTimeStringAxis);
                    absSeries->attachAxis(absTimeStringAxis);
                }
            }
        }

        if (!categoryValues.isEmpty())
        {
            categoryValues.removeDuplicates();
            categoryValues.sort();
            dateTimeStringAxis->setRange(0, categoryValues.count() - 1);
            absTimeStringAxis->setRange(0, categoryValues.count() - 1);
            for (auto i = 0; i < categoryValues.count(); i++)
            {
                dateTimeStringAxis->append(categoryValues[i], i);
                absTimeStringAxis->append(categoryValues[i], i);
            }
            dateTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
            absTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        }

        absTimeYAxis->setRange(dateTimeYAxis->min(), dateTimeYAxis->max());

        auto *gridLayout = new QGridLayout(window);
        auto *axisToggleCheck = new QCheckBox("show absolute time", window);

        connect(axisToggleCheck, SIGNAL(stateChanged(int)), this, SLOT(toggleAxis(int)));

        gridLayout->addWidget(dateTimeChartView, 1, 0, -1, -1);
        gridLayout->addWidget(absTimeChartView, 1, 0, -1, -1);
        absTimeChartView->hide();
        gridLayout->addWidget(axisToggleCheck, 0, 0);
        ui_->tabWidget->addTab(window, "graph");
        ui_->tabWidget->setCurrentIndex(ui_->tabWidget->count() - 1);
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::removeTab(int index) { delete ui_->tabWidget->widget(index); }

void MainWindow::toggleAxis(int state)
{
    auto *toggleBox = qobject_cast<QCheckBox *>(sender());
    auto *graphParent = toggleBox->parentWidget();
    auto tabCharts = graphParent->findChildren<QChartView *>();
    if (toggleBox->isChecked())
    {
        tabCharts[0]->hide();
        tabCharts[1]->show();
    }
    else
    {
        tabCharts[0]->show();
        tabCharts[1]->hide();
    }
}