#include "imagepushbutton.h"
#include "clientUtil.h"
#include "network.h"
#include "imagedialog.h"
#include <QtConcurrent>

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
    readImageFromSSL();
}

void ImagePushButton::readImageFromSSL() {
    (void)QtConcurrent::run([this](){
        QByteArray pathBytes = imgUrl.toUtf8();
        const char* cPath = pathBytes.constData();
        sendFile(cPath, "IMAGE");

        int file_size = 0;
        SSL_read(sock_fd, &file_size, sizeof(int));

        QByteArray imgData;
        char buffer[1024];
        int bytesRead = 0;

        while (bytesRead < file_size) {
            int to_read = qMin(file_size - bytesRead, (int)sizeof(buffer));
            int n = SSL_read(sock_fd, buffer, to_read);
            if (n <= 0) break;
            imgData.append(buffer, n);
            bytesRead += n;
        }

        QMetaObject::invokeMethod(this, [=]() {
            QPixmap pix;
            if (pix.loadFromData(imgData)) {
                ImageDialog* dlg = new ImageDialog(this);

                dlg->setImg(pix);
                dlg->show();
            }
        }, Qt::QueuedConnection);
    });
}
