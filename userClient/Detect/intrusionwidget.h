#ifndef INTRUSIONWIDGET_H
#define INTRUSIONWIDGET_H

#include "detectcorewidget.h"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QPieSeries>
#include <QtCharts/QDateTimeAxis>

namespace Ui {
class IntrusionWidget;
}

class IntrusionWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit IntrusionWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~IntrusionWidget();

private:
    Ui::IntrusionWidget *ui;
};

#endif // INTRUSIONWIDGET_H
