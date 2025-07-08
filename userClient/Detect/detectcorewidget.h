#ifndef DETECTCOREWIDGET_H
#define DETECTCOREWIDGET_H

#include <QWidget>
#include <QStackedWidget>

class DetectCoreWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetectCoreWidget(QStackedWidget *stack, QWidget *parent = nullptr);

private:
    QString type;
    QStackedWidget *stackWidget;

protected slots:
    // 페이지 전환
    void showHomePage();

protected:
    void changePage(const int index);
};

#endif // DETECTCOREWIDGET_H
