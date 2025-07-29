#ifndef INTRUSIONWIDGET_H
#define INTRUSIONWIDGET_H

#include "detectcorewidget.h"
#include <QWidget>
#include <QStackedWidget>
#include <QDate>
#include <QDateTime>
#include <QList>

// Qt Charts 관련
#include <QtCharts/QChartView>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QChart>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QValueAxis>

namespace Ui {
class IntrusionWidget;
}

class IntrusionWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit IntrusionWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~IntrusionWidget();
    void addIntrusionEvent(const QDateTime& timestamp);
    void setDateAndUpdate(const QDate& date);

private:
    Ui::IntrusionWidget *ui;

    // 막대 차트 관련
    QBarSet* barSet;
    QBarSeries* barSeries;
    QChart* barChart;
    QCategoryAxis* axisX;
    QValueAxis* axisY;

    QDate selectedDate;
    QList<QDateTime> intrusionTimestamps;

    QStackedWidget* m_stack;

    QTime getStartTime() const;
    QTime getEndTime() const;

    void setupBarChart();
    void setupBarInteraction();
};

#endif // INTRUSIONWIDGET_H
