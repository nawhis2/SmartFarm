#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <QPixmap>
#include <QDebug>
#include "detectmainwidget.h"
#include "firedetectwidget.h"
#include "intrusionwidget.h"
#include "strawberrywidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QWidget* detectMainWdiget = new DetectMainWidget(ui->stackedWidget, ui->pageMainMenu);
    QWidget* firedWdiget = new FireDetectWidget(ui->stackedWidget, ui->pageFire);
    QWidget* intrusionWdiget = new IntrusionWidget(ui->stackedWidget, ui->pageIntrusion);
    QWidget* strawWdiget = new StrawBerryWidget(ui->stackedWidget, ui->pageGrowth);
}

MainWindow::~MainWindow() {
    delete ui;
}

