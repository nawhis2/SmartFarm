#ifndef CUSTOMTABLEWIDGET_H
#define CUSTOMTABLEWIDGET_H

#include <QWidget>
#include <QVector>

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

private slots:
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void refreshPage();

private:
    Ui::CustomTableWidget* ui;

    QStandardItemModel*    m_sourceModel;
    PaginationProxyModel*  m_proxy;

    QVector<QPixmap> m_pixmaps;
    std::string detectStr;

private:
    void updateButtons();
    void onNewData(const QStringList& fields);
    void loadData();
};

#endif // CUSTOMTABLEWIDGET_H
