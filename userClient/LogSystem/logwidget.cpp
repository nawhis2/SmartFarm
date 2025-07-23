#include "logwidget.h"
#include "ui_logwidget.h"
#include <QStackedWidget>

LogWidget::LogWidget(QStackedWidget *stack, QWidget *parent)
    : CoreStackWidget(stack, parent)
    , ui(new Ui::LogWidget)
{
    ui->setupUi(this);

    connect(ui->btnBackFromLog, &QPushButton::clicked, this, &LogWidget::showHomePage);
}

LogWidget::~LogWidget()
{
    delete ui;
}
