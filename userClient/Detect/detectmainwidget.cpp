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
    cctvNames.emplace(0, "Fire");
    cctvNames.emplace(1, "Intrusion");
    cctvNames.emplace(2, "Strawberry");
    cctvNames.emplace(3, "cctv");

    videoLabels[0] = ui->videoLabel1;
    videoLabels[1] = ui->videoLabel2;
    videoLabels[2] = ui->videoLabel3;
    videoLabels[3] = ui->videoLabel3;

    connect(ui->btnFire,       &QPushButton::clicked, this, &DetectMainWidget::showFirePage);
    connect(ui->btnGrowth,     &QPushButton::clicked, this, &DetectMainWidget::showGrowthPage);
    connect(ui->btnIntrusion,  &QPushButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    connect(ui->btnMap,     &QPushButton::clicked, this, &DetectMainWidget::showSensorPage);
    connect(ui->startCameraButton, &QPushButton::clicked, this, &DetectMainWidget::setupAllStreams);
}

DetectMainWidget::~DetectMainWidget()
{
    delete ui;
}

void DetectMainWidget::showFirePage()    { changePage(1); }
void DetectMainWidget::showGrowthPage()  { changePage(3); }
void DetectMainWidget::showIntrusionPage(){ changePage(2); }
void DetectMainWidget::showSensorPage()  { /*ui->stackedWidget->setCurrentWidget(ui->pageSensor)*/; }

void DetectMainWidget::setupStream(const char* cctvName, const int index){
    sendFile(cctvName, "IP");
    char buffer[1024];
    int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        QString ip = QString::fromUtf8(buffer);
        if(ip == "NO")
            return;
        qDebug() << "[TEST]" << cctvName <<" ip:" << ip;
        // UI 업데이트는 메인 스레드에서
        QMetaObject::invokeMethod(this, [=]() {
            ipAddress[index] = ip.toStdString();
            QString ipStr = QString::fromStdString(ipAddress[index]);
            QString url = "rtsps://" + ipStr + ":8555/test";
            Streamers[index] = new VideoStreamHandler(index, url, videoLabels[index], this);
            Streamers[index]->start();
        }, Qt::QueuedConnection);
    } else {
        qDebug() << "SSL_read failed or no data.";
    }

}

void DetectMainWidget::setupAllStreams()
{
    qDebug() << "setupAllStreams() called";
    (void)QtConcurrent::run([=]() {
        for(int i=0;i<3;i++){
            if(ipAddress[i] == ""){
                auto it = cctvNames.find(i);
                if (it != cctvNames.end()) {
                    std::string name = it->second;
                    qDebug() << "cctv name: " << name;

                    setupStream(name.c_str(), i);
                }
            }
        }
    });
}

