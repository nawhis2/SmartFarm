#ifndef LOGCUSTOMTABLEWIDGET_H
#define LOGCUSTOMTABLEWIDGET_H

#include <QWidget>

#include <QWidget>
#include <QStandardItemModel>
#include "paginationproxymodel.h"

namespace Ui {
class CustomTableWidget;
}

struct LogEntry;

class LogCustomTableWidget : public QWidget {
    Q_OBJECT

public:
    explicit LogCustomTableWidget(QWidget* parent = nullptr);
    ~LogCustomTableWidget();

    /// 페이지당 아이템 수 설정
    void setPageSize(int sz);

    /// UI에서 전체 데이터를 한 번에 채우고 싶을 때 (필요시)
    void setData(const QList<QStringList>& rows);

private slots:
    /// 로그 파일을 loadLogData()로 다시 불러온 뒤 전체 리셋
    void onLogsLoaded();
    /// makeAndSaveLogData() 호출로 새 로그가 추가될 때 한 줄만 append
    void onLogAppended(const LogEntry& entry);

    void on_prevButton_clicked();
    void on_nextButton_clicked();

private:
    /// prev/next 버튼 활성 상태 갱신
    void updateButtons();

    Ui::CustomTableWidget*   ui;
    QStandardItemModel*      m_sourceModel;
    PaginationProxyModel*    m_proxy;
    int                      m_pageSize;
};

#endif // LOGCUSTOMTABLEWIDGET_H
