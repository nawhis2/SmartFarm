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
                    qDebug() << "[Hook] addIntrusionEvent called at" << ts;
                    addIntrusionEvent(ts);
                } else {
                    qDebug() << "[Hook] Invalid QDateTime:" << fields[0];
                }
            } else {
                qDebug() << "[Hook] Not enough fields:" << fields;
            }
        };

    }

    // 기본 날짜: 오늘
    setDateAndUpdate(QDate::currentDate());
    ui->dateEdit->setDate(QDate::currentDate());
}

IntrusionWidget::~IntrusionWidget()
{
    delete ui;
}
void IntrusionWidget::setupBarChart()
{
    barSet = new QBarSet("Intrusions");
    for (int i = 0; i < 24; ++i) {
        *barSet << 0;
    }

    barSeries = new QBarSeries();
    barSeries->append(barSet);
    barSeries->setBarWidth(0.8);

    barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("시간대별 침입 이벤트");
    barChart->setBackgroundBrush(Qt::transparent);
    barChart->legend()->hide();

    // X축 (0시 ~ 23시)
    axisX = new QCategoryAxis();
    for (int i = 0; i < 24; i += 2) {
        axisX->append(QString("%1시").arg(i), i);
    }
    axisX->setRange(0, 23);
    axisX->setLabelsColor(QColor("#aef3c0"));
    axisX->setTitleBrush(QBrush(QColor("#aef3c0")));
    axisX->setTitleText("Hour");
    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    // Y축
    axisY = new QValueAxis();
    axisY->setRange(0, 5); // 초기 최대값
    axisY->setTitleText("Count");
    axisY->setLabelsColor(QColor("#aef3c0"));
    axisY->setTitleBrush(QBrush(QColor("#aef3c0")));
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    // ChartView에 적용
    ui->chartView->setChart(barChart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartView->setStyleSheet("background-color: transparent; border: none;");
}

void IntrusionWidget::addIntrusionEvent(const QDateTime& timestamp)
{
    intrusionTimestamps.append(timestamp);

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

    ui->chartView->update();
}

void IntrusionWidget::setupBarInteraction()
{
    connect(barSet, &QBarSet::clicked, this, [=](int index) {
        // 모든 막대를 기본 색으로 초기화
        barSet->setColor(QColor("#1de9b6")); // 기본색

        // 클릭된 막대 색 변경
        //barSet->setColor(index, QColor("#ff4081")); // 선택된 색
    });
}
