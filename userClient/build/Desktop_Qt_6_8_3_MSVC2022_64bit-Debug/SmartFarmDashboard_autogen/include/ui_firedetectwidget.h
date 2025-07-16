/********************************************************************************
** Form generated from reading UI file 'firedetectwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FIREDETECTWIDGET_H
#define UI_FIREDETECTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FireDetectWidget
{
public:
    QPushButton *btnBackFromFire;
    QTableWidget *fireEventTable;

    void setupUi(QWidget *FireDetectWidget)
    {
        if (FireDetectWidget->objectName().isEmpty())
            FireDetectWidget->setObjectName("FireDetectWidget");
        FireDetectWidget->resize(968, 600);
        btnBackFromFire = new QPushButton(FireDetectWidget);
        btnBackFromFire->setObjectName("btnBackFromFire");
        btnBackFromFire->setGeometry(QRect(850, 540, 95, 29));
        fireEventTable = new QTableWidget(FireDetectWidget);
        if (fireEventTable->columnCount() < 4)
            fireEventTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        fireEventTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        fireEventTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        fireEventTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        fireEventTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        fireEventTable->setObjectName("fireEventTable");
        fireEventTable->setGeometry(QRect(40, 40, 901, 491));
        QFont font;
        font.setPointSize(20);
        fireEventTable->setFont(font);
        fireEventTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        fireEventTable->verticalHeader()->setVisible(false);

        retranslateUi(FireDetectWidget);

        QMetaObject::connectSlotsByName(FireDetectWidget);
    } // setupUi

    void retranslateUi(QWidget *FireDetectWidget)
    {
        FireDetectWidget->setWindowTitle(QCoreApplication::translate("FireDetectWidget", "Form", nullptr));
        btnBackFromFire->setText(QCoreApplication::translate("FireDetectWidget", " home", nullptr));
        QTableWidgetItem *___qtablewidgetitem = fireEventTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("FireDetectWidget", "Image", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = fireEventTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("FireDetectWidget", "Num", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = fireEventTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("FireDetectWidget", "EventName", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = fireEventTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("FireDetectWidget", "Time", nullptr));
    } // retranslateUi

};

namespace Ui {
    class FireDetectWidget: public Ui_FireDetectWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FIREDETECTWIDGET_H
