#include "CustomTableWidget.h"
#include "ui_CustomTableWidget.h"
#include "paginationproxymodel.h"
#include "clientUtil.h"
#include "network.h"
#include "fullrowimagedelegate.h"
#include "imagedialog.h"
#include <QStandardItemModel>
#include <QLabel>


CustomTableWidget::CustomTableWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomTableWidget)
    , m_sourceModel(nullptr)
    , m_proxy(nullptr)
{
    ui->setupUi(this);
    ui->eventTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->eventTable->verticalHeader()->setDefaultSectionSize(75);  // 행 높이 기본값

    ui->eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->eventTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->eventTable->setItemDelegateForColumn(0, new FullRowImageDelegate(this));

    ui->eventTable->setIconSize(QSize(80,45));
    ui->eventTable->verticalHeader()->setDefaultSectionSize(45);
    ui->eventTable->setColumnWidth(0, 80);
}

CustomTableWidget::~CustomTableWidget(){
    delete ui;
}

void CustomTableWidget::initModelAndView() {
    // Initialize UI components on main thread
    QMetaObject::invokeMethod(this, [this] {
        m_pixmaps.clear();

        delete m_sourceModel;
        m_sourceModel = new QStandardItemModel(this);
        m_sourceModel->setColumnCount(3);
        m_sourceModel->setHorizontalHeaderLabels(
            QStringList() << "Date" << "Eventname" << "Image"
            );

        delete m_proxy;
        m_proxy = new PaginationProxyModel(this);
        m_proxy->setSourceModel(m_sourceModel);
        m_proxy->setPageSize(10);
        connect(m_proxy, &PaginationProxyModel::layoutChanged,
                this, &CustomTableWidget::refreshPage);

        QHeaderView *header = ui->eventTable->horizontalHeader();
        header->setDefaultAlignment(Qt::AlignCenter);               // 헤더 텍스트 가운데 정렬
        header->setSectionsMovable(false);                          // 드래그로 열 순서 못 바꾸게
        header->setStretchLastSection(false);                       // 마지막 열 자동 확장 비활성화
        header->setSectionResizeMode(QHeaderView::Stretch);         // 전체를 Stretch로 설정

        // 필요한 경우, 열별로 비율 다르게 하고 싶다면 아래 코드 참고
        // (Stretch 상태에서는 비율 조절은 직접적으로 불가. Fixed로 바꿔야 함)
        // header->setSectionResizeMode(0, QHeaderView::Stretch); // Image
        // header->setSectionResizeMode(1, QHeaderView::Stretch); // Num
        // header->setSectionResizeMode(2, QHeaderView::Stretch); // Eventname
        // header->setSectionResizeMode(3, QHeaderView::Stretch); // Date

        ui->eventTable->setModel(m_proxy);
        updateButtons();

        // Span first row across all columns
        int cols = m_sourceModel->columnCount();
        ui->eventTable->setSpan(0, 0, 1, cols);
    }, Qt::QueuedConnection);

    // Directly load data (no QtConcurrent)
    loadData();
}

void CustomTableWidget::on_prevButton_clicked(){
    int pg = m_proxy->currentPage();
    if(pg > 0){
        m_proxy->setCurrentPage(pg - 1);
        updateButtons();
    }
}

void CustomTableWidget::on_nextButton_clicked(){
    int pg = m_proxy->currentPage();
    if(pg + 1 < m_proxy->pageCount()){
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
    int oldPageCount = m_proxy ? m_proxy->pageCount() : 0;
    QList<QStandardItem*> items;
    for (const QString& f : fields) {
        QStandardItem* item = new QStandardItem(f);
        item->setTextAlignment(Qt::AlignCenter);
        //item->setBackground(QColor("#0d1e1e"));   // 메인 배경색
        item->setForeground(QColor("#b8f1cc"));   // 텍스트 색상
        items.append(item);
    }
    items.append(new QStandardItem());
    m_sourceModel->appendRow(items);
    int newPageCount = m_proxy->pageCount();
    if (newPageCount != oldPageCount) {
        updateButtons();
    }
}

void CustomTableWidget::loadData() {
    // Reset model and UI before loading
    QMetaObject::invokeMethod(this, [this] {
        m_sourceModel->removeRows(0, m_sourceModel->rowCount());
        m_pixmaps.clear();
        m_proxy->setCurrentPage(0);
        updateButtons();
    }, Qt::QueuedConnection);

    // Perform network I/O synchronously (caller thread)
    sendFile(detectStr.c_str(), "DATA");
    while (true) {
        char textBuf[1024];
        int tn = SSL_read(sock_fd, textBuf, sizeof(textBuf)-1);
        if (tn <= 0) break;
        textBuf[tn] = '\0';
        if (!strncmp(textBuf, "END", 3)) break;

        QStringList fields = QString::fromUtf8(textBuf)
                                 .trimmed()
                                 .split('|', Qt::SkipEmptyParts);

        int fileSize;
        if (SSL_read(sock_fd, &fileSize, sizeof(fileSize)) <= 0 || fileSize == -1)
            break;

        QByteArray imgData;
        imgData.reserve(fileSize);
        int readBytes = 0;
        char buf[4096];
        while (readBytes < fileSize) {
            int r = SSL_read(sock_fd, buf, qMin((int)sizeof(buf), fileSize - readBytes));
            if (r <= 0) break;
            imgData.append(buf, r);
            readBytes += r;
        }

        QPixmap pix;
        if (!pix.loadFromData(imgData)) {
            qWarning() << "이미지 로드 실패, size=" << fileSize;
            continue;
        }

        QMetaObject::invokeMethod(this, [this, fields, pix]() {
            onNewData(fields);
            m_pixmaps.append(pix);
            refreshPage();
        }, Qt::QueuedConnection);
    }
}

void CustomTableWidget::refreshPage() {
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        QModelIndex pIdx = m_proxy->index(r, 2);
        QModelIndex sIdx = m_proxy->mapToSource(pIdx);
        int src = sIdx.row();
        if (!pIdx.isValid() || src < 0 || src >= m_pixmaps.size())
            continue;
        m_sourceModel->setData(
            m_sourceModel->index(src, 2),
            m_pixmaps[src],
            Qt::DecorationRole
            );
    }
}

void CustomTableWidget::on_eventTable_clicked(const QModelIndex &index) {
    int viewRow = index.row();
    QModelIndex proxyIdx = m_proxy->index(viewRow, 0);
    QModelIndex srcIdx   = m_proxy->mapToSource(proxyIdx);
    int srcRow = srcIdx.row();
    if (srcRow < 0 || srcRow >= m_pixmaps.size()) {
        qDebug() << "Clicked row:" << viewRow << "no pixmap.";
        return;
    }
    const QPixmap &pm = m_pixmaps[srcRow];

    ImageDialog* dlg = new ImageDialog(this);
    dlg->setImg(pm);
    dlg->show();
}
