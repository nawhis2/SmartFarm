/********************************************************************************
** Form generated from reading UI file 'intrusionwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INTRUSIONWIDGET_H
#define UI_INTRUSIONWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IntrusionWidget
{
public:
    QPushButton *btnBackFromIntrusion;
    QTableWidget *intrusionEventTable;

    void setupUi(QWidget *IntrusionWidget)
    {
        if (IntrusionWidget->objectName().isEmpty())
            IntrusionWidget->setObjectName("IntrusionWidget");
        IntrusionWidget->resize(968, 600);
        btnBackFromIntrusion = new QPushButton(IntrusionWidget);
        btnBackFromIntrusion->setObjectName("btnBackFromIntrusion");
        btnBackFromIntrusion->setGeometry(QRect(850, 540, 95, 29));
        intrusionEventTable = new QTableWidget(IntrusionWidget);
        if (intrusionEventTable->columnCount() < 4)
            intrusionEventTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        intrusionEventTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        intrusionEventTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        intrusionEventTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        intrusionEventTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        intrusionEventTable->setObjectName("intrusionEventTable");
        intrusionEventTable->setGeometry(QRect(40, 40, 901, 491));
        QFont font;
        font.setPointSize(20);
        intrusionEventTable->setFont(font);
        intrusionEventTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        intrusionEventTable->verticalHeader()->setVisible(false);

        retranslateUi(IntrusionWidget);

        QMetaObject::connectSlotsByName(IntrusionWidget);
    } // setupUi

    void retranslateUi(QWidget *IntrusionWidget)
    {
        IntrusionWidget->setWindowTitle(QCoreApplication::translate("IntrusionWidget", "Form", nullptr));
        btnBackFromIntrusion->setText(QCoreApplication::translate("IntrusionWidget", " home", nullptr));
        QTableWidgetItem *___qtablewidgetitem = intrusionEventTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("IntrusionWidget", "Image", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = intrusionEventTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("IntrusionWidget", "Num", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = intrusionEventTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("IntrusionWidget", "EventName", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = intrusionEventTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("IntrusionWidget", "Time", nullptr));
    } // retranslateUi

};

namespace Ui {
    class IntrusionWidget: public Ui_IntrusionWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INTRUSIONWIDGET_H
