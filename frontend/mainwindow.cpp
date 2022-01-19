// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QDebug>
#include <QDomDocument>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineSeries>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QWidgetAction>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    initialiseElements();
}

MainWindow::~MainWindow() { delete ui_; }

// Configure initial application state
void MainWindow::initialiseElements()
{
    auto instruments = getInstruments();
    fillInstruments(instruments);

    // First Iteration variable for set-up commands
    init_ = true;

    // View menu for column toggles
    viewMenu_ = ui_->menubar->addMenu("View");

    // Allows re-arranging of table columns
    ui_->runDataTable->horizontalHeader()->setSectionsMovable(true);
    ui_->runDataTable->horizontalHeader()->setDragEnabled(true);
    ui_->runDataTable->setAlternatingRowColors(true);
    ui_->runDataTable->setStyleSheet("alternate-background-color: #e7e7e6;");

    // Sets instrument to last used
    QSettings settings;
    QString recentInstrument = settings.value("recentInstrument").toString();
    auto instrumentIndex = ui_->instrumentsBox->findText(recentInstrument);
    if (instrumentIndex != -1)
        ui_->instrumentsBox->setCurrentIndex(instrumentIndex);
    else
        ui_->instrumentsBox->setCurrentIndex(ui_->instrumentsBox->count() - 1);

    // Disables closing data tab + handles tab closing
    ui_->tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_->runDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->runDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");

    ui_->searchContainer->setVisible(false);

    connect(ui_->action_Quit, SIGNAL(triggered()), this, SLOT(close()));
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (ui_->cyclesBox->count() == 0)
    {
        ui_->instrumentsBox->clear();
        QWidget::setEnabled(false);
    }
    QSettings settings;
    QString recentCycle = settings.value("recentCycle").toString();
    auto cycleIndex = ui_->cyclesBox->findText(recentCycle);

    // Sets cycle to last used/ most recent if unavailable
    if (ui_->instrumentsBox->currentText() != "")
    {
        if (cycleIndex != -1)
            ui_->cyclesBox->setCurrentIndex(cycleIndex);
        else
            ui_->cyclesBox->setCurrentIndex(ui_->cyclesBox->count() - 1);
    }
    else
        ui_->cyclesBox->setEnabled(false);
}

// Fill instrument list
void MainWindow::fillInstruments(QList<QPair<QString, QString>> instruments)
{
    // Only allow calls after initial population
    ui_->instrumentsBox->blockSignals(true);
    ui_->instrumentsBox->clear();
    foreach (const auto instrument, instruments)
    {
        ui_->instrumentsBox->addItem(instrument.first, instrument.second);
    }
    ui_->instrumentsBox->blockSignals(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings;
    settings.setValue("recentInstrument", ui_->instrumentsBox->currentText());
    settings.setValue("recentCycle", ui_->cyclesBox->currentText());

    // Close server
    QString url_str = "http://127.0.0.1:5000/shutdown";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);

    event->accept();
}

void MainWindow::massSearch(QString name, QString value)
{
    QString textInput =
        QInputDialog::getText(this, tr("Enter search query"), tr(name.append(": ").toUtf8()), QLineEdit::Normal);
    QString text = name.append(textInput);
    if (text.isEmpty())
        return;

    for (auto tuple : cachedMassSearch_)
    {
        if (std::get<1>(tuple) == text)
        {
            ui_->cyclesBox->setCurrentText("[" + std::get<1>(tuple) + "]");
            setLoadScreen(true);
            handle_result_cycles(std::get<0>(tuple));
            return;
        }
    }

    QString url_str =
        "http://127.0.0.1:5000/getAllJournals/" + ui_->instrumentsBox->currentText() + "/" + value + "/" + textInput;
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handle_result_cycles(HttpRequestWorker *)));
    worker->execute(input);
    cachedMassSearch_.append(std::make_tuple(worker, text));
    ui_->cyclesBox->addItem("[" + text + "]");
    ui_->cyclesBox->setCurrentText("[" + text + "]");
    setLoadScreen(true);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier && Qt::ShiftModifier)
    {
        if (ui_->searchContainer->isVisible())
            ui_->searchContainer->setVisible(false);
    }
    if (event->key() == Qt::Key_F && event->modifiers() == Qt::ControlModifier)
    {
        if (!ui_->searchContainer->isVisible())
            ui_->searchContainer->setVisible(true);
        ui_->searchBox->setFocus();
    }
    if (event->key() == Qt::Key_G && event->modifiers() == Qt::ControlModifier)
    {
        bool checked = ui_->groupButton->isChecked();
        ui_->groupButton->setChecked(!checked);
        on_groupButton_clicked(!checked);
    }
    if (event->key() == Qt::Key_F3 && event->modifiers() == Qt::ShiftModifier)
    {
        on_findUp_clicked();
        return;
    }
    if (event->key() == Qt::Key_F3)
        on_findDown_clicked();
}

void MainWindow::on_closeFind_clicked()
{
    ui_->searchContainer->setVisible(false);
    if (statusBar()->currentMessage() != "")
        ui_->runDataTable->selectionModel()->clearSelection();
    statusBar()->clearMessage();
}

QList<QPair<QString, QString>> MainWindow::getInstruments()
{
    QFile file("../extra/instrumentData.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    QList<QPair<QString, QString>> instruments;
    QPair<QString, QString> instrument;
    QDomNode node;
    QDomElement elem;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        instrument.first = elem.attribute("name");
        instrument.second = elem.elementsByTagName("type").item(0).toElement().text();
        instruments.append(instrument);
    }
    return instruments;
}

QList<QString> MainWindow::getFields(QString instrument, QString instType)
{
    QList<QString> desiredInstFields;
    QDomNodeList desiredInstrumentFields;

    QFile file("../extra/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto rootelem = dom.documentElement();
    auto instList = rootelem.elementsByTagName("inst");
    for (auto i = 0; i < instList.count(); i++)
    {
        if (instList.item(i).toElement().attribute("name") == instrument)
        {
            desiredInstrumentFields = instList.item(i).toElement().elementsByTagName("Column");
            break;
        }
    }
    if (desiredInstrumentFields.isEmpty())
    {
        auto configDefault = rootelem.elementsByTagName(instType).item(0).toElement();
        auto configDefaultFields = configDefault.elementsByTagName("Column");

        if (configDefaultFields.isEmpty())
        {
            QFile file("../extra/instrumentData.xml");
            file.open(QIODevice::ReadOnly);
            dom.setContent(&file);
            file.close();
            auto rootelem = dom.documentElement();
            auto defaultColumns = rootelem.elementsByTagName(instType).item(0).toElement().elementsByTagName("Column");
            for (int i = 0; i < defaultColumns.count(); i++)
            {
                desiredInstFields.append(
                    defaultColumns.item(i).toElement().elementsByTagName("Data").item(0).toElement().text());
            }
            return desiredInstFields;
        }

        for (int i = 0; i < configDefaultFields.count(); i++)
        {
            desiredInstFields.append(
                configDefaultFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text());
        }
        return desiredInstFields;
    }
    for (int i = 0; i < desiredInstrumentFields.count(); i++)
    {
        desiredInstFields.append(
            desiredInstrumentFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text());
    }
    return desiredInstFields;
}

void MainWindow::savePref()
{

    QFile file("../extra/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    QString currentFields;
    int realIndex;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        realIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_->runDataTable->isColumnHidden(realIndex))
        {
            currentFields += ui_->runDataTable->horizontalHeader()->model()->headerData(realIndex, Qt::Horizontal).toString();
            currentFields += ",;";
        }
    }
    currentFields.chop(1);

    QDomNode node;
    QDomElement elem;
    QDomElement columns;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        if (elem.attribute("name") == ui_->instrumentsBox->currentText())
        {
            auto oldColumns = elem.elementsByTagName("Columns");
            if (!oldColumns.isEmpty())
                elem.removeChild(elem.elementsByTagName("Columns").item(0));
            columns = dom.createElement("Columns");
            for (QString field : currentFields.split(";"))
            {
                auto preferredFieldsElem = dom.createElement("Column");
                auto preferredFieldsDataElem = dom.createElement("Data");
                preferredFieldsElem.setAttribute("Title", "placeholder");
                preferredFieldsDataElem.appendChild(dom.createTextNode(field.left(field.indexOf(","))));
                preferredFieldsElem.appendChild(preferredFieldsDataElem);
                columns.appendChild(preferredFieldsElem);
            }
            elem.appendChild(columns);
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QFile file("../extra/tableConfig.xml");
        file.open(QIODevice::WriteOnly);
        file.write(dom.toByteArray());
        file.close();
    }
}

void MainWindow::setLoadScreen(bool state)
{
    if (state)
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QWidget::setEnabled(false);
    }
    else
    {
        QWidget::setEnabled(true);
        QGuiApplication::restoreOverrideCursor();
    }
}

void MainWindow::on_actionMassSearchRB_No_triggered() { massSearch("RB No.", "run_number"); }

void MainWindow::on_actionMassSearchTitle_triggered() { massSearch("Title", "title"); }

void MainWindow::on_actionMassSearchUser_triggered() { massSearch("User name", "user_name"); }

void MainWindow::on_actionClear_cached_searches_triggered()
{
    cachedMassSearch_.clear();
    for (auto i = ui_->cyclesBox->count() - 1; i >= 0; i--)
    {
        if (ui_->cyclesBox->itemText(i)[0] == '[')
        {
            ui_->cyclesBox->removeItem(i);
        }
    }
}
