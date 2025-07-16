/********************************************************************************
** Form generated from reading UI file 'strawberrywidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_STRAWBERRYWIDGET_H
#define UI_STRAWBERRYWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_StrawBerryWidget
{
public:
    QPushButton *btnBackFromGrowth;
    QTableWidget *strawEventTable;

    void setupUi(QWidget *StrawBerryWidget)
    {
        if (StrawBerryWidget->objectName().isEmpty())
            StrawBerryWidget->setObjectName("StrawBerryWidget");
        StrawBerryWidget->resize(968, 600);
        btnBackFromGrowth = new QPushButton(StrawBerryWidget);
        btnBackFromGrowth->setObjectName("btnBackFromGrowth");
        btnBackFromGrowth->setGeometry(QRect(850, 540, 95, 29));
        strawEventTable = new QTableWidget(StrawBerryWidget);
        if (strawEventTable->columnCount() < 4)
            strawEventTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        strawEventTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        strawEventTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        strawEventTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        strawEventTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        strawEventTable->setObjectName("strawEventTable");
        strawEventTable->setGeometry(QRect(40, 40, 901, 491));
        QFont font;
        font.setPointSize(20);
        strawEventTable->setFont(font);
        strawEventTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        strawEventTable->verticalHeader()->setVisible(false);

        retranslateUi(StrawBerryWidget);

        QMetaObject::connectSlotsByName(StrawBerryWidget);
    } // setupUi

    void retranslateUi(QWidget *StrawBerryWidget)
    {
        StrawBerryWidget->setWindowTitle(QCoreApplication::translate("StrawBerryWidget", "Form", nullptr));
        btnBackFromGrowth->setText(QCoreApplication::translate("StrawBerryWidget", " home", nullptr));
        QTableWidgetItem *___qtablewidgetitem = strawEventTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("StrawBerryWidget", "Image", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = strawEventTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("StrawBerryWidget", "Num", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = strawEventTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("StrawBerryWidget", "EventName", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = strawEventTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("StrawBerryWidget", "Time", nullptr));
    } // retranslateUi

};

namespace Ui {
    class StrawBerryWidget: public Ui_StrawBerryWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_STRAWBERRYWIDGET_H
