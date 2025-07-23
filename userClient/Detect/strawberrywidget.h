#ifndef STRAWBERRYWIDGET_H
#define STRAWBERRYWIDGET_H
#define NOMINMAX

#include "detectcorewidget.h"
#include <map>
#include <QtCharts>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QTableWidget>
#include <algorithm>

namespace Ui {
class StrawBerryWidget;
}

class StrawBerryWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit StrawBerryWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~StrawBerryWidget();

protected:
    virtual void pageChangedIdx() override;

private:
    Ui::StrawBerryWidget *ui;
    QMap<QDate, QMap<QString, int>> data;
    QLineSeries* rapeSeries;
    QCategoryAxis* axisX;
    QValueAxis* axisY;


private:
    void appendData(const QString &newDateStr,
                    const QString &newCountsStr,
                    const int &newCountsInt);
    void updatePieChartFromTable();
    void setupLineChart();
    void updateLineChartFromData();
};

#endif // STRAWBERRYWIDGET_H
