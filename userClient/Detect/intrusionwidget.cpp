#include "intrusionwidget.h"
#include "ui_intrusionwidget.h"
#include "clientUtil.h"
IntrusionWidget::IntrusionWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::IntrusionWidget)
{
    ui->setupUi(this);
    type = "intrusion_detected";
    tableWidget = ui->intrusionEventTable;
    tableWidget->SetDetectStr(type);
    myIndex = 2;
    connect(ui->btnBackFromIntrusion, &QPushButton::clicked, this, &IntrusionWidget::showHomePage);
}

IntrusionWidget::~IntrusionWidget()
{
    delete ui;
}
