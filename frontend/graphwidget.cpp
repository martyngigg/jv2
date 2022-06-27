// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "graphwidget.h"
#include "./ui_graphwidget.h"
#include "chartview.h"
#include "mainwindow.h"
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QDebug>
#include <QInputDialog>
#include <QJsonArray>
#include <QValueAxis>
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart, QString type) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    type_ = type;
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
    connect(ui_->divideByRunRadio, &QRadioButton::toggled, [=]() { runDivideSpinHandling(); });
    connect(ui_->divideByRunSpin, &QSpinBox::editingFinished, [=]() { runDivideSpinHandling(); });
    connect(ui_->divideByMonitorSpin, &QSpinBox::editingFinished, [=]() { monDivideSpinHandling(); });
    connect(ui_->divideByMonitorRadio, &QRadioButton::toggled, [=]() { monDivideSpinHandling(); });

    modified_ = "-1";
    ui_->divideByRunSpin->setSpecialValueText(tr(" "));
    ui_->divideByMonitorSpin->setSpecialValueText(tr(" "));
    ui_->divideByRunSpin->setValue(-1);
    ui_->divideByMonitorSpin->setValue(-1);
}

GraphWidget::~GraphWidget() {}

QString GraphWidget::getChartRuns() { return chartRuns_; }
QString GraphWidget::getChartDetector() { return chartDetector_; }
QJsonArray GraphWidget::getChartData() { return chartData_; }

void GraphWidget::setChartRuns(QString chartRuns) { chartRuns_ = chartRuns; }
void GraphWidget::setChartDetector(QString chartDetector) { chartDetector_ = chartDetector; }
void GraphWidget::setChartData(QJsonArray chartData)
{
    chartData_ = chartData;
    getBinWidths();
}
void GraphWidget::setLabel(QString label)
{
    return; // ui_->statusLabel->setText(label);
}

void GraphWidget::getBinWidths()
{
    binWidths_.clear();
    for (auto run : chartData_)
    {
        QVector<double> binWidths;
        auto runArray = run.toArray();
        for (auto i = 0; i < runArray.count() - 1; i++)
        {
            double binWidth = runArray.at(i + 1)[0].toDouble() - runArray.at(i)[0].toDouble();
            binWidths.append(binWidth);
        }
        binWidths_.append(binWidths);
    }
}

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::runDivideSpinHandling()
{
    // IF AMP CHECKED? toggle off -> on?
    if (ui_->divideByRunSpin->isEnabled())
    {
        ui_->countsPerMicrosecondCheck->setChecked(false);
        ui_->countsPerMicrosecondCheck->setEnabled(false);
    }
    else
        ui_->countsPerMicrosecondCheck->setEnabled(true);
    QString value = QString::number(ui_->divideByRunSpin->value());
    if (!ui_->divideByRunSpin->isEnabled())
        value = "-1";
    if (modified_ == value && ui_->divideByRunSpin->isEnabled())
        return;

    if (modified_ != "-1")
    {
        ui_->countsPerMicroAmpCheck->setChecked(false);
        if (type_ == "Detector")
            emit runDivide(chartDetector_, modified_, false);
        else
            emit monDivide(modified_, chartDetector_, false);
        modified_ = "-1";
        if (ui_->divideByRunSpin->isEnabled())
            ui_->countsPerMicroAmpCheck->setChecked(true);
    }

    if (value != "-1" && value != modified_)
    {
        bool toggle = ui_->countsPerMicroAmpCheck->isChecked();
        ui_->countsPerMicroAmpCheck->setChecked(false);
        if (type_ == "Detector")
            emit runDivide(chartDetector_, value, true);
        else
            emit monDivide(value, chartDetector_, true);
        modified_ = value;
        if (toggle)
            ui_->countsPerMicroAmpCheck->setChecked(true);
    }
}

void GraphWidget::monDivideSpinHandling()
{
    if (ui_->divideByMonitorSpin->isEnabled())
    {
        ui_->countsPerMicrosecondCheck->setChecked(false);
        ui_->countsPerMicrosecondCheck->setEnabled(false);
        ui_->countsPerMicroAmpCheck->setChecked(false);
        ui_->countsPerMicroAmpCheck->setEnabled(false);
    }
    else
    {
        ui_->countsPerMicrosecondCheck->setEnabled(true);
        ui_->countsPerMicroAmpCheck->setEnabled(true);
    }
    QString value = QString::number(ui_->divideByMonitorSpin->value());
    if (!ui_->divideByMonitorSpin->isEnabled())
        value = "-1";
    if (modified_ == value && ui_->divideByMonitorSpin->isEnabled())
        return;

    if (modified_ != "-1")
    {
        emit monDivide(chartRuns_, modified_, false);
        modified_ = "-1";
    }

    if (value != "-1" && value != modified_)
    {
        emit monDivide(chartRuns_, value, true);
        modified_ = value;
    }
}

void GraphWidget::on_countsPerMicrosecondCheck_stateChanged(int state)
{
    qreal max = 0;
    qreal min = 0;
    QString modifier = "/microSeconds";
    auto yAxisTitle = ui_->chartView->chart()->axes(Qt::Vertical)[0]->titleText();

    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (state == Qt::Checked)
        {
            for (auto j = 0; j < points.count(); j++)
            {
                auto hold = points[j].y() / binWidths_[i][j];
                if (i == 0 && j == 0)
                {
                    max = hold;
                    min = hold;
                }
                if (hold > max)
                    max = hold;
                else if (hold < min)
                    min = hold;
                points[j].setY(hold);
            }
            yAxisTitle.append(modifier);
        }
        else
        {
            for (auto j = 0; j < points.count(); j++)
            {
                auto hold = points[j].y() * binWidths_[i][j];
                if (i == 0 && j == 0)
                {
                    max = hold;
                    min = hold;
                }
                if (hold > max)
                    max = hold;
                else if (hold < min)
                    min = hold;
                points[j].setY(hold);
            }
            yAxisTitle.remove(modifier);
        }
        ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText(yAxisTitle);

        xySeries->replace(points);
        if (max == min)
        {
            max++;
            min--;
        }
        ui_->chartView->chart()->axes()[1]->setMax(max);
        ui_->chartView->chart()->axes()[1]->setMin(min);
    }
}

void GraphWidget::on_countsPerMicroAmpCheck_stateChanged(int state)
{
    if (state == Qt::Checked)
        emit muAmps(chartRuns_, true, modified_);
    else
        emit muAmps(chartRuns_, false, modified_);
}

void GraphWidget::modifyAgainstString(QString values, bool checked)
{
    qreal max = 0;
    qreal min = 0;
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        double val;
        if (values.split(";").count() > 1)
            val = values.split(";")[i].toDouble();
        else
            val = values.toDouble();
        if (values.split(";").count() > ui_->chartView->chart()->series().count())
            val = val / values.split(";").last().toDouble();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (checked)
        {

            for (auto j = 0; j < points.count(); j++)
            {
                auto hold = points[j].y() / val;
                if (i == 0 && j == 0)
                {
                    max = hold;
                    min = hold;
                }
                if (hold > max)
                    max = hold;
                else if (hold < min)
                    min = hold;
                points[j].setY(hold);
            }
        }
        else
        {
            for (auto j = 0; j < points.count(); j++)
            {
                auto hold = points[j].y() * val;
                if (i == 0 && j == 0)
                {
                    max = hold;
                    min = hold;
                }
                if (hold > max)
                    max = hold;
                else if (hold < min)
                    min = hold;
                points[j].setY(hold);
            }
        }
        xySeries->replace(points);
        if (max == min)
        {
            max++;
            min--;
        }
        ui_->chartView->chart()->axes()[1]->setMax(max);
        ui_->chartView->chart()->axes()[1]->setMin(min);
    }
}

void GraphWidget::modifyAgainstWorker(HttpRequestWorker *worker, bool checked)
{
    QJsonArray valueArray;
    qreal max = 0;
    qreal min = 0;
    auto dataType = worker->json_array[0].toArray()[2].toString(0);
    worker->json_array.removeFirst();
    for (auto i = 0; i < ui_->chartView->chart()->series().count(); i++)
    {
        if (worker->json_array.count() > 1)
            valueArray = worker->json_array[i].toArray();
        else
            valueArray = worker->json_array[0].toArray();
        auto xySeries = qobject_cast<QXYSeries *>(ui_->chartView->chart()->series()[i]);
        auto points = xySeries->points();
        if (checked)
        {
            for (auto j = 0; j < points.count(); j++)
            {
                auto val = valueArray.at(j)[1].toDouble();
                if (val != 0)
                {
                    auto hold = points[j].y() / val;
                    if (i == 0 && j == 0)
                    {
                        max = hold;
                        min = hold;
                    }
                    if (hold > max)
                        max = hold;
                    else if (hold < min)
                        min = hold;
                    points[j].setY(hold);
                }
            }
        }
        else
        {
            for (auto j = 0; j < points.count(); j++)
            {
                auto val = valueArray.at(j)[1].toDouble();
                if (val != 0)
                {
                    auto hold = points[j].y() * val;
                    if (i == 0 && j == 0)
                    {
                        max = hold;
                        min = hold;
                    }
                    if (hold > max)
                        max = hold;
                    else if (hold < min)
                        min = hold;
                    points[j].setY(hold);
                }
            }
        }
        xySeries->replace(points);
        if (QString::number(max) == QString::number(min))
        {
            max++;
            min--;
        }
        ui_->chartView->chart()->axes()[1]->setMax(max);
        ui_->chartView->chart()->axes()[1]->setMin(min);
    }
}