#include "detectcorewidget.h"

DetectCoreWidget::DetectCoreWidget(QStackedWidget *stack,QWidget *parent)
    : QWidget{parent}
    , type("")
    , stackWidget(stack)
{

}

void DetectCoreWidget::showHomePage(){
    changePage(0);
}

void DetectCoreWidget::changePage(const int index){
    if(stackWidget)
        stackWidget->setCurrentIndex(index);
}
