#include "detectcorewidget.h"
#include "clientUtil.h"
#include "network.h"
#include <QtConcurrent>

DetectCoreWidget::DetectCoreWidget(QStackedWidget *stack,QWidget *parent)
    : QWidget{parent}
    , stackWidget(stack)
    , type("")
    , tableWidget(nullptr)
{
    connect(stackWidget, &QStackedWidget::currentChanged, this, &DetectCoreWidget::onPageChanged);
}

void DetectCoreWidget::showHomePage(){
    changePage(0);
}

void DetectCoreWidget::onPageChanged(int index){
    pageChanged();
}

void DetectCoreWidget::changePage(const int index){
    if(stackWidget)
        stackWidget->setCurrentIndex(index);
}

void DetectCoreWidget::pageChanged(){
    sendFile("intrusion_detected", "DATA");

    (void)QtConcurrent::run([this]() {
        char buffer[1024];
        int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            QString json = QString::fromUtf8(buffer);
            qDebug() << "[TEST] intrusion json:" << json;
        } else {
            qDebug() << "SSL_read failed or no data.";
        }
    });
}
