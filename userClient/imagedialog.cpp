#include "imagedialog.h"
#include "ui_imagedialog.h"
#include <QScreen>
#include <QGuiApplication>
ImageDialog::ImageDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ImageDialog)
{
    ui->setupUi(this);
}

ImageDialog::~ImageDialog()
{
    delete ui;
}

void ImageDialog::setImg(const QPixmap &pix) {
    QSize screenSize = QGuiApplication::primaryScreen()->availableSize();
    QSize targetSize = pix.size().boundedTo(screenSize * 0.9);  // 화면 90% 이하 제한

    QPixmap scaled = pix.scaled(targetSize, Qt::KeepAspectRatio);
    ui->labelImg->setPixmap(scaled);
    ui->labelImg->resize(scaled.size());
    resize(scaled.size());
}
