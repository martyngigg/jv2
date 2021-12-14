// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "httprequestworker.h"
#include "jsontablemodel.h"
#include <QChart>
#include <QCheckBox>
#include <QDomDocument>
#include <QMainWindow>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void fillInstruments(QList<QPair<QString, QString>> instruments);
    void initialiseElements();
    void goToCurrentFoundIndex(QModelIndex index);
    QList<QPair<QString, QString>> getInstruments();
    QList<QString> getFields(QString instrument, QString instType);
    void setLoadScreen(bool state);
    private slots:
    void on_filterBox_textChanged(const QString &arg1);
    void on_searchBox_textChanged(const QString &arg1);
    void handle_result_instruments(HttpRequestWorker *worker);
    void handle_result_cycles(HttpRequestWorker *worker);
    void on_instrumentsBox_currentTextChanged(const QString &arg1);
    void on_cyclesBox_currentTextChanged(const QString &arg1);
    void on_groupButton_clicked(bool checked);
    void columnHider(int state);
    void on_clearSearchButton_clicked();
    void on_findUp_clicked();
    void on_findDown_clicked();
    void on_searchAll_clicked();
    void on_closeFind_clicked();
    void recentCycle();
    void customMenuRequested(QPoint pos);
    void handle_result_contextGraph(HttpRequestWorker *worker);
    void contextGraph();
    void handle_result_contextMenu(HttpRequestWorker *worker);
    void removeTab(int index);
    void toggleAxis(int state);
    void savePref();

    protected:
    // Window close event
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);

    private:
    Ui::MainWindow *ui_;
    JsonTableModel *model_;
    QSortFilterProxyModel *proxyModel_;
    QMenu *viewMenu_;
    QMenu *contextMenu_;
    JsonTableModel::Header header_;
    QList<QString> desiredHeader_;
    QModelIndexList foundIndices_;
    int currentFoundIndex_;
    bool init_;
    QChart *chart_;
    QPoint pos_;
};
#endif // MAINWINDOW_H
