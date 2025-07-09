#include "strawberrywidget.h"
#include "ui_strawberrywidget.h"

StrawBerryWidget::StrawBerryWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::StrawBerryWidget)
{
    ui->setupUi(this);
    type = "strawberry_detected";
    myIndex = 3;
    connect(ui->btnBackFromGrowth, &QPushButton::clicked, this, &StrawBerryWidget::showHomePage);
}

StrawBerryWidget::~StrawBerryWidget()
{
    delete ui;
}
