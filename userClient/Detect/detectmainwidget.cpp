#include "detectmainwidget.h"
#include "ui_detectmainwidget.h"
#include <QtConcurrent>
#include "clientUtil.h"
#include "network.h"
#include <QPixmap>
#include <QDebug>

DetectMainWidget::DetectMainWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::DetectMainWidget)
{
    ui->setupUi(this);
    // UI 버튼
    connect(ui->btnFire,       &QPushButton::clicked, this, &DetectMainWidget::showFirePage);
    connect(ui->btnGrowth,     &QPushButton::clicked, this, &DetectMainWidget::showGrowthPage);
    connect(ui->btnIntrusion,  &QPushButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    connect(ui->btnMap,     &QPushButton::clicked, this, &DetectMainWidget::showSensorPage);
    connect(ui->startCameraButton, &QPushButton::clicked, this, &DetectMainWidget::setupAllStreams);
}

DetectMainWidget::~DetectMainWidget()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    delete ui;
}

void DetectMainWidget::showFirePage()    { changePage(1); }
void DetectMainWidget::showGrowthPage()  { changePage(3); }
void DetectMainWidget::showIntrusionPage(){ changePage(2);
    /*sendFile("Intrusion\n", "IP");
    //sendFile("Strawberry\n", "IP");
    (void)QtConcurrent::run([this]() {
        char buffer[1024];
        int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            QString ip = QString::fromUtf8(buffer);
            qDebug() << "[TEST] intrusion ip:" << ip;

            // UI 업데이트는 메인 스레드에서
            QMetaObject::invokeMethod(this, [this, ip]() {
                ipAddress = ip.toStdString();
                // 필요 시 라벨 갱신 등 UI 처리
            }, Qt::QueuedConnection);
        } else {
            qDebug() << "SSL_read failed or no data.";
        }
    });*/
}
void DetectMainWidget::showSensorPage()  { /*ui->stackedWidget->setCurrentWidget(ui->pageSensor)*/; }


void DetectMainWidget::setupAllStreams()
{
    qDebug() << "setupAllStreams() called";
    fireStream = new VideoStreamHandler("rtsp://192.168.0.46:8554/test", ui->videoLabel1, this);
    growthStream = new VideoStreamHandler("rtsps://192.168.0.119:8555/test", ui->videoLabel2, this);
    intrusionStream = new VideoStreamHandler("rtsp://192.168.0.81:8554/test", ui->videoLabel3, this);
    strawberryStream = new VideoStreamHandler("rtsp://<ip4>:8554/strawberry", ui->videoLabel4, this);

    fireStream->start();
    growthStream->start();
    intrusionStream->start();
    strawberryStream->start();
}

