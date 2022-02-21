// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "chartview.h"
#include "graphwidget.h"
#include "mainwindow.h"
#include <QAction>
#include <QCategoryAxis>
#include <QChartView>
#include <QDateTimeAxis>
#include <QInputDialog>
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
    auto runNos = getRunNos();

    QString url_str = "http://127.0.0.1:5000/getNexusFields/";
    QString cycle = cyclesMap_[ui_->cycleButton->text()];
    cycle.replace(0, 7, "cycle").replace(".xml", "");
    url_str += instName_ + "/" + cycle + "/" + runNos;

    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_contextMenu(HttpRequestWorker *)));
    contextMenu_->popup(ui_->runDataTable->viewport()->mapToGlobal(pos_));
    worker->execute(input);
}

// Fills field menu
void MainWindow::handle_result_contextMenu(HttpRequestWorker *worker)
{
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

        auto *action = new QAction("Select runs with same title", this);
        connect(action, SIGNAL(triggered()), this, SLOT(selectSimilar()));
        contextMenu_->addAction(action);

        auto *action2 = new QAction("Plot detector spectrum", this);
        connect(action2, SIGNAL(triggered()), this, SLOT(getSpectrumCount()));
        contextMenu_->addAction(action2);
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

QString MainWindow::getRunNos()
{
    // Gathers all selected runs
    auto selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();
    // Finds run number location in table
    int runNoColumn;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        if (model_->headerData(i, Qt::Horizontal, Qt::UserRole).toString() == "run_number")
        {
            runNoColumn = i;
            break;
        }
    }
    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;
    // Concats runs
    for (auto run : selectedRuns)
    {
        runNo = model_->index(run.row(), runNoColumn).data().toString();
        if (runNo.contains("-") || runNo.contains(","))
        {
            QString groupedRuns;
            auto runArr = runNo.split(",");
            foreach (const auto &string, runArr)
            {
                if (string.contains("-"))
                {
                    for (auto i = string.split("-")[0].toInt(); i <= string.split("-")[1].toInt(); i++)
                        groupedRuns += QString::number(i) + ";";
                }
                else
                    groupedRuns += string + ";";
            }
            groupedRuns.chop(1);
            runNos.append(groupedRuns + ";");
        }
        else
            runNos.append(runNo + ";");
    }
    // Removes final ";"
    runNos.chop(1);
    return runNos;
}

void MainWindow::contextGraph()
{
    // Gets signal object
    auto *contextAction = qobject_cast<QAction *>(sender());

    auto runNos = getRunNos();

    // Error handling
    if (runNos.size() == 0)
        return;
    QString url_str = "http://127.0.0.1:5000/getNexusData/";
    QString cycle = cyclesMap_[ui_->cycleButton->text()];
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    QString field = contextAction->data().toString().replace("/", ":");
    url_str += instName_ + "/" + cycle + "/" + runNos + "/" + field;

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
    auto *relTimeChart = new QChart();
    auto *relTimeChartView = new ChartView(relTimeChart, window);

    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        auto *timeAxis = new QDateTimeAxis();
        timeAxis->setFormat("yyyy-MM-dd<br>H:mm:ss");
        dateTimeChart->addAxis(timeAxis, Qt::AlignBottom);

        auto *dateTimeYAxis = new QValueAxis();
        dateTimeYAxis->setRange(0, 0);

        auto *dateTimeStringAxis = new QCategoryAxis();
        QStringList categoryValues;

        auto *relTimeXAxis = new QValueAxis();
        relTimeXAxis->setTitleText("Relative Time (s)");
        relTimeChart->addAxis(relTimeXAxis, Qt::AlignBottom);

        auto *relTimeYAxis = new QValueAxis();

        auto *relTimeStringAxis = new QCategoryAxis();

        QList<QString> chartFields;
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
                relTimeXAxis->setRange(0, 0);
            }

            foreach (const auto &fieldData, runFieldsArray)
            {
                auto fieldDataArray = fieldData.toArray();
                fieldDataArray.removeFirst();
                if (!fieldDataArray.first()[1].isString())
                    break;
                foreach (const auto &dataPair, fieldDataArray)
                {
                    auto dataPairArray = dataPair.toArray();
                    categoryValues.append(dataPairArray[1].toString());
                }
            }

            if (!categoryValues.isEmpty())
            {
                categoryValues.removeDuplicates();
                categoryValues.sort();
            }
            if (firstRun)
            {
                if (!categoryValues.isEmpty())
                {
                    dateTimeChart->addAxis(dateTimeStringAxis, Qt::AlignLeft);
                    relTimeChart->addAxis(relTimeStringAxis, Qt::AlignLeft);
                }
                else
                {
                    dateTimeChart->addAxis(dateTimeYAxis, Qt::AlignLeft);
                    relTimeChart->addAxis(relTimeYAxis, Qt::AlignLeft);
                }
                firstRun = false;
            }
            // For each field
            foreach (const auto &fieldData, runFieldsArray)
            {
                auto fieldDataArray = fieldData.toArray();

                // For each plot point
                auto *dateSeries = new QLineSeries();
                auto *relSeries = new QLineSeries();

                connect(dateSeries, &QLineSeries::hovered, [=](const QPointF point, bool hovered) {
                    dateTimeChartView->setHovered(point, hovered, dateSeries->name());
                });
                connect(dateTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                        SLOT(showStatus(qreal, qreal, QString)));
                connect(dateTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));
                connect(relSeries, &QLineSeries::hovered, [=](const QPointF point, bool hovered) {
                    relTimeChartView->setHovered(point, hovered, relSeries->name());
                });
                connect(relTimeChartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this,
                        SLOT(showStatus(qreal, qreal, QString)));
                connect(relTimeChartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

                // Set dateSeries ID
                QString name = fieldDataArray.first()[0].toString();
                QString field = fieldDataArray.first()[1].toString().section(':', -1);
                if (!chartFields.contains(field))
                    chartFields.append(field);
                dateSeries->setName(name);
                relSeries->setName(name);
                fieldDataArray.removeFirst();

                if (fieldDataArray.first()[1].isString())
                {
                    foreach (const auto &dataPair, fieldDataArray)
                    {
                        auto dataPairArray = dataPair.toArray();
                        dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                           categoryValues.indexOf(dataPairArray[1].toString()));
                        relSeries->append(dataPairArray[0].toDouble(), categoryValues.indexOf(dataPairArray[1].toString()));
                    }
                }
                else
                {
                    foreach (const auto &dataPair, fieldDataArray)
                    {
                        auto dataPairArray = dataPair.toArray();
                        dateSeries->append(startTime.addSecs(dataPairArray[0].toDouble()).toMSecsSinceEpoch(),
                                           dataPairArray[1].toDouble());
                        relSeries->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
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

                if (relSeries->at(0).x() < relTimeXAxis->min())
                    relTimeXAxis->setMin(relSeries->at(0).x());
                if (relSeries->at(relSeries->count() - 1).x() > relTimeXAxis->max())
                    relTimeXAxis->setMax(relSeries->at(relSeries->count() - 1).x());

                dateTimeChart->addSeries(dateSeries);
                dateSeries->attachAxis(timeAxis);
                relTimeChart->addSeries(relSeries);
                relSeries->attachAxis(relTimeXAxis);
                if (categoryValues.isEmpty())
                {
                    dateSeries->attachAxis(dateTimeYAxis);
                    relSeries->attachAxis(relTimeYAxis);
                }
                else
                {
                    dateSeries->attachAxis(dateTimeStringAxis);
                    relSeries->attachAxis(relTimeStringAxis);
                }
            }
        }

        if (!categoryValues.isEmpty())
        {
            dateTimeStringAxis->setRange(0, categoryValues.count() - 1);
            relTimeStringAxis->setRange(0, categoryValues.count() - 1);
            for (auto i = 0; i < categoryValues.count(); i++)
            {
                dateTimeStringAxis->append(categoryValues[i], i);
                relTimeStringAxis->append(categoryValues[i], i);
            }
            dateTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
            relTimeStringAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        }

        relTimeYAxis->setRange(dateTimeYAxis->min(), dateTimeYAxis->max());

        auto *gridLayout = new QGridLayout(window);
        auto *axisToggleCheck = new QCheckBox("Plot relative to run start times", window);

        connect(axisToggleCheck, SIGNAL(stateChanged(int)), this, SLOT(toggleAxis(int)));

        gridLayout->addWidget(dateTimeChartView, 1, 0, -1, -1);
        gridLayout->addWidget(relTimeChartView, 1, 0, -1, -1);
        relTimeChartView->hide();
        gridLayout->addWidget(axisToggleCheck, 0, 0);
        QString tabName;
        for (auto i = 0; i < chartFields.size(); i++)
        {
            tabName += chartFields[i];
            if (i < chartFields.size() - 1)
                tabName += ",";
        }
        if (!categoryValues.isEmpty())
        {
            dateTimeStringAxis->setTitleText(tabName);
            relTimeStringAxis->setTitleText(tabName);
        }
        else
        {
            dateTimeYAxis->setTitleText(tabName);
            relTimeYAxis->setTitleText(tabName);
        }
        ui_->tabWidget->addTab(window, tabName);
        QString runs;
        for (auto series : dateTimeChart->series())
            runs.append(series->name() + ", ");
        runs.chop(2);
        QString toolTip = instDisplayName_ + "\n" + tabName + "\n" + runs;
        ui_->tabWidget->setTabToolTip(ui_->tabWidget->count() - 1, toolTip);
        ui_->tabWidget->setCurrentIndex(ui_->tabWidget->count() - 1);
        dateTimeChartView->setFocus();
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
        tabCharts[1]->setFocus();
    }
    else
    {
        tabCharts[0]->show();
        tabCharts[0]->setFocus();
        tabCharts[1]->hide();
    }
}

void MainWindow::showStatus(qreal x, qreal y, QString title)
{
    QString message;
    auto *chartView = qobject_cast<ChartView *>(sender());
    auto yVal = QString::number(y);
    if (QString(chartView->chart()->axes(Qt::Vertical)[0]->metaObject()->className()) == QString("QCategoryAxis"))
    {
        auto *yAxis = qobject_cast<QCategoryAxis *>(chartView->chart()->axes(Qt::Vertical)[0]);
        yVal = yAxis->categoriesLabels()[(int)y];
    }
    if (QString(chartView->chart()->axes(Qt::Horizontal)[0]->metaObject()->className()) == QString("QDateTimeAxis"))
        message = QDateTime::fromMSecsSinceEpoch(x).toString("yyyy-MM-dd HH:mm:ss") + ", " + yVal;
    else
        message = QString::number(x) + ", " + yVal;
    statusBar()->showMessage("Run " + title + ": " + message);
}

void MainWindow::handleSpectraCharting(HttpRequestWorker *worker)
{
    auto *chart = new QChart();
    auto *window = new GraphWidget(this, chart);
    ChartView *chartView = window->getChartView();

    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        QString field = "Detector ";
        QString runs;
        bool init = true;
        foreach (const auto &run, worker->json_array)
        {
            field += init ? run.toString() : nullptr;
            if (init)
            {
                init = false;
                continue;
            }
            auto runArray = run.toArray();
            // For each plot point
            auto *series = new QLineSeries();

            connect(series, &QLineSeries::hovered,
                    [=](const QPointF point, bool hovered) { chartView->setHovered(point, hovered, series->name()); });
            connect(chartView, SIGNAL(showCoordinates(qreal, qreal, QString)), this, SLOT(showStatus(qreal, qreal, QString)));
            connect(chartView, SIGNAL(clearCoordinates()), statusBar(), SLOT(clearMessage()));

            // Set dateSeries ID
            QString name = runArray.first().toString();
            runs += name + ", ";
            series->setName(name);
            runArray.removeFirst();

            for (auto i = 0; i < runArray.count() - 1; i++)
            {
                const auto &dataPairTOFStart = runArray.at(i);
                auto dataPairTOFStartArray = dataPairTOFStart.toArray();
                const auto &dataPairTOFEnd = runArray.at(i + 1);
                auto dataPairTOFEndArray = dataPairTOFEnd.toArray();
                auto centreBin =
                    dataPairTOFStart[0].toDouble() + (dataPairTOFEnd[0].toDouble() - dataPairTOFStart[0].toDouble()) / 2;
                series->append(centreBin, dataPairTOFStart[1].toDouble());
            }
            chart->addSeries(series);
        }
        chart->createDefaultAxes();
        chart->axes(Qt::Horizontal)[0]->setTitleText("Time of flight, &#181;s");
        chart->axes(Qt::Vertical)[0]->setTitleText("Counts");
        QString tabName = field;
        ui_->tabWidget->addTab(window, tabName);
        ui_->tabWidget->setCurrentIndex(ui_->tabWidget->count() - 1);
        runs.chop(2);
        QString toolTip = field + "\n" + runs;
        ui_->tabWidget->setTabToolTip(ui_->tabWidget->count() - 1, toolTip);
        chartView->setFocus();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::getSpectrumCount()
{
    auto runNos = getRunNos();
    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = cyclesMap_[ui_->cycleButton->text()];
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    QString url_str = "http://127.0.0.1:5000/getSpectrumRange/";
    url_str += instName_ + "/" + cycle + "/" + runNos;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(plotSpectra(HttpRequestWorker *)));
    setLoadScreen(true);
    worker->execute(input);
}
void MainWindow::plotSpectra(HttpRequestWorker *count)
{
    setLoadScreen(false);
    auto spectraCount = count->response.toUtf8();
    spectraCount.chop(1);
    auto spectrumNumber = QInputDialog::getInt(this, tr("Plot Detector Spectrum"),
                                               tr("Enter detector spectrum to plot (0-" + spectraCount + "):"), 0, 0,
                                               count->response.toInt() - 1, 1);
    auto runNos = getRunNos();
    // Error handling
    if (runNos.size() == 0)
        return;

    QString cycle = cyclesMap_[ui_->cycleButton->text()];
    cycle.replace(0, 7, "cycle").replace(".xml", "");

    QString url_str = "http://127.0.0.1:5000/getSpectrum/";
    url_str += instName_ + "/" + cycle + "/" + runNos + "/" + QString::number(spectrumNumber);
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handleSpectraCharting(HttpRequestWorker *)));
    worker->execute(input);
}