#ifndef DETECTCOREWIDGET_H
#define DETECTCOREWIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QTableWidget>

class DetectCoreWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetectCoreWidget(QStackedWidget *stack, QWidget *parent = nullptr);

private:
    QStackedWidget *stackWidget;
protected:
    QString type;
    QTableWidget *tableWidget;

protected slots:
    // 페이지 전환
    void showHomePage();
    void onPageChanged(int index);

protected:
    void changePage(const int index);
    virtual void pageChanged();
};

#endif // DETECTCOREWIDGET_H
