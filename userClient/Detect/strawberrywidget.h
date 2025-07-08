#ifndef STRAWBERRYWIDGET_H
#define STRAWBERRYWIDGET_H

#include "detectcorewidget.h"

namespace Ui {
class StrawBerryWidget;
}

class StrawBerryWidget : public DetectCoreWidget
{
    Q_OBJECT

public:
    explicit StrawBerryWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~StrawBerryWidget();

private:
    Ui::StrawBerryWidget *ui;
};

#endif // STRAWBERRYWIDGET_H
