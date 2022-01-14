// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "chartview.h"
#include <QApplication>
#include <QDebug>
#include <QtGui/QMouseEvent>

ChartView::ChartView(QChart *chart, QWidget *parent) : QChartView(chart, parent)
{
    setRubberBand(QChartView::HorizontalRubberBand);
    setDragMode(QGraphicsView::NoDrag);
    this->setMouseTracking(true);
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    {
        switch (event->key())
        {
            case Qt::Key_Control:
                setRubberBand(QChartView::VerticalRubberBand);
                break;
            case Qt::Key_Left:
                chart()->scroll(-10, 0);
                break;
            case Qt::Key_Right:
                chart()->scroll(10, 0);
                break;
            case Qt::Key_Up:
                chart()->scroll(0, 10);
                break;
            case Qt::Key_Down:
                chart()->scroll(0, -10);
                break;
            default:
                QGraphicsView::keyPressEvent(event);
                break;
        }
    }
}

void ChartView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::wheelEvent(QWheelEvent *event)
{
    qreal factor;
    if (event->angleDelta().y() > 0)
        factor = 1.1;
    else
        factor = 0.91;

    auto graphArea = QRectF(chart()->plotArea().left(), chart()->plotArea().top(), chart()->plotArea().width() / factor,
                            chart()->plotArea().height() / factor);
    auto mousePos = mapFromGlobal(QCursor::pos());
    graphArea.moveCenter(mousePos);
    chart()->zoomIn(graphArea);
    auto delta = chart()->plotArea().center() - mousePos;
    chart()->scroll(delta.x(), -delta.y());
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        chart()->zoomIn();
        chart()->zoomOut();
        QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
        lastMousePos_ = event->pos();
        event->accept();
    }
    QChartView::mousePressEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton)
    {
        chart()->zoomReset();
        return;
    }
    if (event->button() == Qt::MiddleButton)
    {
        QApplication::restoreOverrideCursor();
    }
    QChartView::mouseReleaseEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    // pan the chart with a middle mouse drag
    if (event->buttons() & Qt::MiddleButton)
    {
        auto dPos = event->pos() - lastMousePos_;
        chart()->scroll(-dPos.x(), dPos.y());

        lastMousePos_ = event->pos();
        event->accept();
    }

    QChartView::mouseMoveEvent(event);
}