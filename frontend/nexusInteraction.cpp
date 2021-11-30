// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "./ui_mainwindow.h"
#include "mainwindow.h"
#include <QAction>
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

// Gathers all fields based on selected runs
void MainWindow::on_graph_clicked()
{
    // Clear related fields
    chart_->removeAllSeries();
    runsMenu_->clear();
    nexusMenu_->clear();

    QModelIndexList selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColum;
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;

    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
        runNos.append(runNo + ";");

        // Fills runsMenu_ with all runNo's
        auto *action = new QAction(runNo, this);
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()), this, SLOT(runToggled()));
        runsMenu_->addAction(action);
    }
    // Removes final ";"
    runNos.chop(1);

    QString url_str = "http://127.0.0.1:5000/getNexusFields/";
    QString cycle = ui_->cyclesBox->currentText().replace("journal", "cycle").replace(".xml", "");
    url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos;

    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
            SLOT(handle_result_fieldQuery(HttpRequestWorker *)));
    worker->execute(input);
}

// Fills field menu
void MainWindow::handle_result_fieldQuery(HttpRequestWorker *worker)
{
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        nexusMenu_->clear();
        foreach (const QJsonValue &value, worker->json_array)
        {
            // Fills nexusMenu_ with all columns
            auto *action = new QAction(value.toString(), this);
            action->setCheckable(true);
            connect(action, SIGNAL(triggered()), this, SLOT(fieldToggled()));
            nexusMenu_->addAction(action);
        }
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

// Handles field selection
void MainWindow::fieldToggled()
{
    // Gets signal object
    auto *fieldAction = qobject_cast<QAction *>(sender());

    // Gathers all selected runs
    QString runNos = "";
    for (auto *runAction : runsMenu_->actions())
    {
        if (runAction->isChecked() == true)
            runNos.append(runAction->text() + ";");
    }
    runNos.chop(1);

    if (runNos.size() == 0)
        return;

    if (fieldAction->isChecked() == false)
    {
        for (auto series : chart_->series())
        {
            if (series->name().contains(fieldAction->text().replace("/", ":")))
            {
                chart_->removeSeries(series);
            }
        }
        chart_->createDefaultAxes();
    }
    else
    {
        QString url_str = "http://127.0.0.1:5000/getNexusData/";
        QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");
        QString field = fieldAction->text().replace("/", ":");
        url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runNos + "/" + field;

        HttpRequestInput input(url_str);
        auto *worker = new HttpRequestWorker(this);
        connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handle_result_logData(HttpRequestWorker *)));
        worker->execute(input);
    }
}

// Handles run selection
void MainWindow::runToggled()
{
    auto *runAction = qobject_cast<QAction *>(sender());

    // Gathers selected fields
    QString fields = "";
    for (auto *fieldAction : nexusMenu_->actions())
    {
        if (fieldAction->isChecked() == true)
            fields.append(fieldAction->text().replace("/", ":") + ";");
    }
    // removes end ";"
    fields.chop(1);

    if (fields.size() == 0)
        return;

    if (runAction->isChecked() == false)
    {
        for (auto series : chart_->series())
        {
            if (series->name().contains(runAction->text()))
            {
                chart_->removeSeries(series);
            }
        }
        chart_->createDefaultAxes();
    }
    else
    {
        QString url_str = "http://127.0.0.1:5000/getNexusData/";
        QString cycle = ui_->cyclesBox->currentText().replace(0, 7, "cycle").replace(".xml", "");
        url_str += ui_->instrumentsBox->currentText() + "/" + cycle + "/" + runAction->text() + "/" + fields;
        HttpRequestInput input(url_str);
        auto *worker = new HttpRequestWorker(this);
        connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this,
                SLOT(handle_result_logData(HttpRequestWorker *)));
        worker->execute(input);
    }
}

// Handles log data
void MainWindow::handle_result_logData(HttpRequestWorker *worker)
{
    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        // For each Run
        foreach (const auto &runFields, worker->json_array)
        {
            auto runFieldsArray = runFields.toArray();

            // For each field
            foreach (const auto &fieldData, runFieldsArray)
            {
                auto fieldDataArray = fieldData.toArray();

                // For each plot point
                auto *series = new QLineSeries();

                // Set series ID
                QString name = fieldDataArray.first()[0].toString() + " " + fieldDataArray.first()[1].toString();
                series->setName(name);
                fieldDataArray.removeFirst();
                foreach (const auto &dataPair, fieldDataArray)
                {
                    auto dataPairArray = dataPair.toArray();
                    series->append(dataPairArray[0].toDouble(), dataPairArray[1].toDouble());
                }
                chart_->addSeries(series);
            }
        }
        // Resize chart
        chart_->createDefaultAxes();
    }
    else
    {
        // an error occurred
        msg = "Error2: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::customMenuRequested(QPoint pos)
{
    pos_ = pos;
    auto index = ui_->runDataTable->indexAt(pos);
    auto selectedRuns = ui_->runDataTable->selectionModel()->selectedRows();

    // Finds run number location in table
    int runNoColum;
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }

    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;

    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
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
    for (auto column = 0; column < proxyModel_->columnCount(); column++)
    {
        if (proxyModel_->headerData(column, Qt::Horizontal).toString() == "run_number")
        {
            runNoColum = column;
            break;
        }
    }
    // Gets all selected run numbers and fills graphing toggles
    QString runNos = "";
    QString runNo;
    // Concats runs
    for (auto run : selectedRuns)
    {
        runNo = proxyModel_->index(run.row(), runNoColum).data().toString();
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
    worker->execute(input);
}

// Handles log data
void MainWindow::handle_result_contextGraph(HttpRequestWorker *worker)
{
    auto *window = new QWidget;
    auto *contextChart = new QChart();
    auto *contextChartView = new QChartView(contextChart, window);

    QString msg;
    if (worker->error_type == QNetworkReply::NoError)
    {
        auto *timeAxis = new QDateTimeAxis();
        timeAxis->setFormat("yyyy-MM-dd<br>H:mm:ss");
        timeAxis->setTitleText("Real Time");
        bool firstRun = true;
        // For each Run
        foreach (const auto &runFields, worker->json_array)
        {
            auto runFieldsArray = runFields.toArray();

            auto startTime = QDateTime::fromString(runFieldsArray.first()[0].toString(), "yyyy-MM-dd'T'HH:mm:ss");
            auto endTime = QDateTime::fromString(runFieldsArray.first()[1].toString(), "yyyy-MM-dd'T'HH:mm:ss");
            runFieldsArray.removeFirst();

            if (firstRun)
                timeAxis->setRange(startTime, endTime);

            // For each field
            foreach (const auto &fieldData, runFieldsArray)
            {
                auto fieldDataArray = fieldData.toArray();

                // For each plot point
                auto *series = new QLineSeries();

                // Set series ID
                QString name = fieldDataArray.first()[0].toString() + " " + fieldDataArray.first()[1].toString();
                series->setName(name);
                fieldDataArray.removeFirst();
                foreach (const auto &dataPair, fieldDataArray)
                {
                    auto dataPairArray = dataPair.toArray();
                    series->append(startTime.addSecs(dataPairArray[0].toDouble()).toSecsSinceEpoch(),
                                   dataPairArray[1].toDouble());
                }
                if (startTime.addSecs(startTime.secsTo(QDateTime::fromSecsSinceEpoch(series->at(0).x()))) < timeAxis->min())
                    timeAxis->setMin(startTime.addSecs(startTime.secsTo(QDateTime::fromSecsSinceEpoch(series->at(0).x()))));
                if (endTime > timeAxis->max())
                    timeAxis->setMax(endTime);
                contextChart->addSeries(series);
            }
        }
        // Resize chart
        contextChart->createDefaultAxes();
        contextChart->addAxis(timeAxis, Qt::AlignBottom);
        contextChart->axes()[0]->hide();

        auto *absTimeAxis = new QValueAxis;
        absTimeAxis->setRange(0, timeAxis->max().toSecsSinceEpoch() - timeAxis->min().toSecsSinceEpoch());
        absTimeAxis->setTitleText("Absolute Time");
        contextChart->addAxis(absTimeAxis, Qt::AlignBottom);
        contextChart->axes()[3]->hide();

        auto *gridLayout = new QGridLayout(window);
        auto *testCheck = new QCheckBox("test", window);

        connect(testCheck, SIGNAL(stateChanged(int)), this, SLOT(toggleAxis(int)));

        gridLayout->addWidget(contextChartView, 0, 0);
        gridLayout->addWidget(testCheck, 1, 0);
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
    QList<QChartView *> help = graphParent->findChildren<QChartView *>();
    if (toggleBox->isChecked())
    {
        help[0]->chart()->axes()[2]->hide();
        help[0]->chart()->axes()[3]->show();
    }
    else
    {
        help[0]->chart()->axes()[2]->show();
        help[0]->chart()->axes()[3]->hide();
    }
}