#include "imagepushbutton.h"

ImagePushButton::ImagePushButton(QWidget *parent)
    : QPushButton(parent)
{
    setText("image");
    setStyleSheet("QPushButton { background-color: transparent; border: none; text-decoration: underline; color: blue}");
    connect(this, &QPushButton::clicked, this, &ImagePushButton::clickedButton);
}

ImagePushButton::~ImagePushButton()
{
}

void ImagePushButton::clickedButton(){
    qDebug() << imgUrl;
}
