// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

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

HttpRequestWorker::HttpRequestWorker(QObject *parent)
    : QObject(parent), manager(NULL) {
  manager = new QNetworkAccessManager(this);
  connect(manager, SIGNAL(finished(QNetworkReply *)), this,
          SLOT(on_manager_finished(QNetworkReply *)));
}

// Execute request
void HttpRequestWorker::execute(HttpRequestInput *input) {

  // reset variables
  QByteArray request_content = "";
  response = "";
  error_type = QNetworkReply::NoError;
  error_str = "";
  
  // execute connection
  QNetworkRequest request = QNetworkRequest(QUrl(input->url_str));
  request.setRawHeader("User-Agent", "Agent name goes here");
  manager->get(request);
}

// Process request
void HttpRequestWorker::on_manager_finished(QNetworkReply *reply) {
  error_type = reply->error();
  if (error_type == QNetworkReply::NoError) {
    response = reply->readAll();
    jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    json_array = jsonResponse.array();

  } else {
    error_str = reply->errorString();
  }

  reply->deleteLater();

  emit on_execution_finished(this);
}
