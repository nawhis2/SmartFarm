#ifndef PAGINATIONPROXYMODEL_H
#define PAGINATIONPROXYMODEL_H

#include <QSortFilterProxyModel>

class PaginationProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    PaginationProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent), m_pageSize(10), m_currentPage(0) {}
public:
    void setPageSize(int sz) {
        beginResetModel();           // 🔥 필수
        m_pageSize = sz;
        endResetModel();             // 🔥 필수
    }
    void setCurrentPage(int pg) {
        beginResetModel();       // 👈 추가
        m_currentPage = pg;
        endResetModel();         // 👈 추가
    }
    int  pageCount() const {
        int rows = sourceModel() ? sourceModel()->rowCount() : 0;
        return (rows + m_pageSize - 1) / m_pageSize;
    }
    int  currentPage() const { return m_currentPage; }
    int pageSize() const { return m_pageSize; }

protected:
    bool filterAcceptsRow(int srcRow, const QModelIndex&) const override {
        int start = m_currentPage * m_pageSize;
        return srcRow >= start && srcRow < start + m_pageSize;
    }
private:
    int m_pageSize;
    int m_currentPage;
};

#endif // PAGINATIONPROXYMODEL_H
