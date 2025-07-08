#ifndef FIREDETECTWIDGET_H
#define FIREDETECTWIDGET_H

#include "detectcorewidget.h"
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
};

#endif // FIREDETECTWIDGET_H
