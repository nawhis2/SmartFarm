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
        if (index.column() == 3) {  // 이미지 열만 처리
            QVariant var = index.data(Qt::DecorationRole);
            if (var.canConvert<QPixmap>()) {
                QPixmap pm = var.value<QPixmap>();
                if (!pm.isNull()) {
                    QRect rect = option.rect;
                    pm = pm.scaled(rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    int x = rect.x() + (rect.width() - pm.width()) / 2;
                    int y = rect.y() + (rect.height() - pm.height()) / 2;
                    painter->drawPixmap(x, y, pm);
                    return;
                }
            }
        }
        QStyledItemDelegate::paint(painter, option, index);
    }

};

#endif // FULLROWIMAGEDELEGATE_H
