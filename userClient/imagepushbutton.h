#ifndef IMAGEPUSHBUTTON_H
#define IMAGEPUSHBUTTON_H

#include <QPushButton>

class ImagePushButton : public QPushButton {
    Q_OBJECT
public:
    explicit ImagePushButton(QWidget *parent = nullptr);
    ~ImagePushButton();

    // 외부에서 pixmap을 통째로 전달
    void setPixmap(const QPixmap& pix);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    // 버튼 클릭 시, 저장된 pixmap을 다이얼로그에 띄워 줌
    void clickedButton();

private:
    // 버튼 프리뷰용 pixmap
    QPixmap m_preview;
    // 클릭 시 보여줄 풀사이즈 pixmap
    QPixmap m_full;

    // m_preview 를 버튼 크기에 맞춰 icon 로 갱신
    void updateImage();
};

#endif // IMAGEPUSHBUTTON_H
