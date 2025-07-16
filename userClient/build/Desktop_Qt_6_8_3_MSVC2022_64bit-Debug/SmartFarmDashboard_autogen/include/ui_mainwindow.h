/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QStackedWidget *stackedWidget;
    QWidget *pageMainMenu;
    QWidget *pageFire;
    QWidget *pageIntrusion;
    QWidget *pageGrowth;
    QWidget *pageSensor;
    QPushButton *btnBackFromSensor;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(968, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(0, 0, 971, 581));
        pageMainMenu = new QWidget();
        pageMainMenu->setObjectName("pageMainMenu");
        stackedWidget->addWidget(pageMainMenu);
        pageFire = new QWidget();
        pageFire->setObjectName("pageFire");
        stackedWidget->addWidget(pageFire);
        pageIntrusion = new QWidget();
        pageIntrusion->setObjectName("pageIntrusion");
        stackedWidget->addWidget(pageIntrusion);
        pageGrowth = new QWidget();
        pageGrowth->setObjectName("pageGrowth");
        stackedWidget->addWidget(pageGrowth);
        pageSensor = new QWidget();
        pageSensor->setObjectName("pageSensor");
        btnBackFromSensor = new QPushButton(pageSensor);
        btnBackFromSensor->setObjectName("btnBackFromSensor");
        btnBackFromSensor->setGeometry(QRect(120, 180, 95, 29));
        stackedWidget->addWidget(pageSensor);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 968, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btnBackFromSensor->setText(QCoreApplication::translate("MainWindow", " home", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
