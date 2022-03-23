// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "jsontablemodel.h"
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QDateTime>
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
#include <QTimer>
#include <QWidgetAction>
#include <QtGui>

#include "./ui_graphwidget.h"
#include "graphwidget.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    initialiseElements();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [=]() { checkForUpdates(); });
    timer->start(30000);
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
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentInstrument = settings.value("recentInstrument").toString();
    int instrumentIndex = -1;
    bool found = false;
    for (auto i = 0; i < instrumentsMenu_->actions().count(); i++)
        if (instrumentsMenu_->actions()[i]->text() == recentInstrument)
        {
            instrumentsMenu_->actions()[i]->trigger();
            found = true;
        }
    if (!found)
        instrumentsMenu_->actions()[instrumentsMenu_->actions().count() - 1]->trigger();

    // Disables closing data tab + handles tab closing
    ui_->tabWidget->tabBar()->setTabButton(0, QTabBar::RightSide, 0);
    connect(ui_->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));

    // Context menu stuff
    ui_->runDataTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_->runDataTable, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customMenuRequested(QPoint)));
    contextMenu_ = new QMenu("Context");

    connect(ui_->action_Quit, SIGNAL(triggered()), this, SLOT(close()));

    searchString_ = "";

    ui_->runDataTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (cyclesMenu_->actions().count() == 0)
        QWidget::setEnabled(false);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    QString recentCycle = settings.value("recentCycle").toString();
    // Sets cycle to last used/ most recent if unavailable
    for (QAction *action : cyclesMenu_->actions())
    {
        if (action->text() == recentCycle)
        {
            action->trigger();
            return;
        }
    }
    cyclesMenu_->actions()[cyclesMenu_->actions().count() - 1]->trigger();
}

// Fill instrument list
void MainWindow::fillInstruments(QList<std::tuple<QString, QString, QString>> instruments)
{
    // Only allow calls after initial population
    instrumentsMenu_ = new QMenu("instrumentsMenu");
    cyclesMenu_ = new QMenu("cyclesMenu");

    connect(ui_->instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_->instrumentButton->mapToGlobal(QPoint(0, ui_->instrumentButton->height()))); });
    connect(ui_->cycleButton, &QPushButton::clicked,
            [=]() { cyclesMenu_->exec(ui_->cycleButton->mapToGlobal(QPoint(0, ui_->cycleButton->height()))); });
    foreach (const auto instrument, instruments)
    {
        auto *action = new QAction(std::get<2>(instrument), this);
        connect(action, &QAction::triggered, [=]() { changeInst(instrument); });
        instrumentsMenu_->addAction(action);
    }
}

// Handle Instrument selection
void MainWindow::changeInst(std::tuple<QString, QString, QString> instrument)
{
    instType_ = std::get<1>(instrument);
    instName_ = std::get<0>(instrument);
    instDisplayName_ = std::get<2>(instrument);
    ui_->instrumentButton->setText(instDisplayName_);
    currentInstrumentChanged(instName_);
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "ISIS", "jv2");
    settings.setValue("recentInstrument", instDisplayName_);
    settings.setValue("recentCycle", ui_->cycleButton->text());

    // Close server
    QString url_str = "http://127.0.0.1:5000/shutdown";
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    worker->execute(input);

    event->accept();
}

void MainWindow::massSearch(QString name, QString value)
{
    QString textInput = QInputDialog::getText(this, tr("Find"), tr(name.append(": ").toUtf8()), QLineEdit::Normal);
    QString text = name.append(textInput);
    if (textInput.isEmpty())
        return;

    for (auto tuple : cachedMassSearch_)
    {
        if (std::get<1>(tuple) == text)
        {
            for (QAction *action : cyclesMenu_->actions())
            {
                if (action->text() == "[" + std::get<1>(tuple) + "]")
                    action->trigger();
            }
            setLoadScreen(true);
            return;
        }
    }

    QString url_str = "http://127.0.0.1:5000/getAllJournals/" + instName_ + "/" + value + "/" + textInput;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, SIGNAL(on_execution_finished(HttpRequestWorker *)), this, SLOT(handle_result_cycles(HttpRequestWorker *)));
    worker->execute(input);

    cachedMassSearch_.append(std::make_tuple(worker, text));

    auto *action = new QAction("[" + text + "]", this);
    connect(action, &QAction::triggered, [=]() { changeCycle("[" + text + "]"); });
    cyclesMenu_->addAction(action);
    ui_->cycleButton->setText("[" + text + "]");
    setLoadScreen(true);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_G && event->modifiers() == Qt::ControlModifier)
    {
        bool checked = ui_->groupButton->isChecked();
        ui_->groupButton->setChecked(!checked);
        on_groupButton_clicked(!checked);
    }
    if (event->key() == Qt::Key_R && event->modifiers() == Qt::ControlModifier)
        checkForUpdates();

    if (event->key() == Qt::Key_K && event->modifiers() == Qt::ControlModifier)
    {
        auto index = model_->index(0, 0);
        auto data = model_->getJsonObject(index);
        model_->insertRows(0, 1);
        model_->setData(index, data);
    }

    if (event->key() == Qt::Key_F && event->modifiers() & Qt::ControlModifier && Qt::ShiftModifier)
    {
        searchString_ = "";
        updateSearch(searchString_);
        return;
    }
}

// Get instrument data from config file
QList<std::tuple<QString, QString, QString>> MainWindow::getInstruments()
{
    QFile file(":/instrumentData.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();
    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    QList<std::tuple<QString, QString, QString>> instruments;
    std::tuple<QString, QString, QString> instrument;
    QDomNode node;
    QDomElement elem;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        auto instrumentDisplayName = elem.elementsByTagName("displayName").item(0).toElement().text();
        auto instrumentType = elem.elementsByTagName("type").item(0).toElement().text();
        auto instrumentName = elem.attribute("name");
        instruments.append(std::make_tuple(instrumentName, instrumentType, instrumentDisplayName));
    }
    return instruments;
}

// Get the desired fields and their titles
std::vector<std::pair<QString, QString>> MainWindow::getFields(QString instrument, QString instType)
{
    std::vector<std::pair<QString, QString>> desiredInstFields;
    QDomNodeList desiredInstrumentFields;

    QFile file(":/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    std::pair<QString, QString> column;

    auto rootelem = dom.documentElement();
    auto instList = rootelem.elementsByTagName("inst");
    for (auto i = 0; i < instList.count(); i++)
    {
        if (instList.item(i).toElement().attribute("name").toLower() == instrument)
        {
            desiredInstrumentFields = instList.item(i).toElement().elementsByTagName("Column");
            break;
        }
    }
    // If inst preferences blank
    if (desiredInstrumentFields.isEmpty())
    {
        auto configDefault = rootelem.elementsByTagName(instType).item(0).toElement();
        auto configDefaultFields = configDefault.elementsByTagName("Column");
        // If config preferences blank
        if (configDefaultFields.isEmpty())
        {
            QFile file(":/instrumentData.xml");
            file.open(QIODevice::ReadOnly);
            dom.setContent(&file);
            file.close();
            auto rootelem = dom.documentElement();
            auto defaultColumns = rootelem.elementsByTagName(instType).item(0).toElement().elementsByTagName("Column");
            // Get config preferences
            for (int i = 0; i < defaultColumns.count(); i++)
            {
                // Get column index and title from xml
                column.first = defaultColumns.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
                column.second = defaultColumns.item(i).toElement().attribute("name");
                desiredInstFields.push_back(column);
            }
            return desiredInstFields;
        }
        // Get config default
        for (int i = 0; i < configDefaultFields.count(); i++)
        {
            column.first = configDefaultFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
            column.second = configDefaultFields.item(i).toElement().attribute("name");
            desiredInstFields.push_back(column);
        }
        return desiredInstFields;
    }
    // Get instrument preferences
    for (int i = 0; i < desiredInstrumentFields.count(); i++)
    {
        column.first = desiredInstrumentFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
        column.second = desiredInstrumentFields.item(i).toElement().attribute("name");
        desiredInstFields.push_back(column);
    }
    return desiredInstFields;
}

void MainWindow::savePref()
{

    QFile file(":/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");
    // Get current table fields
    QString currentFields;
    int realIndex;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        realIndex = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (!ui_->runDataTable->isColumnHidden(realIndex))
        {
            currentFields += model_->headerData(realIndex, Qt::Horizontal, Qt::UserRole).toString();
            currentFields += ",";
            currentFields += model_->headerData(realIndex, Qt::Horizontal).toString();
            currentFields += ",;";
        }
    }
    currentFields.chop(1);

    // Add preferences to xml file
    QDomNode node;
    QDomElement elem;
    QDomElement columns;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        if (elem.attribute("name") == instName_)
        {
            auto oldColumns = elem.elementsByTagName("Columns");
            if (!oldColumns.isEmpty())
                elem.removeChild(elem.elementsByTagName("Columns").item(0));
            columns = dom.createElement("Columns");
            for (QString field : currentFields.split(";"))
            {
                auto preferredFieldsElem = dom.createElement("Column");
                auto preferredFieldsDataElem = dom.createElement("Data");
                preferredFieldsElem.setAttribute("name", field.split(",")[1]);
                preferredFieldsDataElem.appendChild(dom.createTextNode(field.split(",")[0]));
                preferredFieldsElem.appendChild(preferredFieldsDataElem);
                columns.appendChild(preferredFieldsElem);
            }
            elem.appendChild(columns);
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QFile file(":/tableConfig.xml");
        file.open(QIODevice::WriteOnly);
        file.write(dom.toByteArray());
        file.close();
    }
}

void MainWindow::clearPref()
{

    QFile file(":/tableConfig.xml");
    file.open(QIODevice::ReadOnly);
    QDomDocument dom;
    dom.setContent(&file);
    file.close();

    auto rootelem = dom.documentElement();
    auto nodelist = rootelem.elementsByTagName("inst");

    // Clear preferences from xml file
    QDomNode node;
    QDomElement elem;
    QDomElement columns;
    for (auto i = 0; i < nodelist.count(); i++)
    {
        node = nodelist.item(i);
        elem = node.toElement();
        if (elem.attribute("name") == instName_)
        {
            auto oldColumns = elem.elementsByTagName("Columns");
            if (!oldColumns.isEmpty())
                elem.removeChild(elem.elementsByTagName("Columns").item(0));
        }
    }
    if (!dom.toByteArray().isEmpty())
    {
        QFile file(":/tableConfig.xml");
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

void MainWindow::checkForUpdates()
{
    QString url_str = "http://127.0.0.1:5000/pingCycle/" + instName_;
    HttpRequestInput input(url_str);
    auto *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { refresh(workerProxy->response); });
    worker->execute(input);
}

void MainWindow::refresh(QString status)
{
    if (status != "")
    {
        qDebug() << "Update";
        currentInstrumentChanged(instName_);
        if (cyclesMap_[cyclesMenu_->actions()[cyclesMenu_->actions().count() - 1]->text()] != status)
        {
            auto displayName = "Cycle " + status.split("_")[1] + "/" + status.split("_")[2].remove(".xml");
            cyclesMap_[displayName] = status;

            auto *action = new QAction(displayName, this);
            connect(action, &QAction::triggered, [=]() { changeCycle(displayName); });
            cyclesMenu_->addAction(action);
        }
        else if (cyclesMap_[ui_->cycleButton->text()] == status)
        {
            QString url_str = "http://127.0.0.1:5000/updateJournal/" + instName_ + "/" + status + "/" +
                              model_->getJsonObject(model_->index(model_->rowCount() - 1, 0))["run_number"].toString();
            HttpRequestInput input(url_str);
            auto *worker = new HttpRequestWorker(this);
            connect(worker, &HttpRequestWorker::on_execution_finished,
                    [=](HttpRequestWorker *workerProxy) { update(workerProxy); });
            worker->execute(input);
        }
    }
    else
    {
        qDebug() << "no change";
        return;
    }
}

void MainWindow::update(HttpRequestWorker *worker)
{
    for (auto row : worker->json_array)
    {
        auto rowObject = row.toObject();
        model_->insertRows(model_->rowCount(), 1);
        auto index = model_->index(model_->rowCount() - 1, 0);
        model_->setData(index, rowObject);
    }
}