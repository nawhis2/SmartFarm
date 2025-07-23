#include "detectcorewidget.h"
#include <QtConcurrent>
#include <QHeaderView>
#include "customtablewidget.h"
#include <QFutureWatcher>

// Static member definitions
QFuture<void> DetectCoreWidget::m_loadFuture;
QFutureWatcher<void> DetectCoreWidget::m_loadWatcher;

DetectCoreWidget::DetectCoreWidget(QStackedWidget *stack, QWidget *parent)
    : CoreStackWidget(stack, parent)
    , tableWidget(nullptr)
    , myIndex(-1)
    , m_currentProcessingPage(-1)
    , m_pendingPage(-1)
{
    connect(stackWidget, &QStackedWidget::currentChanged,
            this, &DetectCoreWidget::onPageChanged);

    // When previous loadFuture finishes, handle any pending page
    connect(&m_loadWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        m_currentProcessingPage = -1;
        if (m_pendingPage != -1) {
            int next = m_pendingPage;
            m_pendingPage = -1;
            startPageChanged(next);
        }
    });
}

void DetectCoreWidget::onPageChanged(int index) {
    if(index){
        if (myIndex == index) {
            pageChanged(index);
        }
    }
}

void DetectCoreWidget::pageChanged(int index) {
    if (m_loadFuture.isRunning()) {
        if (index == m_currentProcessingPage)
            return;
        m_pendingPage = index;
        return;
    }
    startPageChanged(index);
}

void DetectCoreWidget::startPageChanged(int index) {
    m_currentProcessingPage = index;
    m_loadFuture = QtConcurrent::run([this]() {
        pageChangedIdx();
    });
    m_loadWatcher.setFuture(m_loadFuture);
}

void DetectCoreWidget::pageChangedIdx() {
    if (!tableWidget) return;
    tableWidget->initModelAndView();
}
