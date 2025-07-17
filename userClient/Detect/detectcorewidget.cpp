#include "detectcorewidget.h"
#include "clientUtil.h"
#include "network.h"
#include <QtConcurrent>
#include <QHeaderView>
#include "customtablewidget.h"

DetectCoreWidget::DetectCoreWidget(QStackedWidget *stack,QWidget *parent)
    : QWidget{parent}
    , stackWidget(stack)
    , type("")
    , tableWidget(nullptr)
    , myIndex(-1)
{
    connect(stackWidget, &QStackedWidget::currentChanged, this, &DetectCoreWidget::onPageChanged);
}

void DetectCoreWidget::showHomePage(){
    changePage(0);
}

void DetectCoreWidget::onPageChanged(int index){
    if(index){
        if (myIndex == index) {
            pageChanged(index);
        }
    }
}

void DetectCoreWidget::changePage(const int index){
    if(stackWidget)
        stackWidget->setCurrentIndex(index);
}

void DetectCoreWidget::pageChanged(const int index){
    pageChangedIdx(index);
}

void DetectCoreWidget::pageChangedIdx(const int index){
    tableWidget->initModelAndView();
}
