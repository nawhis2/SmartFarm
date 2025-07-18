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
    // 🔁 랜덤 센서 시뮬레이션 (3초 간격)
    QTimer* simTimer = new QTimer(this);
    connect(simTimer, &QTimer::timeout, this, [=]() {
        double co2 = 400 + QRandomGenerator::global()->bounded(400);   // 400~800 ppm
        double temp = 20 + QRandomGenerator::global()->bounded(10);    // 20~30℃
        QString data = QString("%1 %2").arg(co2).arg(temp);
        onSensorDataReceived(data);
    });
    simTimer->start(3000);  // 3초마다
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
void FireDetectWidget::onSensorDataReceived(const QString& data) {
    // 예: "738 23.4"
    QStringList parts = data.split(" ");
    if (parts.size() >= 2) {
        double co2 = parts[0].toDouble();
        double temp = parts[1].toDouble();

        // 1. QLabel에 실시간 표시
        ui->label_co2->setText(
            QString("<span style='font-size:32pt; font-weight:bold;'>%1</span><br>"
                    "<span style='font-size:12pt;'>ppm</span>").arg((int)co2));
        ui->label_temp->setText(
            QString("<span style='font-size:32pt; font-weight:bold;'>%1</span><br>"
                    "<span style='font-size:12pt;'>℃</span>").arg(temp, 0, 'f', 1));

        // 2. 그래프에 추가
        QDateTime now = QDateTime::currentDateTime();
        co2Series->append(now.toMSecsSinceEpoch(), co2);

        // X축 시간 범위 업데이트
        axisX->setRange(now.addSecs(-600), now);  // 최근 10분만 보기
    }
}



FireDetectWidget::~FireDetectWidget()
{
    delete ui;
}
