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
    setupLineChart();
    connect(ui->btnBackFromGrowth, &QPushButton::clicked, this, &StrawBerryWidget::showHomePage);
}

StrawBerryWidget::~StrawBerryWidget()
{
    delete ui;
}

void StrawBerryWidget::appendData(const QString &newDateStr,
                                  const QString &newCountsStr,
                                  const int &newCountsInt) {
    qDebug() << "[appendData] date =" << newDateStr
             << ", class =" << newCountsStr
             << ", count =" << newCountsInt;

    // 1) 문자열을 QDate로 변환
    QDate newDate = QDate::fromString(newDateStr, "yyyy-MM-dd");
    if (!newDate.isValid()) return;

    // 2) 삽입 (기존 같은 날짜가 있으면 덮어쓰기)
    data[newDate][newCountsStr] = newCountsInt;

    // 3) 사이즈가 7개 초과하면 맨 앞(가장 오래된) 제거
    while (data.size() > 7) {
        data.erase(data.begin());
    }

    // 4) 차트 갱신
    QMetaObject::invokeMethod(this, [=]() {
        updatePieChartFromTable();
        updateLineChartFromData();
    }, Qt::QueuedConnection);
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
void StrawBerryWidget::setupLineChart()
{
    rapeSeries = new QLineSeries();
    rapeSeries->setName("Ripe Count");

    QPen pen(QColor("#1de9b6")); // 밝은 민트색 강조
    pen.setWidthF(2.0);
    rapeSeries->setPen(pen);

    // 1. Chart 설정
    QChart *chart = new QChart();
    chart->addSeries(rapeSeries);
    chart->setTitle("Ripe Strawberry Count");
    chart->setTitleFont(QFont("Segoe UI", 20, QFont::Bold));
    chart->setTitleBrush(QBrush(QColor("#aef3c0")));
    chart->setBackgroundBrush(Qt::transparent);  // 투명
    chart->legend()->hide();

    // 2. X축 (날짜 기반)
    axisX = new QCategoryAxis();  // 날짜 문자열 표현을 위해 QCategoryAxis 사용
    axisX->setTitleText("Date");
    axisX->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisX->setLabelsColor(QColor("#aef3c0"));
    axisX->setGridLineColor(QColor("#225544"));
    chart->addAxis(axisX, Qt::AlignBottom);
    rapeSeries->attachAxis(axisX);

    // 3. Y축 (개수)
    axisY = new QValueAxis;
    axisY->setTitleText("Count");
    axisY->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisY->setLabelsColor(QColor("#aef3c0"));
    axisY->setGridLineColor(QColor("#225544"));
    axisY->setRange(0, 30);
    chart->addAxis(axisY, Qt::AlignLeft);
    rapeSeries->attachAxis(axisY);

    // 4. ChartView 스타일
    ui->lineChartView->setChart(chart);
    ui->lineChartView->setRenderHint(QPainter::Antialiasing);
    ui->lineChartView->setStyleSheet("background-color: transparent; border: none;");
}


void StrawBerryWidget::updateLineChartFromData()
{
    if (!rapeSeries || !axisX || !axisY) return;

    rapeSeries->clear();
    QChart* chart = ui->lineChartView->chart();
    chart->removeAxis(axisX);
    axisX = new QCategoryAxis();
    axisX->setTitleText("Date");
    axisX->setLabelsColor(Qt::white);
    chart->addAxis(axisX, Qt::AlignBottom);
    rapeSeries->attachAxis(axisX);


    QList<QDate> dates = data.keys();
    std::sort(dates.begin(), dates.end());

    int x = 0;
    int maxVal = 1;

    for (const QDate& date : dates) {
        int count = data[date].value("rape", 0);
        rapeSeries->append(x, count);
        axisX->append(date.toString("MM-dd"), x);
        x++;
        maxVal = std::max(maxVal, count);
    }

    axisY->setRange(0, maxVal + 2);
}

void StrawBerryWidget::updatePieChartFromTable()
{
    QPieSeries* series = new QPieSeries();

    // 1. eventName 개수 세기 (모든 날짜에 대해 합산)
    QMap<QString, int> eventCounts;

    for (auto outerIt = data.begin(); outerIt != data.end(); ++outerIt) {
        const QMap<QString, int>& dailyMap = outerIt.value();
        for (auto innerIt = dailyMap.begin(); innerIt != dailyMap.end(); ++innerIt) {
            eventCounts[innerIt.key()] += innerIt.value();
        }
    }

    qDebug() << "[Chart] eventCounts from data:" << eventCounts;

    // 2. 파이 조각 추가
    for (auto it = eventCounts.begin(); it != eventCounts.end(); ++it) {
        series->append(it.key(), it.value());
    }

    // 3. 차트 구성
    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Strawberry Status");
    chart->setBackgroundVisible(false);
    chart->setBackgroundBrush(Qt::NoBrush);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->setVisible(true);  // 범례

    series->setLabelsVisible(false);     // 파이 조각 라벨 제거
    for (QPieSlice *slice : series->slices()) {
        slice->setPen(Qt::NoPen);        // 파이 조각 테두리 제거
    }

    ui->pieChartView->setChart(chart);
    ui->pieChartView->setRenderHint(QPainter::Antialiasing);
}



