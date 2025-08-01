#include "corestackwidget.h"
#include <QStackedWidget>
CoreStackWidget::CoreStackWidget(QStackedWidget *stack, QWidget *parent)
    : QWidget{parent}
    , stackWidget(stack)
    , type("")
{

}

void CoreStackWidget::showHomePage() {
    changePage(0);
}

void CoreStackWidget::changePage(const int index) {
    if (stackWidget)
        stackWidget->setCurrentIndex(index);
}
