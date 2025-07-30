#include "intrusionwidget.h"
#include "ui_intrusionwidget.h"
#include "clientUtil.h"
#include <QDateTime>
#include <QDebug>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QColor>
#include <QTimer>

IntrusionWidget::IntrusionWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::IntrusionWidget)
    , m_stack(stack)
{
    ui->setupUi(this);
    setupBarChart();
    setupBarInteraction();

    type = "intrusion_detected";
    tableWidget = ui->intrusionEventTable;
    tableWidget->SetDetectStr(type);
    tableWidget->setPageSize(10);
    myIndex = 2;

    connect(ui->btnBackFromIntrusion, &QPushButton::clicked, this, &IntrusionWidget::showHomePage);
    connect(ui->dateEdit, &QDateEdit::dateChanged, this, &IntrusionWidget::setDateAndUpdate);


    CustomTableWidget* tbl = dynamic_cast<CustomTableWidget*>(ui->intrusionEventTable);
    if (tbl) {
        tbl->onNewDataHook = [this](const QStringList& fields) {
            if (fields.size() >= 2) {
                QDateTime ts = QDateTime::fromString(fields[0], "yyyy-MM-dd HH:mm:ss");
                if (ts.isValid()) {
                    qDebug() << "[Hook] Queueing addIntrusionEvent at" << ts;

                    QMetaObject::invokeMethod(this, [=]() {
                        addIntrusionEvent(ts);
                    }, Qt::QueuedConnection);  // <-- ⭐ 여기!
                } else {
                    qDebug() << "[Hook] Invalid QDateTime:" << fields[0];
                }
            } else {
                qDebug() << "[Hook] Not enough fields:" << fields;
            }
        };

    }

    QTimer::singleShot(0, this, [=]() {
        ui->dateEdit->setDate(QDate::currentDate());
    });
}

IntrusionWidget::~IntrusionWidget()
{
    delete ui;
}

void IntrusionWidget::setupBarChart()
{
    // 1. BarSet 생성
    barSet = new QBarSet("Intrusions");
    for (int i = 0; i < 24; ++i) {
        *barSet << 0;
    }

    // 2. BarSeries 설정
    barSeries = new QBarSeries();
    barSeries->append(barSet);
    barSeries->setBarWidth(0.9);  // 바 너비 늘려서 맞추기

    // 3. Chart 설정
    barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("시간대별 침입 이벤트");
    barChart->setBackgroundBrush(Qt::transparent);
    barChart->legend()->hide();

    // 4. X축: QCategoryAxis (2시간 간격 라벨)
    axisX = new QCategoryAxis();  // 꼭 헤더에 QCategoryAxis* axisX 로 선언

    for (int i = 0; i < 24; i += 2) {
        axisX->append(QString("%1시").arg(i), i);
    }
    axisX->setRange(0, 23);
    axisX->setLabelsColor(QColor("#aef3c0"));
    axisX->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisX->setTitleText("Hour");

    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    // 5. Y축: 값 범위 자동 조절
    axisY = new QValueAxis();
    axisY->setRange(0, 5);
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Count");
    axisY->setLabelsColor(QColor("#aef3c0"));
    axisY->setTitleBrush(QBrush(QColor("#aef3c0")));
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    // 6. ChartView 적용
    ui->chartView->setChart(barChart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartView->setStyleSheet("background-color: transparent; border: none;");
}



void IntrusionWidget::addIntrusionEvent(const QDateTime& timestamp)
{
    if (!intrusionTimestamps.contains(timestamp)) {  // ✅ 중복 방지
        intrusionTimestamps.append(timestamp);
    }
    // 현재 선택된 날짜와 같으면 차트 갱신
    if (timestamp.date() == selectedDate) {
        setDateAndUpdate(selectedDate);
    }
}

void IntrusionWidget::setDateAndUpdate(const QDate& date)
{
    selectedDate = date;

    // 0~23시 count 초기화
    QVector<int> hourlyCount(24, 0);

    for (const QDateTime& ts : intrusionTimestamps) {
        if (ts.date() == date) {
            int hour = ts.time().hour();
            if (hour >= 0 && hour < 24)
                hourlyCount[hour]++;
        }
    }

    // barSet 갱신
    barSet->remove(0, barSet->count());
    for (int c : hourlyCount)
        *barSet << c;

    // 최대값에 따라 Y축 조정
    int maxCount = *std::max_element(hourlyCount.begin(), hourlyCount.end());
    axisY->setRange(0, std::max(5, maxCount + 1));
    axisY->setTickCount(std::max(6, maxCount + 2));
    axisY->applyNiceNumbers();
    axisY->setLabelFormat("%d");

    ui->chartView->update();
}

void IntrusionWidget::setupBarInteraction()
{
    connect(barSet, &QBarSet::clicked, this, [=](int index) {
        tableWidget->setDateHour(makeTimeRangeString(index));
    });
}

std::string IntrusionWidget::makeTimeRangeString(int index) {
    // 1) dateEdit에서 날짜를 꺼냅니다.
    QDate date = ui->dateEdit->date();

    // 2) 시작 시각 계산 (index 시 0분 0초)
    QDateTime start(date, QTime(index, 0, 0));

    // 3) 1시간 뒤 계산
    QDateTime end = start.addSecs(3600);

    // 4) 문자열 포맷
    const QString fmt = "yyyy-MM-dd HH:mm:ss";
    QString s = start.toString(fmt);
    QString e = end  .toString(fmt);

    // 5) std::string으로 변환해 반환
    return (s + "|" + e).toStdString();
}
