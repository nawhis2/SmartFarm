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
#include "logWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    qDebug() << "🟢 ui setup 시작";
    ui->setupUi(this);
    qDebug() << "✅ ui setup 완료";

    qDebug() << "🟢 DetectMainWidget 생성";
    DetectMainWidget* detectMainWidget = new DetectMainWidget(ui->stackedWidget);
    qDebug() << "✅ DetectMainWidget 생성됨";

    QVBoxLayout* mainMenuLayout = new QVBoxLayout();
    mainMenuLayout->addWidget(detectMainWidget);
    ui->pageMainMenu->setLayout(mainMenuLayout);
    qDebug() << "✅ mainMenu layout 연결 완료";

    FireDetectWidget* fireWidget = new FireDetectWidget(ui->stackedWidget);
    QVBoxLayout* fireLayout = new QVBoxLayout();
    fireLayout->addWidget(fireWidget);
    ui->pageFire->setLayout(fireLayout);
    qDebug() << "✅ fire 생성 완료";

    IntrusionWidget* intrusionWidget = new IntrusionWidget(ui->stackedWidget);
    QVBoxLayout* intrusionLayout = new QVBoxLayout();
    intrusionLayout->addWidget(intrusionWidget);
    ui->pageIntrusion->setLayout(intrusionLayout);
    qDebug() << "✅ intrusion 생성 완료";

    StrawBerryWidget* strawWidget = new StrawBerryWidget(ui->stackedWidget);
    QVBoxLayout* growthLayout = new QVBoxLayout();
    growthLayout->addWidget(strawWidget);
    ui->pageGrowth->setLayout(growthLayout);
    qDebug() << "✅ growth 생성 완료";

    LogWidget* logWidget = new LogWidget(ui->stackedWidget);
    QVBoxLayout* logLayout = new QVBoxLayout();
    logLayout->addWidget(logWidget);
    ui->pageLog->setLayout(logLayout);
    qDebug() << "✅ log 생성 완료";
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    }
}
MainWindow::~MainWindow() {
    delete ui;
}

