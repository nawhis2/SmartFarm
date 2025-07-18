#ifndef DETECTCOREWIDGET_H
#define DETECTCOREWIDGET_H

#include <QWidget>
#include <QStackedWidget>

class CustomTableWidget;

class DetectCoreWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetectCoreWidget(QStackedWidget *stack, QWidget *parent = nullptr);

private:
    QStackedWidget *stackWidget;

protected:
    std::string type;
    CustomTableWidget *tableWidget;
    int myIndex;

protected slots:
    // 페이지 전환
    void showHomePage();
    void onPageChanged(int index);

protected:
    void changePage(const int index);
    void pageChanged(const int index);
    virtual void pageChangedIdx();
};

#endif // DETECTCOREWIDGET_H
