#ifndef DETECTCOREWIDGET_H
#define DETECTCOREWIDGET_H

#include "corestackwidget.h"
#include <QStackedWidget>
#include <QFuture>

class CustomTableWidget;

class DetectCoreWidget : public CoreStackWidget
{
    Q_OBJECT
public:
    explicit DetectCoreWidget(QStackedWidget *stack, QWidget *parent = nullptr);

private:
    int                   m_currentProcessingPage;
    int                   m_pendingPage;

    static QFuture<void>        m_loadFuture;
    static QFutureWatcher<void> m_loadWatcher;

protected:
    CustomTableWidget *tableWidget;
    int myIndex;

protected slots:
    void onPageChanged(int index);

protected:
    void pageChanged(const int index);
    void startPageChanged(const int index);
    virtual void pageChangedIdx();
};

#endif // DETECTCOREWIDGET_H
