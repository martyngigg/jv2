// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "mainwindow.h"

#include <QApplication>
#include <future>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setWindowIcon(QIcon(":/icon"));
  MainWindow w;
  w.show();
  return a.exec();
}
