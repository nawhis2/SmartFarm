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
    , m_pageSize(10)
    , m_currentPage(0)
    , m_totalCount(0)
    , m_pageCount(0)
    , m_hasNext(false)
{
    ui->setupUi(this);

    ui->eventTable->verticalHeader()->setVisible(false);
    ui->eventTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->eventTable->verticalHeader()->setDefaultSectionSize(75);  // 행 높이 기본값

    ui->eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->eventTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->eventTable->setItemDelegateForColumn(3, new FullRowImageDelegate(this));

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
        m_sourceModel->setColumnCount(4);
        m_sourceModel->setHorizontalHeaderLabels(
            QStringList() << "Num" << "Date" << "Eventname" << "Image"
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
    loadData(0);
}

void CustomTableWidget::on_prevButton_clicked(){
    if(m_currentPage > 0)
        loadData(m_currentPage - 1);
}

void CustomTableWidget::on_nextButton_clicked(){
    loadData(m_currentPage + 1);
}

void CustomTableWidget::updateButtons() {
    ui->prevButton->setEnabled(m_currentPage > 0);
    ui->nextButton->setEnabled(m_hasNext);
    refreshPage();
}

void CustomTableWidget::onNewData(const QStringList& fields) {
    // 1) 현재 페이지에서의 이 행 인덱스
    int rowInPage = m_sourceModel->rowCount();
    // 2) 전체 일련번호 계산 (0-based page * size + localIndex + 1)
    int globalNum = m_currentPage * m_pageSize + rowInPage + 1;

    QList<QStandardItem*> items;

    // 3) Num 칼럼 아이템
    QStandardItem* numItem = new QStandardItem(QString::number(globalNum));
    numItem->setTextAlignment(Qt::AlignCenter);
    items.append(numItem);

    // 4) Date, Eventname 칼럼
    for (const QString& f : fields) {
        QStandardItem* item = new QStandardItem(f);
        item->setTextAlignment(Qt::AlignCenter);
        //item->setBackground(QColor("#0d1e1e"));   // 메인 배경색
        item->setForeground(QColor("#b8f1cc"));
        items.append(item);
    }

    // 5) Image 자리(나중에 decorationRole로 채움)
    QStandardItem* imgPlaceholder = new QStandardItem();
    items.append(imgPlaceholder);

    // 6) 모델에 추가
    int oldPageCount = m_proxy ? m_proxy->pageCount() : 0;
    m_sourceModel->appendRow(items);

    // 7) (필요시) 페이지 버튼 업데이트
    int newPageCount = m_proxy->pageCount();
    if (newPageCount != oldPageCount) {
        updateButtons();
    }
}

void CustomTableWidget::loadData(int page) {
    m_currentPage = page;

    // 1) 모델 초기화
    QMetaObject::invokeMethod(this, [this] {
        m_sourceModel->removeRows(0, m_sourceModel->rowCount());
        m_pixmaps.clear();
    }, Qt::QueuedConnection);

    // 2) 서버에 요청: "eventType|limit|offset"
    int limit  = m_pageSize;
    int offset = page * limit;
    std::string payload = detectStr
                          + "|" + std::to_string(limit)
                          + "|" + std::to_string(offset);
    sendFile(payload.c_str(), "DATA");

    // 3) 데이터 받기 (limit개) + 플래그 받기
    // ───────────────────────────────
    while (true) {
        // 1) 텍스트 라인 읽기
        char lineBuf[1024];
        int ln = SSL_read(sock_fd, lineBuf, sizeof(lineBuf)-1);
        if (ln <= 0) return;            // 통신 오류
        lineBuf[ln] = '\0';

        // 2) 플래그 검사
        if (strcmp(lineBuf, "HAS_NEXT") == 0) {
            m_hasNext = true;
            break;
        }
        if (strcmp(lineBuf, "NO_MORE") == 0) {
            m_hasNext = false;
            break;
        }

        // 3) 데이터 필드 파싱
        QStringList fields = QString::fromUtf8(lineBuf)
                                 .trimmed()
                                 .split('|', Qt::SkipEmptyParts);

        // 4) 이미지 크기 읽기
        int fileSize = 0;
        if (SSL_read(sock_fd, &fileSize, sizeof(fileSize)) <= 0
            || fileSize <= 0) {
            m_hasNext = false;
            break;
        }

        // 5) 이미지 데이터 읽기
        QByteArray imgData;
        imgData.reserve(fileSize);
        int got = 0;
        char buf[4096];
        while (got < fileSize) {
            int r = SSL_read(sock_fd, buf,
                             qMin((int)sizeof(buf), fileSize - got));
            if (r <= 0) break;
            imgData.append(buf, r);
            got += r;
        }

        // 6) UI 반영
        QPixmap pix;
        if (pix.loadFromData(imgData)) {
            QMetaObject::invokeMethod(this, [this, fields, pix] {
                onNewData(fields);
                m_pixmaps.append(pix);
            }, Qt::QueuedConnection);
        }
    }

    // 4) 버튼 상태 갱신 (Qt 스레드 안전)
    QMetaObject::invokeMethod(this, [this] {
        updateButtons();
    }, Qt::QueuedConnection);
}

void CustomTableWidget::refreshPage() {
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        QModelIndex pIdx = m_proxy->index(r, 3);
        QModelIndex sIdx = m_proxy->mapToSource(pIdx);
        int src = sIdx.row();
        if (!pIdx.isValid() || src < 0 || src >= m_pixmaps.size())
            continue;
        m_sourceModel->setData(
            m_sourceModel->index(src, 3),
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

QTableView* CustomTableWidget::getInnerTable() const {
    return ui->eventTable;  // QTableView
}
