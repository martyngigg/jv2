// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "graphwidget.h"
#include "./ui_graphwidget.h"
#include "chartview.h"
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QXYSeries>

GraphWidget::GraphWidget(QWidget *parent, QChart *chart) : QWidget(parent), ui_(new Ui::GraphWidget)
{
    ui_->setupUi(this);
    ui_->chartView->assignChart(chart);
    ui_->binWidths->setText("Counts/ µs");
}

GraphWidget::~GraphWidget() {}

ChartView *GraphWidget::getChartView() { return ui_->chartView; }

void GraphWidget::on_binWidths_clicked(bool checked)
{
    checked ? ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts/ &#181;s")
            : ui_->chartView->chart()->axes(Qt::Vertical)[0]->setTitleText("Counts");
    for (auto *series : ui_->chartView->chart()->series())
    {
        auto xySeries = qobject_cast<QXYSeries *>(series);
        auto points = xySeries->points();
        xySeries->clear();
        if (checked)
        {

            for (auto i = 0; i < points.count() - 1; i++)
            {
                auto binWidth = points[i + 1].x() - points[i].x();
                points[i].setY(points[i].y() / binWidth);
            }
        }
        else
        {
            for (auto i = 0; i < points.count() - 1; i++)
            {
                auto binWidth = points[i + 1].x() - points[i].x();
                points[i].setY(points[i].y() * binWidth);
            }
        }
        xySeries->append(points);
    }
}