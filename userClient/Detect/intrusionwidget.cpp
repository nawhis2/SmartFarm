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
    // 1. BarSet 생성 (색상/테두리 커스텀)
    barSet = new QBarSet("Intrusions");
    for (int i = 0; i < 24; ++i) {
        *barSet << 0;
    }
    barSet->setColor(QColor("#33aa88"));  // 메인 민트 컬러
    barSet->setBorderColor(QColor("#1de9b6")); // 엣지 민트
    barSet->setLabelBrush(QColor("#aef3c0")); // 값 표시 색
    barSet->setLabelFont(QFont("Segoe UI", 10, QFont::Bold));

    // 2. BarSeries 설정
    barSeries = new QBarSeries();
    barSeries->append(barSet);
    barSeries->setBarWidth(0.9);
    barSeries->setLabelsVisible(true);  // 막대 위 숫자 표시

    // 3. Chart 설정
    barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("Hourly Intrusion Events");
    barChart->setTitleFont(QFont("Segoe UI", 13));
    barChart->setTitleBrush(QBrush(QColor("#aef3c0")));
    barChart->setBackgroundVisible(false);
    barChart->setBackgroundBrush(Qt::NoBrush);
    barChart->setPlotAreaBackgroundVisible(false);
    barChart->legend()->hide();

    // 4. X축: QCategoryAxis (2시간 간격 라벨)
    axisX = new QCategoryAxis();
    for (int i = 0; i < 24; i += 2) {
        axisX->append(QString("%1시").arg(i), i);
    }
    axisX->setRange(0, 23);
    axisX->setLabelsColor(QColor("#aef3c0"));
    axisX->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisX->setTitleText("Hour");
    axisX->setTitleFont(QFont("Segoe UI", 11));
    axisX->setLabelsFont(QFont("Segoe UI", 9));

    QPen gridPen(QColor("#406060"));
    gridPen.setStyle(Qt::DashLine);
    axisX->setGridLinePen(gridPen);

    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    // 5. Y축
    axisY = new QValueAxis();
    axisY->setRange(0, 5);
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Count");
    axisY->setLabelsColor(QColor("#aef3c0"));
    axisY->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisY->setTitleFont(QFont("Segoe UI", 11));
    axisY->setLabelsFont(QFont("Segoe UI", 9));
    axisY->setGridLinePen(gridPen);

    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    // 6. ChartView
    ui->chartView->setChart(barChart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
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

