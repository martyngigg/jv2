// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

#include "httprequestworker.h"
#include <QBuffer>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

// Object for request URL
HttpRequestInput::HttpRequestInput(QString v_url_str) { url_str = v_url_str; }

HttpRequestWorker::HttpRequestWorker(QObject *parent) : QObject(parent), manager_(NULL)
{
    manager_ = new QNetworkAccessManager(this);
    connect(manager_, SIGNAL(finished(QNetworkReply *)), this, SLOT(on_manager_finished(QNetworkReply *)));
}

// Execute request
void HttpRequestWorker::execute(HttpRequestInput input)
{

    // reset variables
    response = "";
    errorType = QNetworkReply::NoError;
    errorString = "";

    // execute connection
    QNetworkRequest request = QNetworkRequest(QUrl(input.url_str));
    request.setRawHeader("User-Agent", "Agent name goes here");
    manager_->get(request);
}

// Process request
void HttpRequestWorker::on_manager_finished(QNetworkReply *reply)
{
    errorType = reply->error();
    if (errorType == QNetworkReply::NoError)
    {
        response = reply->readAll();
        jsonResponse = QJsonDocument::fromJson(response.toUtf8());
        jsonArray = jsonResponse.array();
    }
    else
        errorString = reply->errorString();

    reply->deleteLater();

    emit on_execution_finished(this);
}
