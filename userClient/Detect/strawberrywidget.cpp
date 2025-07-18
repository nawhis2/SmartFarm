#include "strawberrywidget.h"
#include "ui_strawberrywidget.h"
#include <QDate>
#include "clientUtil.h"
#include "network.h"

StrawBerryWidget::StrawBerryWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::StrawBerryWidget)
{
    ui->setupUi(this);
    type = "strawberry_detected";
    tableWidget = ui->strawEventTable;
    tableWidget->SetDetectStr(type);
    myIndex = 3;
    connect(ui->btnBackFromGrowth, &QPushButton::clicked, this, &StrawBerryWidget::showHomePage);
}

StrawBerryWidget::~StrawBerryWidget()
{
    delete ui;
}

void StrawBerryWidget::appendData(const QString &newDateStr,
                                  const QString &newCountsStr,
                                  const int &newCountsInt){
    qDebug() << "[appendData] date =" << newDateStr
             << ", class =" << newCountsStr
             << ", count =" << newCountsInt;

    // 1) 문자열을 QDate로 변환
    QDate newDate = QDate::fromString(newDateStr, "yyyy-MM-dd");
    if (!newDate.isValid()) return;


    // 1. 삽입 (기존 같은 날짜가 있으면 덮어쓰기)
    data[newDate][newCountsStr] = newCountsInt;

    // 2. 사이즈가 7개 초과하면 맨 앞(가장 오래된) 제거
    while (data.size() > 7) {
        data.erase(data.begin());
    }

    // 3. 차트 갱신

}

void StrawBerryWidget::pageChangedIdx(){
    DetectCoreWidget::pageChangedIdx();

    sendFile(type.c_str(), "WEEKLY");

    while (1) {
        char buffer[1024];
        int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';

            if (strncmp(buffer, "END", 3) == 0) break;

            QString json = QString::fromUtf8(buffer);
            qDebug() << "[TEST] intrusion json:" << json;

            QStringList fields = json.trimmed().split('|', Qt::SkipEmptyParts);

            appendData(fields[0], fields[1], fields[2].toInt());
        } else {
            qDebug() << "WeeklyData failed or no data.";
            break;
        }
    }
}
