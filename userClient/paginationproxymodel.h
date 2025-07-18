#ifndef PAGINATIONPROXYMODEL_H
#define PAGINATIONPROXYMODEL_H

#include <QSortFilterProxyModel>

class PaginationProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    PaginationProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent), m_pageSize(10), m_currentPage(0) {}

    void setPageSize(int sz) { m_pageSize = sz; invalidateFilter(); }
    void setCurrentPage(int pg) { m_currentPage = pg; invalidateFilter(); }
    int  pageCount() const {
        int rows = sourceModel() ? sourceModel()->rowCount() : 0;
        return (rows + m_pageSize - 1) / m_pageSize;
    }
    int  currentPage() const { return m_currentPage; }

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
