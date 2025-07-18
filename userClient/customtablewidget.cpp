#include "CustomTableWidget.h"
#include "ui_CustomTableWidget.h"
#include "paginationproxymodel.h"
#include "clientUtil.h"
#include "network.h"
#include "imagedialog.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QLabel>
#include <QPainter>

// Delegate that fills the first row cell (spanned across columns) with the pixmap
class FullRowImageDelegate : public QStyledItemDelegate {
public:
    FullRowImageDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}
    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        if (index.row() == 0 && index.column() == 0) {
            QVariant var = index.data(Qt::DecorationRole);
            if (var.canConvert<QPixmap>()) {
                QPixmap pm = var.value<QPixmap>();
                if (!pm.isNull()) {
                    QRect rect = option.rect;
                    pm = pm.scaled(rect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                    painter->drawPixmap(rect, pm);
                    return;
                }
            }
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
};

CustomTableWidget::CustomTableWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomTableWidget)
    , m_sourceModel(nullptr)
    , m_proxy(nullptr)
{
    ui->setupUi(this);
    ui->eventTable->verticalHeader()->hide();
    ui->eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->eventTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->eventTable->setItemDelegateForColumn(0, new FullRowImageDelegate(this));

    ui->eventTable->setIconSize(QSize(80,45));
    ui->eventTable->verticalHeader()->setDefaultSectionSize(45);
    ui->eventTable->setColumnWidth(0, 80);
}

CustomTableWidget::~CustomTableWidget() {
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
            QStringList() << "Image" << "Num" << "Eventname" << "Date"
            );

        delete m_proxy;
        m_proxy = new PaginationProxyModel(this);
        m_proxy->setSourceModel(m_sourceModel);
        m_proxy->setPageSize(10);
        connect(m_proxy, &PaginationProxyModel::layoutChanged,
                this, &CustomTableWidget::refreshPage);

        ui->eventTable->setModel(m_proxy);
        updateButtons();

        // Span first row across all columns
        int cols = m_sourceModel->columnCount();
        ui->eventTable->setSpan(0, 0, 1, cols);
    }, Qt::QueuedConnection);

    // Directly load data (no QtConcurrent)
    loadData();
}

void CustomTableWidget::on_prevButton_clicked() {
    int pg = m_proxy->currentPage();
    if (pg > 0) {
        m_proxy->setCurrentPage(pg - 1);
    }
    updateButtons();
}

void CustomTableWidget::on_nextButton_clicked() {
    int pg = m_proxy->currentPage();
    if (pg + 1 < m_proxy->pageCount()) {
        m_proxy->setCurrentPage(pg + 1);
    }
    updateButtons();
}

void CustomTableWidget::updateButtons() {
    ui->prevButton->setEnabled(m_proxy->currentPage() > 0);
    ui->nextButton->setEnabled(m_proxy->currentPage() + 1 < m_proxy->pageCount());
    refreshPage();
}

void CustomTableWidget::onNewData(const QStringList& fields) {
    int oldPageCount = m_proxy ? m_proxy->pageCount() : 0;
    QList<QStandardItem*> items;
    items.append(new QStandardItem());
    for (const QString& f : fields) {
        items.append(new QStandardItem(f));
    }
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

        char textBuf[1024];
        int tn = SSL_read(sock_fd, textBuf, sizeof(textBuf)-1);
        if (tn <= 0) break;
        textBuf[tn] = '\0';
        if (!strncmp(textBuf, "END", 3)) break;

        QStringList fields = QString::fromUtf8(textBuf)
                                 .trimmed()
                                 .split('|', Qt::SkipEmptyParts);

        QMetaObject::invokeMethod(this, [this, fields, pix]() {
            onNewData(fields);
            m_pixmaps.append(pix);
            refreshPage();
        }, Qt::QueuedConnection);
    }
}

void CustomTableWidget::refreshPage() {
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        QModelIndex pIdx = m_proxy->index(r, 0);
        QModelIndex sIdx = m_proxy->mapToSource(pIdx);
        int src = sIdx.row();
        if (!pIdx.isValid() || src < 0 || src >= m_pixmaps.size())
            continue;
        m_sourceModel->setData(
            m_sourceModel->index(src, 0),
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
    qDebug() << "Clicked row:" << viewRow << "pixmap size:" << pm.size();
    // e.g. display in QLabel
    // ui->previewLabel->setPixmap(pm);

    ImageDialog* dlg = new ImageDialog(this);
    dlg->setImg(pm);
    dlg->show();
}
