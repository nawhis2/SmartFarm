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

    // 스트림 정보 설정
    cctvNames.emplace(0, "Fire");
    cctvNames.emplace(1, "Intrusion");
    cctvNames.emplace(2, "Strawberry");
    cctvNames.emplace(3, "cctv");

    videoLabels[0] = ui->videoLabel1;
    videoLabels[1] = ui->videoLabel2;
    videoLabels[2] = ui->videoLabel3;
    videoLabels[3] = ui->videoLabel4;

    // 버튼 연결 유지
    connect(ui->btnFire,       &QPushButton::clicked, this, &DetectMainWidget::showFirePage);
    connect(ui->btnGrowth,     &QPushButton::clicked, this, &DetectMainWidget::showGrowthPage);
    connect(ui->btnIntrusion,  &QPushButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    connect(ui->btnMap,        &QPushButton::clicked, this, &DetectMainWidget::showSensorPage);
    connect(ui->startCameraButton, &QPushButton::clicked, this, &DetectMainWidget::setupAllStreams);
}

DetectMainWidget::~DetectMainWidget()
{
    delete ui;
}

void DetectMainWidget::showFirePage()     { changePage(1); }
void DetectMainWidget::showGrowthPage()   { changePage(3); }
void DetectMainWidget::showIntrusionPage(){ changePage(2); }
void DetectMainWidget::showSensorPage()   {}

void DetectMainWidget::setupStream(const char* cctvName, const int index){
    sendFile(cctvName, "IP");
    char buffer[1024];
    int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        QString ip = QString::fromUtf8(buffer);
        if(ip == "NO") {
            ipAddress[index].clear();
            return;
        }

        qDebug() << "[TEST]" << cctvName <<" ip:" << ip;
        ipAddress[index] = ip.toStdString();
        QString url = "rtsps://" + ip + ":8555/test";

        Streamers[index] = new VideoStreamHandler(index, url);

        connect(Streamers[index], &VideoStreamHandler::frameReady, this, [=](int idx, QImage img) {
            if (idx >= 0 && idx < 4 && videoLabels[idx]) {
                videoLabels[idx]->setPixmap(QPixmap::fromImage(img).scaled(videoLabels[idx]->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }, Qt::QueuedConnection);

        //Streamers[index]->initialize();  // 자동 재연결 포함된 초기화
    } else {
        qDebug() << "SSL_read failed or no data.";
        ipAddress[index].clear();
    }
}

void DetectMainWidget::setupAllStreams()
{
    qDebug() << "setupAllStreams() called";

    for(int i = 0; i < 3; i++){
        QTimer::singleShot(i * 1000, this, [=]() {
            if (ipAddress[i].empty()) {
                auto it = cctvNames.find(i);
                if (it != cctvNames.end()) {
                    setupStream(it->second.c_str(), i);
                }
            }
        });
    }
}
