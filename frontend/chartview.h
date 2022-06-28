// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include "httprequestworker.h"
#include <QtCharts/QChartView>
#include <QtWidgets/QRubberBand>

class ChartView : public QChartView
{
    Q_OBJECT

    public:
    ChartView(QChart *chart, QWidget *parent = 0);
    ChartView(QWidget *parent = 0);
    void assignChart(QChart *chart);

    public slots:
    void setHovered(const QPointF point, bool hovered, QString title);
    void addSeries(HttpRequestWorker *worker);

    signals:
    void showCoordinates(qreal x, qreal y, QString title);
    void clearCoordinates();

    protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    private slots:
    void setGraphics(QChart *chart);

    private:
    QPointF lastMousePos_;
    QString hovered_;
    QGraphicsSimpleTextItem *coordLabelX_;
    QGraphicsSimpleTextItem *coordLabelY_;
    QGraphicsSimpleTextItem *coordStartLabelX_;
    QGraphicsSimpleTextItem *coordStartLabelY_;
};

#endif // CHARTVIEW_H
