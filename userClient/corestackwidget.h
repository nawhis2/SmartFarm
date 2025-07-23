#ifndef CORESTACKWIDGET_H
#define CORESTACKWIDGET_H

#include <QWidget>

class QStackedWidget;

class CoreStackWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CoreStackWidget(QStackedWidget *stack, QWidget *parent = nullptr);

protected:
    std::string type;
    QStackedWidget        *stackWidget;

    void changePage(const int index);

protected slots:
    // 페이지 전환
    void showHomePage();
};

#endif // CORESTACKWIDGET_H
