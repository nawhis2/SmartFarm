#ifndef CUSTOMTABLEWIDGET_H
#define CUSTOMTABLEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTableWidget>
#include "customtablewidget.h"
#include "ui_customtablewidget.h"
#include <functional>

class QStandardItemModel;
class PaginationProxyModel;
class ImagePushButton;

namespace Ui {
class CustomTableWidget;
}

class CustomTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit CustomTableWidget(QWidget* parent = nullptr);
    ~CustomTableWidget();

public:
    void SetDetectStr(const std::string& detectStr){ this->detectStr = detectStr;}
    void initModelAndView();
    QTableView* getInnerTable() const;
    void setPageSize(const int pageSize) { m_pageSize = pageSize; }
    std::function<void(const QStringList&)> onNewDataHook;  // 외부 후처리 훅

private slots:
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void on_eventTable_clicked(const QModelIndex &index);
    void refreshPage();

private:
    Ui::CustomTableWidget* ui;

    QStandardItemModel*    m_sourceModel;
    PaginationProxyModel*  m_proxy;

    QVector<QPixmap> m_pixmaps;
    std::string detectStr;

    int m_pageSize;    // 한 페이지에 몇 개
    int m_currentPage;    // 현재 페이지
    int m_totalCount; // 전체 페이지
    int m_pageCount; // 전체 페이지 수
    bool m_hasNext;

private:
    void updateButtons();
    void onNewData(const QStringList& fields);
    void loadData(const int page);
    void adjustRowHeightsToFitTable();
};

#endif // CUSTOMTABLEWIDGET_H
