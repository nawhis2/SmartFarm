#ifndef FIREDETECTWIDGET_H
#define FIREDETECTWIDGET_H
#include "detectcorewidget.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QtGlobal>
#include <QTimer>
#include <QRandomGenerator>

namespace Ui {
class FireDetectWidget;
}

class FireDetectWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit FireDetectWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~FireDetectWidget();

private:
    Ui::FireDetectWidget *ui;
    QChartView *chartView;
    QLineSeries *co2Series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    void setupChart();
    void onSensorDataReceived(const QString& data);
};

#endif // FIREDETECTWIDGET_H
