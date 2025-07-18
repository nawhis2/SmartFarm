#include "imagepushbutton.h"
#include "imagedialog.h"
#include <QIcon>
#include <QResizeEvent>
#include <QDebug>

ImagePushButton::ImagePushButton(QWidget *parent)
    : QPushButton(parent)
{
    setFlat(true);
    setText(QString());
    connect(this, &QPushButton::clicked, this, &ImagePushButton::clickedButton);
}

ImagePushButton::~ImagePushButton()
{
}

void ImagePushButton::setPixmap(const QPixmap& pix) {
    // preview 와 full 을 모두 저장해 두면, click 시 빠르게 띄울 수 있음
    m_full    = pix;
    m_preview = pix;
    updateImage();
}

void ImagePushButton::resizeEvent(QResizeEvent* event) {
    QPushButton::resizeEvent(event);
    updateImage();
}

void ImagePushButton::updateImage() {
    if (m_preview.isNull()) return;

    // 버튼 크기에서 약간 여백 제외
    QSize target = size() - QSize(4,4);
    QPixmap scaled = m_preview.scaled(
        target,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );
    setIcon(QIcon(scaled));
    setIconSize(scaled.size());
}

void ImagePushButton::clickedButton() {
    if (m_full.isNull()) {
        qWarning() << "ImagePushButton: no full image to show";
        return;
    }
    ImageDialog* dlg = new ImageDialog(this);
    dlg->setImg(m_full);
    dlg->show();
}
