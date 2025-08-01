#include "logwidget.h"
#include "ui_logwidget.h"
#include <QStackedWidget>

LogWidget::LogWidget(QStackedWidget *stack, QWidget *parent)
    : CoreStackWidget(stack, parent)
    , ui(new Ui::LogWidget)
{
    ui->setupUi(this);

    connect(ui->btnBackFromLog, &QPushButton::clicked, this, &LogWidget::showHomePage);
    ui->LogEventTable->setPageSize(27);
}

LogWidget::~LogWidget()
{
    delete ui;
}
