#include "firedetectwidget.h"
#include "ui_firedetectwidget.h"

FireDetectWidget::FireDetectWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::FireDetectWidget)
{
    ui->setupUi(this);
    type = "fire_detected";
    tableWidget = ui->fireEventTable;
    tableWidget->SetDetectStr(type);
    myIndex = 1;
    setupChart();
    simulateData();
    connect(ui->btnBackFromFire, &QPushButton::clicked, this, &FireDetectWidget::showHomePage);
}
void FireDetectWidget::setupChart()
{
    // 1. CO₂ 선 그래프 스타일
    co2Series = new QLineSeries();
    co2Series->setName("CO₂ 농도 (ppm)");
    QPen pen(QColor("#1de9b6")); // 밝은 민트색 강조
    pen.setWidthF(2.0);
    co2Series->setPen(pen);

    // 2. Chart 설정
    QChart *chart = new QChart();
    chart->addSeries(co2Series);
    chart->setTitle("CO₂ LEVELS");
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    chart->setTitleBrush(QBrush(QColor("#aef3c0")));
    chart->setBackgroundBrush(Qt::transparent);  // 투명
    chart->legend()->hide();

    // 3. X축 (시간)
    axisX = new QDateTimeAxis;
    axisX->setFormat("HH:mm");
    axisX->setTitleText("Time");
    axisX->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisX->setLabelsColor(QColor("#aef3c0"));
    axisX->setGridLineColor(QColor("#225544"));  // 테이블과 유사
    chart->addAxis(axisX, Qt::AlignBottom);
    co2Series->attachAxis(axisX);

    // 4. Y축 (농도)
    axisY = new QValueAxis;
    axisY->setRange(300, 1000);
    axisY->setTitleText("농도 (ppm)");
    axisY->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisY->setLabelsColor(QColor("#aef3c0"));
    axisY->setGridLineColor(QColor("#225544"));
    chart->addAxis(axisY, Qt::AlignLeft);
    co2Series->attachAxis(axisY);

    // 5. ChartView 스타일
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartView->setStyleSheet("background-color: transparent; border: none;");
}



void FireDetectWidget::simulateData() {
    // 예시 하드코딩된 CO₂ 값
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < 10; ++i) {
        qreal value = 400 + QRandomGenerator::global()->bounded(500);  // 0~99
        co2Series->append(now.addSecs(i * 60).toMSecsSinceEpoch(), value);
    }
    axisX->setRange(now, now.addSecs(600));
}

FireDetectWidget::~FireDetectWidget()
{
    delete ui;
}
