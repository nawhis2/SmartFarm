/********************************************************************************
** Form generated from reading UI file 'imagedialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGEDIALOG_H
#define UI_IMAGEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_ImageDialog
{
public:
    QLabel *labelImg;

    void setupUi(QDialog *ImageDialog)
    {
        if (ImageDialog->objectName().isEmpty())
            ImageDialog->setObjectName("ImageDialog");
        ImageDialog->resize(640, 640);
        labelImg = new QLabel(ImageDialog);
        labelImg->setObjectName("labelImg");
        labelImg->setGeometry(QRect(0, 0, 640, 640));
        labelImg->setAlignment(Qt::AlignmentFlag::AlignLeading|Qt::AlignmentFlag::AlignLeft|Qt::AlignmentFlag::AlignTop);

        retranslateUi(ImageDialog);

        QMetaObject::connectSlotsByName(ImageDialog);
    } // setupUi

    void retranslateUi(QDialog *ImageDialog)
    {
        ImageDialog->setWindowTitle(QCoreApplication::translate("ImageDialog", "Dialog", nullptr));
        labelImg->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ImageDialog: public Ui_ImageDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGEDIALOG_H
