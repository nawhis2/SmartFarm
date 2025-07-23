#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include "corestackwidget.h"

namespace Ui {
class LogWidget;
}

class LogWidget : public CoreStackWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QStackedWidget *stack, QWidget *parent = nullptr);
    ~LogWidget();

private:
    Ui::LogWidget *ui;
};

#endif // LOGWIDGET_H
