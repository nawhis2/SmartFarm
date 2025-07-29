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

class DrawMap;

class StrawBerryWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit StrawBerryWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~StrawBerryWidget();

protected:
    virtual void pageChangedIdx() override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::StrawBerryWidget *ui;
    QMap<QDate, QMap<QString, int>> data;

    // Chart components
    QLineSeries* ripeSeries;
    QDateTimeAxis* axisX;
    QValueAxis* axisY;
    // Pie chart containers
    QChartView *mainPieChartView;
    QChartView* diseasePieChartView = nullptr;
    QChart* diseaseChart = nullptr;
    QPushButton* miniPieButton = nullptr;
    QStackedWidget *pieStack;
    DrawMap *drawMap = nullptr;

    // Setup & update
    void setupLineChart();
    void setupPieCharts();
    void updatePieChartFromTable();
    void updateDiseasePieChart();
    void updateLineChartFromData();
    void updateLineChartFromData(const QString &category = "ripe");

    // UI Animation helpers
    void animateToMiniPie();
    void animateToMainPie();

    // Data appending
    void appendData(const QString &dateStr, const QString &cls, const int &count);
    void showDiseasePieChart();
    void showDiseaseMode(bool enable);

    // Map Drawing
    void setupMapView();
    void requestMapData();

    bool eventFilter(QObject *obj, QEvent *event) override;
    QRect originalPieGeometry; // 👈 축소 전 geometry 저장용
    bool isDiseaseMode = false; // 현재 모드 상태

    void refreshCharts();

private slots:
    void onPieSliceClicked(QPieSlice *slice);
    void onDiseaseSliceClicked(QPieSlice *slice);
};

#endif // STRAWBERRYWIDGET_H
