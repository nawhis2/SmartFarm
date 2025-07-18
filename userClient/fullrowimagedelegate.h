#ifndef FULLROWIMAGEDELEGATE_H
#define FULLROWIMAGEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

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

#endif // FULLROWIMAGEDELEGATE_H
