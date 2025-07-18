#ifndef STRAWBERRYWIDGET_H
#define STRAWBERRYWIDGET_H

#include "detectcorewidget.h"
#include <map>
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
    std::map<QDate, std::map<QString,int>> data;

private:
    void appendData(const QString &newDateStr,
                    const QString &newCountsStr,
                    const int &newCountsInt);
};

#endif // STRAWBERRYWIDGET_H
