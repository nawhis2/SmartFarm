/********************************************************************************
** Form generated from reading UI file 'detectmainwidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DETECTMAINWIDGET_H
#define UI_DETECTMAINWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DetectMainWidget
{
public:
    QPushButton *btnFire;
    QPushButton *btnMap;
    QPushButton *btnSync;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QLabel *videoLabel2;
    QPushButton *startCameraButton;
    QLabel *videoLabel3;
    QLabel *videoLabel4;
    QLabel *videoLabel1;
    QPushButton *btnIntrusion;
    QPushButton *btnGrowth;

    void setupUi(QWidget *DetectMainWidget)
    {
        if (DetectMainWidget->objectName().isEmpty())
            DetectMainWidget->setObjectName("DetectMainWidget");
        DetectMainWidget->resize(968, 600);
        btnFire = new QPushButton(DetectMainWidget);
        btnFire->setObjectName("btnFire");
        btnFire->setGeometry(QRect(80, 140, 71, 61));
        btnMap = new QPushButton(DetectMainWidget);
        btnMap->setObjectName("btnMap");
        btnMap->setGeometry(QRect(80, 410, 71, 61));
        btnSync = new QPushButton(DetectMainWidget);
        btnSync->setObjectName("btnSync");
        btnSync->setGeometry(QRect(880, 40, 41, 41));
        QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::SyncSynchronizing));
        btnSync->setIcon(icon);
        layoutWidget = new QWidget(DetectMainWidget);
        layoutWidget->setObjectName("layoutWidget");
        layoutWidget->setGeometry(QRect(180, 110, 697, 416));
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        videoLabel2 = new QLabel(layoutWidget);
        videoLabel2->setObjectName("videoLabel2");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Ignored);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(videoLabel2->sizePolicy().hasHeightForWidth());
        videoLabel2->setSizePolicy(sizePolicy);

        gridLayout->addWidget(videoLabel2, 1, 0, 1, 1);

        startCameraButton = new QPushButton(layoutWidget);
        startCameraButton->setObjectName("startCameraButton");

        gridLayout->addWidget(startCameraButton, 3, 0, 1, 2);

        videoLabel3 = new QLabel(layoutWidget);
        videoLabel3->setObjectName("videoLabel3");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(videoLabel3->sizePolicy().hasHeightForWidth());
        videoLabel3->setSizePolicy(sizePolicy1);
        videoLabel3->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout->addWidget(videoLabel3, 2, 0, 1, 1);

        videoLabel4 = new QLabel(layoutWidget);
        videoLabel4->setObjectName("videoLabel4");
        sizePolicy1.setHeightForWidth(videoLabel4->sizePolicy().hasHeightForWidth());
        videoLabel4->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(videoLabel4, 2, 1, 1, 1);

        videoLabel1 = new QLabel(layoutWidget);
        videoLabel1->setObjectName("videoLabel1");
        sizePolicy1.setHeightForWidth(videoLabel1->sizePolicy().hasHeightForWidth());
        videoLabel1->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(videoLabel1, 1, 1, 1, 1);

        btnIntrusion = new QPushButton(DetectMainWidget);
        btnIntrusion->setObjectName("btnIntrusion");
        btnIntrusion->setGeometry(QRect(80, 320, 71, 61));
        btnGrowth = new QPushButton(DetectMainWidget);
        btnGrowth->setObjectName("btnGrowth");
        btnGrowth->setGeometry(QRect(80, 230, 71, 61));

        retranslateUi(DetectMainWidget);

        QMetaObject::connectSlotsByName(DetectMainWidget);
    } // setupUi

    void retranslateUi(QWidget *DetectMainWidget)
    {
        DetectMainWidget->setWindowTitle(QCoreApplication::translate("DetectMainWidget", "Form", nullptr));
        btnFire->setText(QCoreApplication::translate("DetectMainWidget", "\360\237\224\245", nullptr));
        btnMap->setText(QCoreApplication::translate("DetectMainWidget", "\360\237\223\212", nullptr));
        btnSync->setText(QString());
        videoLabel2->setText(QString());
        startCameraButton->setText(QCoreApplication::translate("DetectMainWidget", "Start Camera", nullptr));
        videoLabel3->setText(QString());
        videoLabel4->setText(QString());
        videoLabel1->setText(QString());
        btnIntrusion->setText(QCoreApplication::translate("DetectMainWidget", "\360\237\232\252", nullptr));
        btnGrowth->setText(QCoreApplication::translate("DetectMainWidget", "\360\237\215\223", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DetectMainWidget: public Ui_DetectMainWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DETECTMAINWIDGET_H
