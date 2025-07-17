// CustomTableWidget.cpp
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
{
    ui->setupUi(this);
    ui->eventTable->verticalHeader()->hide();
}

CustomTableWidget::~CustomTableWidget(){
    delete ui;
}

void CustomTableWidget::initModelAndView() {

    // 1) 소스 모델 준비 (컬럼 수는 데이터에 맞게 조절)
    m_sourceModel = new QStandardItemModel(this);

    // 2) 헤더 설정
    m_sourceModel->setColumnCount(4);
    m_sourceModel->setHorizontalHeaderLabels(
        QStringList() << "Image" << "Num" << "Eventname" << "Date"
        );

    // 3) 페이지네이션 프록시 설정
    m_proxy = new PaginationProxyModel(this);
    m_proxy->setSourceModel(m_sourceModel);
    m_proxy->setPageSize(10);

    // 4) QTableView에 연결
    ui->eventTable->setModel(m_proxy);

    // 5) 버튼 업데이트
    updateButtons();

    // 6) 데이터 불러오기
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

void CustomTableWidget::updateButtons(){
    ui->prevButton->setEnabled(m_proxy->currentPage() > 0);
    ui->nextButton->setEnabled(
        m_proxy->currentPage()+1 < m_proxy->pageCount());
}

void CustomTableWidget::onNewData(const QStringList& fields) {
    // 1) 컬럼 수가 아직 설정되지 않았거나 부족하면 늘려준다
    int neededCols = 1 + fields.size();
    if (m_sourceModel->columnCount() < neededCols) {
        m_sourceModel->setColumnCount(neededCols);
    }

    // 2) QStringList → QStandardItem 리스트 변환
    QList<QStandardItem*> items;
    items.append(new QStandardItem());
    for (const auto& f : fields) {
        items.append(new QStandardItem(f));
    }

    // 3) 페이지 수 비교 후 행 추가
    int oldPageCount = m_proxy->pageCount();
    m_sourceModel->appendRow(items);
    int newPageCount = m_proxy->pageCount();

    // 4) 버튼 상태 갱신
    if (newPageCount != oldPageCount) {
        updateButtons();
    }
}

void CustomTableWidget::loadData(){
    (void)QtConcurrent::run([this](){
        // 서버에 요청
        sendFile(detectStr.c_str(), "DATA");

        // 1) 모델 초기화도 UI 스레드에서
        QMetaObject::invokeMethod(this, [this](){
            m_sourceModel->removeRows(0, m_sourceModel->rowCount());
            // (원하면) 첫 페이지로 돌아가기
            m_proxy->setCurrentPage(0);
            updateButtons();
        }, Qt::QueuedConnection);

        // 2) END 신호 올 때까지 읽어서 onNewData 호출
        while (1) {
            // 2-1) **이미지 먼저**: 크기(int) 읽기
            int fileSize = 0;
            int n = SSL_read(sock_fd, &fileSize, sizeof(fileSize));
            if (n <= 0) break;

            // END 신호용 특별 크기(예: –1) 처리
            if (fileSize == -1)
                break;

            // 2-2) 이미지 바이트 읽기
            QByteArray imgData;
            imgData.reserve(fileSize);
            char buf[4096];
            int  bytesRead = 0;
            while (bytesRead < fileSize) {
                int toRead = qMin(fileSize - bytesRead, (int)sizeof(buf));
                int r = SSL_read(sock_fd, buf, toRead);
                if (r <= 0) break;
                imgData.append(buf, r);
                bytesRead += r;
            }

            // 2-3) QPixmap으로 변환
            QPixmap pixmap;
            if (!pixmap.loadFromData(imgData)) {
                qWarning() << "이미지 로드 실패, size=" << fileSize;
                continue;
            }

            // 2-4) **텍스트 레코드** 읽기
            char textBuf[1024];
            int  tn = SSL_read(sock_fd, textBuf, sizeof(textBuf)-1);
            if (tn <= 0) break;
            textBuf[tn] = '\0';
            if (!strncmp(textBuf, "END", 3)) break;

            QStringList fields = QString::fromUtf8(textBuf)
                                     .trimmed()
                                     .split('|', Qt::SkipEmptyParts);

            // 4) UI 스레드에서 모델/뷰 갱신
            QMetaObject::invokeMethod(this, [this, fields, pixmap]() {
                // 4-1) 텍스트 기반 컬럼 추가
                onNewData(fields);

                // 4-2) 새로 추가된 행(마지막)의 0번 컬럼에 버튼 삽입
                int srcRow = m_sourceModel->rowCount() - 1;
                QModelIndex srcIdx   = m_sourceModel->index(srcRow, 0);
                QModelIndex proxyIdx = m_proxy->mapFromSource(srcIdx);
                if (!proxyIdx.isValid()) return;

                auto *btn = new ImagePushButton(this);
                btn->setFixedSize(80, 45);       // 16:9 비율 예시
                btn->setPixmap(pixmap);          // pixmap 직접 설정
                ui->eventTable->setIndexWidget(proxyIdx, btn);
            }, Qt::QueuedConnection);
        }
    });
}
