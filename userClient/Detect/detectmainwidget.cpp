#include "detectmainwidget.h"
#include "ui_detectmainwidget.h"
#include <QtConcurrent>
#include "clientUtil.h"
#include "network.h"
#include <QPixmap>
#include <QDebug>
#include "maildialog.h"

DetectMainWidget::DetectMainWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::DetectMainWidget)
{
    ui->setupUi(this);

    // 스트림 정보 설정
    cctvNames.emplace(0, "Fire");
    cctvNames.emplace(1, "Intrusion");
    cctvNames.emplace(2, "Strawberry");
    cctvNames.emplace(3, "Map");

    videoLabels[0] = ui->videoLabel1;
    videoLabels[1] = ui->videoLabel2;
    videoLabels[2] = ui->videoLabel3;
    videoLabels[3] = ui->videoLabel4;

    for (int i = 0; i < 4; ++i)
        videoLabels[i]->setScaledContents(true);

    // 버튼 연결 유지
    connect(ui->btnFire,       &QToolButton::clicked, this, &DetectMainWidget::showFirePage);
    ui->btnFire->setMinimumSize(100, 100);
    ui->btnFire->setIcon(QIcon(":/icons/fire2.png"));
    ui->btnFire->setIconSize(QSize(100, 100));  // 아이콘 크기 조절
    ui->btnFire->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnGrowth,     &QToolButton::clicked, this, &DetectMainWidget::showGrowthPage);
    ui->btnGrowth->setMinimumSize(100, 100);
    ui->btnGrowth->setIcon(QIcon(":/icons/strawberry2.png"));
    ui->btnGrowth->setIconSize(QSize(100, 100));  // 아이콘 크기 조절
    ui->btnGrowth->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnIntrusion,  &QToolButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    ui->btnIntrusion->setIcon(QIcon(":/icons/intrusion.png"));
    ui->btnIntrusion->setIconSize(QSize(100, 100));  // 아이콘 크기 조절
    ui->btnIntrusion->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnLog,        &QToolButton::clicked, this, &DetectMainWidget::showLogPage);
    ui->btnLog->setIcon(QIcon(":/icons/log.png"));
    ui->btnLog->setIconSize(QSize(100, 100));  // 아이콘 크기 조절
    ui->btnLog->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnMail, &QToolButton::clicked, this, &DetectMainWidget::showMailDialog);
    ui->btnMail->setMinimumSize(100, 100); // 다른 버튼과 통일
    ui->btnMail->setIcon(QIcon(":/icons/mail.png"));
    ui->btnMail->setIconSize(QSize(100, 100));
    ui->btnMail->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->startCameraButton, &QToolButton::clicked, this, &DetectMainWidget::setupAllStreams);
}

DetectMainWidget::~DetectMainWidget()
{
    delete ui;
}

void DetectMainWidget::on_btnSync_clicked(){ sendFile("start", "CMD"); }

void DetectMainWidget::showFirePage()     { changePage(1); }
void DetectMainWidget::showGrowthPage()   { changePage(3); }
void DetectMainWidget::showIntrusionPage(){ changePage(2); }
void DetectMainWidget::showLogPage()   { changePage(4); }

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

    for(int i = 0; i < 4; i++){
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

void DetectMainWidget::showMailDialog()
{
    MailDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString email = dialog.getEmail();
        QString password = dialog.getPassword();
        qDebug() << "[MailDialog] Email:" << email << ", Password:" << password;
    }
}
