#ifndef DRAWMAP_H
#define DRAWMAP_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QString>
#include <QDebug>
#include <QPainterPath>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <openssl/ssl.h>

// 맵 데이터 타입
struct MapPoint {
    qint64 ts;
    double cx, cy, rx, ry, lx, ly;
    QString createdAt;
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
    explicit DrawMap(QGraphicsView *view, QObject *parent = nullptr);

    void feedLine(const QByteArray &line);
    bool receiveMapData(SSL *ssl);

private:
    enum Section { None, MapSection, BoundsSection };
    Section section = None;

    QGraphicsView  *view   = nullptr;
    QGraphicsScene *scene  = nullptr;
    QVector<MapPoint> mapData;
    QVector<BoundEvt> boundsData;

    void processMapLine  (const QStringList &parts);
    void processBoundLine(const QStringList &parts);
    void drawMap();
    void drawBounds();
    QPointF scale(double x,double y) const;

    double minX=0,maxX=0,minY=0,maxY=0;
};

#endif // DRAWMAP_H
