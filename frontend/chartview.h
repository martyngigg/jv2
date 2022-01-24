// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include <QtCharts/QChartView>
#include <QtWidgets/QRubberBand>

class ChartView : public QChartView
{
    Q_OBJECT

    public:
    ChartView(QChart *chart, QWidget *parent = 0);

    public slots:
    void setHovered(const QPointF point, bool hovered);

    signals:
    void showCoordinates(qreal x, qreal y);
    void clearCoordinates();

    protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    private:
    QPointF lastMousePos_;
    bool hovered_;
    QGraphicsSimpleTextItem *coordLabelX_;
    QGraphicsSimpleTextItem *coordLabelY_;
    QGraphicsSimpleTextItem *coordStartLabelX_;
    QGraphicsSimpleTextItem *coordStartLabelY_;
};

#endif // CHARTVIEW_H
