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
    ui->btnFire->setIconSize(QSize(75, 75));  // 아이콘 크기 조절
    ui->btnFire->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnGrowth,     &QToolButton::clicked, this, &DetectMainWidget::showGrowthPage);
    ui->btnGrowth->setMinimumSize(100, 100);
    ui->btnGrowth->setIcon(QIcon(":/icons/strawberry2.png"));
    ui->btnGrowth->setIconSize(QSize(75, 75));  // 아이콘 크기 조절
    ui->btnGrowth->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnIntrusion,  &QToolButton::clicked, this, &DetectMainWidget::showIntrusionPage);
    ui->btnIntrusion->setIcon(QIcon(":/icons/intrusion.png"));
    ui->btnIntrusion->setIconSize(QSize(75, 75));  // 아d이콘 크기 조절
    ui->btnIntrusion->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnLog,        &QToolButton::clicked, this, &DetectMainWidget::showLogPage);
    ui->btnLog->setIcon(QIcon(":/icons/log.png"));
    ui->btnLog->setIconSize(QSize(75, 75));  // 아이콘 크기 조절
    ui->btnLog->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 아이콘 위, 텍스트 아래

    connect(ui->btnMail, &QToolButton::clicked, this, &DetectMainWidget::showMailDialog);
    //ui->btnMail->setMinimumSize(100, 100); // 다른 버튼과 통일
    ui->btnMail->setIcon(QIcon(":/icons/mail2.png"));
    ui->btnMail->setIconSize(QSize(50, 50));

    connect(ui->btnCamera, &QToolButton::clicked, this, &DetectMainWidget::setupAllStreams);
    ui->btnCamera->setIcon(QIcon(":/icons/camera.png"));
    ui->btnCamera->setIconSize(QSize(50, 50));

    ui->btnSync->setIcon(QIcon(":/icons/car2.png"));
    ui->btnSync->setIconSize(QSize(50, 50));

    weatherManager = new QNetworkAccessManager(this);
    connect(weatherManager, &QNetworkAccessManager::finished, this, &DetectMainWidget::handleWeatherData);

    fetchWeather();  // 날씨 가져오기 요청
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

// 날씨 정보 요청
void DetectMainWidget::fetchWeather()
{
    QString apiKey = "ed464dec9a556ac76d138891022188b5";
    QString city = "Seoul";
    QString urlStr = QString("https://api.openweathermap.org/data/2.5/weather?q=%1&appid=%2&units=metric&lang=kr")
                         .arg(city).arg(apiKey);

    QNetworkReply* reply = weatherManager->get(QNetworkRequest(QUrl(urlStr)));
    connect(reply, &QNetworkReply::finished, this, &DetectMainWidget::handleWeatherData);
}

// 날씨 아이콘 요청
void DetectMainWidget::fetchWeatherIcon(const QString& iconCode)
{
    QString iconUrlStr = QString("https://openweathermap.org/img/wn/%1@2x.png").arg(iconCode);
    QNetworkReply* iconReply = weatherManager->get(QNetworkRequest(QUrl(iconUrlStr)));
    connect(iconReply, &QNetworkReply::finished, this, &DetectMainWidget::handleWeatherIcon);
}

void DetectMainWidget::handleWeatherData()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray response = reply->readAll();
    reply->deleteLater();

    qDebug() << "[날씨 응답 JSON]" << QString::fromUtf8(response);

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(response, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "[JSON 파싱 에러]" << err.errorString();
        ui->label_weather->setText("날씨 정보 오류");
        return;
    }

    QJsonObject obj = doc.object();
    QJsonArray weatherArray = obj["weather"].toArray();
    if (weatherArray.isEmpty()) {
        qDebug() << "[날씨 응답 오류] weather 배열이 비어 있음";
        ui->label_weather->setText("날씨 정보 없음");
        return;
    }

    QJsonObject weatherObj = weatherArray.first().toObject();
    QString description = weatherObj["description"].toString();
    QString iconCode = weatherObj["icon"].toString();
    double temp = obj["main"].toObject()["temp"].toDouble();

    ui->label_weather->setText(QString("%1 / %2°C").arg(description).arg(temp, 0, 'f', 1));

    // ✅ 아이콘 요청 별도 함수로 분리
    fetchWeatherIcon(iconCode);
}

void DetectMainWidget::handleWeatherIcon()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray imageData = reply->readAll();
    reply->deleteLater();

    QPixmap pix;
    if (!pix.loadFromData(imageData)) {
        qDebug() << "[날씨 아이콘 오류] loadFromData 실패";
        return;
    }

    ui->label_weatherIcon->setPixmap(pix.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}




