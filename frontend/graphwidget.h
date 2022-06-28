// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include "chartview.h"
#include "httprequestworker.h"
#include <QChart>
#include <QChartView>
#include <QWidget>

namespace Ui
{
class GraphWidget;
}

class GraphWidget : public QWidget
{
    Q_OBJECT

    public:
    GraphWidget(QWidget *parent = nullptr, QChart *chart = nullptr, QString type = nullptr);
    ~GraphWidget();
    ChartView *getChartView();

    QString getChartRuns();
    QString getChartDetector();
    QJsonArray getChartData();

    void setChartRuns(QString chartRuns);
    void setChartDetector(QString chartDetector);
    void setChartData(QJsonArray chartData);
    void setLabel(QString label);

    public slots:
    void modifyAgainstString(QString values, bool checked);
    void modifyAgainstWorker(HttpRequestWorker *worker, bool checked);

    private:
    void getBinWidths();

    private slots:
    void runDivideSpinHandling(); // Handle normalisation conflicts
    void monDivideSpinHandling(); // Handle normalisation conflicts
    void on_countsPerMicrosecondCheck_stateChanged(int state);
    void on_countsPerMicroAmpCheck_stateChanged(int state);

    private:
    Ui::GraphWidget *ui_;
    QString run_;
    QString chartRuns_;
    QString chartDetector_;
    QJsonArray chartData_;
    QVector<QVector<double>> binWidths_;
    QString type_;
    QString modified_;

    signals:
    void muAmps(QString runs, bool checked, QString modified);
    void runDivide(QString currentDetector, QString run, bool checked);
    void monDivide(QString currentRun, QString mon, bool checked);
};

#endif
