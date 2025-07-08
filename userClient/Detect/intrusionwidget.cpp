#include "intrusionwidget.h"
#include "ui_intrusionwidget.h"

IntrusionWidget::IntrusionWidget(QStackedWidget *stack, QWidget *parent)
    : DetectCoreWidget(stack, parent)
    , ui(new Ui::IntrusionWidget)
{
    ui->setupUi(this);

    connect(ui->btnBackFromIntrusion, &QPushButton::clicked, this, &IntrusionWidget::showHomePage);
}

IntrusionWidget::~IntrusionWidget()
{
    delete ui;
}
