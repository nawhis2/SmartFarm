#ifndef FIREDETECTWIDGET_H
#define FIREDETECTWIDGET_H
#define NOMINMAX
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
#include <QPixmap>
#include <algorithm>


namespace Ui {
class FireDetectWidget;
}

class FireDetectWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit FireDetectWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    Q_INVOKABLE void onSensorDataReceivedWrapper();
    void onSensorDataReceived(const QString& data); //private에서 여기로 옮김
    ~FireDetectWidget();

protected:
    void resizeEvent(QResizeEvent *event) override;


private:
    Ui::FireDetectWidget *ui;
    QChartView *chartView;
    QLineSeries *co2Series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    void setupChart();

    double maxCo2 = 0;
};
extern FireDetectWidget* fireWidget; // 전역 포인터 추가
#endif // FIREDETECTWIDGET_H
