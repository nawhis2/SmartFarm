#include "firedetectwidget.h"
#include "ui_firedetectwidget.h"

FireDetectWidget::FireDetectWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::FireDetectWidget)
{
    ui->setupUi(this);

    connect(ui->btnBackFromFire, &QPushButton::clicked, this, &FireDetectWidget::showHomePage);
}

FireDetectWidget::~FireDetectWidget()
{
    delete ui;
}
