// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 E. Devlin and T. Youngs

#include "mainwindow.h"

#include <QApplication>
#include <future>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setWindowIcon(QIcon(":/icon"));
    MainWindow window;
    window.show();
    return application.exec();
}
