#ifndef DRAWMAP_H
#define DRAWMAP_H

#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include <QPointF>
#include <QMap>
#include <QColor>

// 맵 데이터 타입 (created_at 추가)
struct MapPoint {
    qint64 ts;
    double cx, cy, rx, ry, lx, ly;
    QString created_at;    // 7번 인덱스 (문자열)
    bool hasRight;         // 우측 객체 존재 여부
    bool hasLeft;          // 좌측 객체 존재 여부
};

// 바운드 이벤트 타입
struct BoundEvt {
    qint64 ts;
    QString cls;
    QString feat; // "x,y"
};

class DrawMap : public QObject {
    Q_OBJECT
public:
    explicit DrawMap(QGraphicsScene *scene, QObject *parent=nullptr);

    // ssl_read로 한 줄씩 받아서 전달
    void feedLine(const QByteArray &line);

    // 중심점 보간 위치 계산 (center_x, center_y), 없는 경우 양 끝 return
    QPointF interpolateAt(qint64 ts) const;

private:
    enum Section { None, MapSection, BoundsSection };
    Section section;
    QGraphicsScene *scene;
    QVector<MapPoint> mapData;
    QVector<BoundEvt> boundsData;

    void processMapLine(const QStringList &parts);
    void processBoundLine(const QStringList &parts);
    void drawMap();
    void drawBounds();
};

#endif // DRAWMAP_H