#include "detectcorewidget.h"
#include "clientUtil.h"
#include "network.h"
#include <QtConcurrent>
#include <QHeaderView>
#include "imagepushbutton.h"

DetectCoreWidget::DetectCoreWidget(QStackedWidget *stack,QWidget *parent)
    : QWidget{parent}
    , stackWidget(stack)
    , type("")
    , tableWidget(nullptr)
    , myIndex(-1)
{
    connect(stackWidget, &QStackedWidget::currentChanged, this, &DetectCoreWidget::onPageChanged);
}

void DetectCoreWidget::showHomePage(){
    changePage(0);
}

void DetectCoreWidget::onPageChanged(int index){
    if(index){
        if (myIndex == index) {
            pageChanged(index);
        }
    }
}

void DetectCoreWidget::changePage(const int index){
    if(stackWidget)
        stackWidget->setCurrentIndex(index);
}

void DetectCoreWidget::pageChanged(const int index){
    if(index == 1)
        sendFile("fire_detected", "DATA");

    else if(index == 2)
        sendFile("intrusion_detected", "DATA");

    else if(index == 3)
        sendFile("strawberry_detected", "DATA");

    (void)QtConcurrent::run([this]() {
        while(1){
            char buffer[1024];
            int n = SSL_read(sock_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';

                if(strncmp(buffer, "END", 3) == 0){
                    break;
                }

                QString json = QString::fromUtf8(buffer);
                qDebug() << "[TEST] intrusion json:" << json;

                QStringList fields = json.trimmed().split('|', Qt::SkipEmptyParts);
                int row = tableWidget->rowCount();
                if (tableWidget->columnCount() < fields.size()) {
                    tableWidget->setColumnCount(fields.size());
                }
                tableWidget->insertRow(row);

                QMetaObject::invokeMethod(this, [=]() {
                    ImagePushButton *imgBtn = new ImagePushButton();
                    imgBtn->setImgUrl(fields[0]);
                    tableWidget->setCellWidget(row, 0, imgBtn);
                }, Qt::QueuedConnection);

                for (int col = 1; col < fields.size(); ++col) {
                    qDebug()<<fields[col]<<"\n";
                    tableWidget->setItem(row, col, new QTableWidgetItem(fields[col]));
                }
                tableWidget->resizeColumnsToContents();
                tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

            } else {
                qDebug() << "SSL_read failed or no data.";
                break;
            }
        }
    });
}
