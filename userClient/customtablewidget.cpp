#include "CustomTableWidget.h"
#include "ui_CustomTableWidget.h"
#include "paginationproxymodel.h"
#include "imagepushbutton.h"
#include "clientUtil.h"
#include "network.h"
#include <QStandardItemModel>
#include <QtConcurrent>

CustomTableWidget::CustomTableWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomTableWidget)
    , m_sourceModel(nullptr)
    , m_proxy(nullptr)
{
    ui->setupUi(this);
    ui->eventTable->verticalHeader()->hide();
}

CustomTableWidget::~CustomTableWidget(){
    delete ui;
}

void CustomTableWidget::initModelAndView() {
    m_pixmaps.clear();

    m_sourceModel = new QStandardItemModel(this);
    m_sourceModel->setColumnCount(4);
    m_sourceModel->setHorizontalHeaderLabels(
        QStringList() << "Image" << "Num" << "Eventname" << "Date"
        );

    m_proxy = new PaginationProxyModel(this);
    m_proxy->setSourceModel(m_sourceModel);
    m_proxy->setPageSize(10);

    connect(m_proxy, &PaginationProxyModel::layoutChanged,
            this, &CustomTableWidget::refreshPage);

    ui->eventTable->setModel(m_proxy);

    updateButtons();
    loadData();
}

void CustomTableWidget::on_prevButton_clicked() {
    int pg = m_proxy->currentPage();
    if (pg > 0 && pg <= m_proxy->pageCount()) {
        m_proxy->setCurrentPage(pg - 1);
        updateButtons();
    }
}

void CustomTableWidget::on_nextButton_clicked() {
    int pg = m_proxy->currentPage();
    if (pg + 1 < m_proxy->pageCount()) {
        m_proxy->setCurrentPage(pg + 1);
        updateButtons();
    }
}

void CustomTableWidget::updateButtons() {
    ui->prevButton->setEnabled(m_proxy->currentPage() > 0);
    ui->nextButton->setEnabled(m_proxy->currentPage() + 1 < m_proxy->pageCount());
    refreshPage();
}

void CustomTableWidget::onNewData(const QStringList& fields) {
    int neededCols = fields.size();
    if (m_sourceModel->columnCount() < neededCols) {
        m_sourceModel->setColumnCount(neededCols);
    }

    QList<QStandardItem*> items;
    items.append(new QStandardItem());
    for (const auto& f : fields) {
        items.append(new QStandardItem(f));
    }

    int oldPageCount = m_proxy->pageCount();
    m_sourceModel->appendRow(items);
    int newPageCount = m_proxy->pageCount();

    if (newPageCount != oldPageCount) {
        updateButtons();
    }
}

void CustomTableWidget::loadData() {
    (void)QtConcurrent::run([this]() {
        sendFile(detectStr.c_str(), "DATA");

        QMetaObject::invokeMethod(this, [this]() {
            m_sourceModel->removeRows(0, m_sourceModel->rowCount());
            m_pixmaps.clear();
            m_proxy->setCurrentPage(0);
            updateButtons();
        }, Qt::QueuedConnection);

        while (1) {
            int fileSize = 0;
            int n = SSL_read(sock_fd, &fileSize, sizeof(fileSize));
            if (n <= 0 || fileSize == -1) break;

            QByteArray imgData;
            imgData.reserve(fileSize);
            char buf[4096];
            int bytesRead = 0;
            while (bytesRead < fileSize) {
                int toRead = qMin(fileSize - bytesRead, (int)sizeof(buf));
                int r = SSL_read(sock_fd, buf, toRead);
                if (r <= 0) break;
                imgData.append(buf, r);
                bytesRead += r;
            }

            QPixmap pixmap;
            if (!pixmap.loadFromData(imgData)) {
                qWarning() << "이미지 로드 실패, size=" << fileSize;
                continue;
            }

            char textBuf[1024];
            int tn = SSL_read(sock_fd, textBuf, sizeof(textBuf) - 1);
            if (tn <= 0) break;
            textBuf[tn] = '\0';
            if (!strncmp(textBuf, "END", 3)) break;

            QStringList fields = QString::fromUtf8(textBuf).trimmed().split('|', Qt::SkipEmptyParts);

            QMetaObject::invokeMethod(this, [this, fields, pixmap]() {
                onNewData(fields);
                m_pixmaps.append(pixmap);
            }, Qt::QueuedConnection);
        }
    });
}

void CustomTableWidget::refreshPage()
{
    // 기존 위젯 정리
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        QModelIndex pIdx = m_proxy->index(row, 0);
        if (!pIdx.isValid()) continue;

        QWidget* existing = ui->eventTable->indexWidget(pIdx);
        if (existing) {
            ui->eventTable->setIndexWidget(pIdx, nullptr);
            delete existing;
        }
    }

    // 새로 셀에 맞게 버튼 생성해서 넣기
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        QModelIndex pIdx = m_proxy->index(row, 0);
        QModelIndex sIdx = m_proxy->mapToSource(pIdx);
        int srcRow = sIdx.row();

        if (!pIdx.isValid() || srcRow < 0 || srcRow >= m_pixmaps.size())
            continue;

        auto* btn = new ImagePushButton(ui->eventTable);
        btn->setFixedSize(80, 45);
        btn->setPixmap(m_pixmaps[srcRow]);

        ui->eventTable->setIndexWidget(pIdx, btn);
    }
}
