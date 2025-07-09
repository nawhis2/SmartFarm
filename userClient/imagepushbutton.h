#ifndef IMAGEPUSHBUTTON_H
#define IMAGEPUSHBUTTON_H

#include <QPushButton>

class ImagePushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ImagePushButton(QWidget *parent = nullptr);
    ~ImagePushButton();

private slots:
    void clickedButton();

private:
    QString imgUrl;

public:
    void setImgUrl(const QString url) {imgUrl = url;}

private:
    void readImageFromSSL();
};

#endif // IMAGEPUSHBUTTON_H
