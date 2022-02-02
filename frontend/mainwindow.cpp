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
    ui_->instrumentsBox->hide();
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

    ui_->searchContainer->setVisible(false);

    connect(ui_->action_Quit, SIGNAL(triggered()), this, SLOT(close()));
}

// Sets cycle to most recently viewed
void MainWindow::recentCycle()
{
    // Disable selections if api fails
    if (ui_->cyclesBox->count() == 0)
        QWidget::setEnabled(false);
    QSettings settings;
    QString recentCycle = settings.value("recentCycle").toString();
    auto cycleIndex = ui_->cyclesBox->findText(recentCycle);

    // Sets cycle to last used/ most recent if unavailable
    if (instName_ != "")
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
    instrumentsMenu_ = new QMenu("test");
    connect(ui_->instrumentButton, &QPushButton::clicked,
            [=]() { instrumentsMenu_->exec(ui_->instrumentButton->mapToGlobal(QPoint(0, ui_->instrumentButton->height()))); });
    foreach (const auto instrument, instruments)
    {
        auto *action = new QAction(instrument.first, this);
        connect(action, &QAction::triggered, [=]() { changeInst(instrument); });
        instrumentsMenu_->addAction(action);
    }
}

void MainWindow::changeInst(QPair<QString, QString> instrument)
{
    ui_->instrumentButton->setText(instrument.first.toUpper());
    on_instrumentsBox_currentTextChanged(instrument.first);
    instType_ = instrument.second;
    instName_ = instrument.first;
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Update history on close
    QSettings settings;
    settings.setValue("recentInstrument", instName_.toLower());
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
    if (textInput.isEmpty())
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

    QString url_str = "http://127.0.0.1:5000/getAllJournals/" + instName_ + "/" + value + "/" + textInput;
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
    /*
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
    */
    if (event->key() == Qt::Key_G && event->modifiers() == Qt::ControlModifier)
    {
        bool checked = ui_->groupButton->isChecked();
        ui_->groupButton->setChecked(!checked);
        on_groupButton_clicked(!checked);
    }
    if (event->key() == Qt::Key_F3 && event->modifiers() == Qt::ControlModifier)
    {
        on_searchAll_clicked();
        return;
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

std::vector<std::pair<QString, QString>> MainWindow::getFields(QString instrument, QString instType)
{
    std::vector<std::pair<QString, QString>> desiredInstFields;
    QDomNodeList desiredInstrumentFields;

    QFile file("../extra/tableConfig.xml");
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
                // Get column index and title from xml
                column.first = defaultColumns.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
                column.second = defaultColumns.item(i).toElement().attribute("name");
                desiredInstFields.push_back(column);
            }
            return desiredInstFields;
        }
        for (int i = 0; i < configDefaultFields.count(); i++)
        {
            column.first = configDefaultFields.item(i).toElement().elementsByTagName("Data").item(0).toElement().text();
            column.second = configDefaultFields.item(i).toElement().attribute("name");
            desiredInstFields.push_back(column);
        }
        return desiredInstFields;
    }
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

void MainWindow::goTo(HttpRequestWorker *worker, QString runNumber)
{
    setLoadScreen(false);
    QString msg;

    if (worker->error_type == QNetworkReply::NoError)
    {
        if (worker->response == "Not Found")
        {
            statusBar()->showMessage("Run number not found", 5000);
            return;
        }

        if (ui_->cyclesBox->currentText() == worker->response)
        {
            selectIndex(runNumber);
            return;
        }
        connect(this, &MainWindow::tableFilled, [=]() { selectIndex(runNumber); });
        ui_->cyclesBox->setCurrentText(worker->response);
    }
    else
    {
        // an error occurred
        msg = "Error1: " + worker->error_str;
        QMessageBox::information(this, "", msg);
    }
}

void MainWindow::selectIndex(QString runNumber)
{
    ui_->runDataTable->selectionModel()->clearSelection();

    on_searchBox_textChanged(runNumber);
    statusBar()->showMessage("Found run " + runNumber + " in " + ui_->cyclesBox->currentText(), 5000);
    disconnect(this, &MainWindow::tableFilled, nullptr, nullptr);
}

void MainWindow::selectSimilar()
{
    int TitleColumn;
    for (auto i = 0; i < ui_->runDataTable->horizontalHeader()->count(); ++i)
    {
        if (ui_->runDataTable->horizontalHeader()->model()->headerData(i, Qt::Horizontal).toString() == "title")
        {
            TitleColumn = i;
            break;
        }
    }
    QString title = model_->index(ui_->runDataTable->rowAt(pos_.y()), TitleColumn).data().toString();
    for (auto i = 0; i < model_->rowCount(); i++)
    {
        if (model_->index(i, TitleColumn).data().toString() == title)
            ui_->runDataTable->selectionModel()->setCurrentIndex(model_->index(i, TitleColumn),
                                                                 QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
}
void MainWindow::on_actionSearch_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Enter search query"), tr("search runs for:"), QLineEdit::Normal);
    searchString_ = textInput;
    foundIndices_.clear();
    currentFoundIndex_ = 0;
    if (textInput.isEmpty())
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->clearMessage();
        return;
    }
    // Find all occurences of search string in table elements
    for (auto i = 0; i < proxyModel_->columnCount(); ++i)
    {
        auto location = ui_->runDataTable->horizontalHeader()->logicalIndex(i);
        if (ui_->runDataTable->isColumnHidden(location) == false)
            foundIndices_.append(
                proxyModel_->match(proxyModel_->index(0, location), Qt::DisplayRole, textInput, -1, Qt::MatchContains));
    }
    // Select first match
    if (foundIndices_.size() > 0)
    {
        goToCurrentFoundIndex(foundIndices_[0]);
        statusBar()->showMessage("Find \"" + searchString_ + "\": 1/" + QString::number(foundIndices_.size()) + " Results");
    }
    else
    {
        ui_->runDataTable->selectionModel()->clearSelection();
        statusBar()->showMessage("No results");
    }
}

void MainWindow::on_actionSelectNext_triggered() { on_findDown_clicked(); }
void MainWindow::on_actionSelectPrevious_triggered() { on_findUp_clicked(); }
void MainWindow::on_actionSelectAll_triggered() { on_searchAll_clicked(); }

void MainWindow::on_actionRun_Number_triggered()
{
    QString textInput = QInputDialog::getText(this, tr("Enter search query"), tr("Run No: "), QLineEdit::Normal);
    if (textInput.isEmpty())
        return;

    QString url_str = "http://127.0.0.1:5000/getGoToCycle/" + instName_ + "/" + textInput;
    HttpRequestInput input(url_str);
    HttpRequestWorker *worker = new HttpRequestWorker(this);
    connect(worker, &HttpRequestWorker::on_execution_finished,
            [=](HttpRequestWorker *workerProxy) { goTo(workerProxy, textInput); });
    worker->execute(input);
    setLoadScreen(true);
}
