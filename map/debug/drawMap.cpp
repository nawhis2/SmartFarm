#include "drawMap.h"
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QStringList>

// 클래스별 색상
static const QMap<QString, QColor> classColor = {
    { "Angular Leafspot",         QColor(255, 99, 132) },
    { "Anthracnose Fruit Rot",    QColor(255, 159, 64) },
    { "Blossom Blight",           QColor(255, 205, 86) },
    { "Gray Mold",                QColor(201, 203, 207) },
    { "Leaf Spot",                QColor(54, 162, 235) },
    { "Powdery Mildew Fruit",     QColor(153, 102, 255) },
    { "Powdery Mildew Leaf",      QColor(75, 192, 192) },
    { "ripe",                     QColor(0, 128, 0) }
};

DrawMap::DrawMap(QGraphicsScene *scene_, QObject *parent)
    : QObject(parent), section(None), scene(scene_) {}

void DrawMap::feedLine(const QByteArray &lineRaw) {
    QString line = QString::fromUtf8(lineRaw).trimmed();
    if (line == "MAP")      { section = MapSection; mapData.clear(); return; }
    if (line == "BOUNDS")   { section = BoundsSection; boundsData.clear(); return; }
    if (line == "END") {
        if (section == MapSection)      drawMap();
        else if (section == BoundsSection) drawBounds();
        section = None;
        return;
    }
    if (line.isEmpty()) return;

    // 데이터 처리 (created_at 대응)
    if (section == MapSection)
        processMapLine(line.split('|'));
    else if (section == BoundsSection)
        processBoundLine(line.split('|'));
}

void DrawMap::processMapLine(const QStringList &parts) {
    // 맵 포맷: ts|cx|cy|rx|ry|lx|ly|created_at
    if (parts.size() < 8) return;
    bool ok;
    MapPoint pt;
    pt.ts = parts[0].toLongLong(&ok);  if (!ok) return;
    pt.cx = parts[1].toDouble(&ok);    if (!ok) return;
    pt.cy = parts[2].toDouble(&ok);    if (!ok) return;
    pt.rx = parts[3].toDouble(&ok);    if (!ok) return;
    pt.ry = parts[4].toDouble(&ok);    if (!ok) return;
    pt.lx = parts[5].toDouble(&ok);    if (!ok) return;
    pt.ly = parts[6].toDouble(&ok);    if (!ok) return;
    pt.created_at = parts[7];

    // 객체 존재 여부 판정: rx,ry/lx,ly 가 유의미하면 true (라이브러리/서버 상황에 맞게 조건 조정)
    pt.hasRight = !(parts[3].isEmpty() || parts[3] == "0") &&
                  !(parts[4].isEmpty() || parts[4] == "0");
    pt.hasLeft  = !(parts[5].isEmpty() || parts[5] == "0") &&
                  !(parts[6].isEmpty() || parts[6] == "0");

    mapData << pt;
}

void DrawMap::processBoundLine(const QStringList &parts) {
    // 바운드: ts|class|feature ("x,y")
    if (parts.size() != 3) return;
    bool ok;
    BoundEvt evt;
    evt.ts   = parts[0].toLongLong(&ok); if (!ok) return;
    evt.cls  = parts[1];
    evt.feat = parts[2];
    boundsData << evt;
}

void DrawMap::drawMap() {
    scene->clear();
    if (mapData.isEmpty()) return;

    // 중심 polyline
    QPainterPath path;
    path.moveTo(mapData[0].cx, mapData[0].cy);
    for (int i = 1; i < mapData.size(); ++i)
        path.lineTo(mapData[i].cx, mapData[i].cy);
    auto *pathItem = new QGraphicsPathItem(path);
    pathItem->setPen(QPen(Qt::blue, 2));
    scene->addItem(pathItem);

    // 중심좌표 라벨 (created_at), 그리고 우/좌 객체 존재 시 점 표시
    for (const auto &pt : mapData) {
        // created_at 라벨 표시
        if (!pt.created_at.isEmpty()) {
            auto *label = scene->addText(pt.created_at, QFont("Arial", 8));
            label->setDefaultTextColor(Qt::black);
            label->setPos(pt.cx + 6, pt.cy - 16);
        }
        if (pt.hasRight)
            scene->addEllipse(pt.rx-3, pt.ry-3, 6, 6, QPen(Qt::red), QBrush(Qt::red));
        if (pt.hasLeft)
            scene->addEllipse(pt.lx-3, pt.ly-3, 6, 6, QPen(Qt::green), QBrush(Qt::green));
    }
}

void DrawMap::drawBounds() {
    if (boundsData.isEmpty()) return;

    auto parsePt = [](const QString &f) {
        auto xy = f.split(',');
        if (xy.size() < 2) return QPointF();
        return QPointF(xy[0].toDouble(), xy[1].toDouble());
    };

    auto drawCircle = [&](const QPointF &p, const QColor &c) {
        scene->addEllipse(p.x()-10, p.y()-10, 20, 20, QPen(c,2), Qt::NoBrush);
    };

    for (const auto &evt : boundsData) {
        QPointF pt = parsePt(evt.feat);
        QColor  col = classColor.value(evt.cls, Qt::black);
        drawCircle(pt, col);
    }
}

QPointF DrawMap::interpolateAt(qint64 ts) const {
    if (mapData.isEmpty()) return QPointF();
    int idx = -1;
    for (int i = 0; i < mapData.size(); ++i)
        if (mapData[i].ts >= ts) { idx = i; break; }
    if (idx <= 0) return QPointF(mapData.first().cx, mapData.first().cy);
    if (idx == -1) return QPointF(mapData.last().cx, mapData.last().cy);

    const auto &p1 = mapData[idx-1], &p2 = mapData[idx];
    double ratio = double(ts - p1.ts) / double(p2.ts - p1.ts);
    double x = p1.cx + (p2.cx - p1.cx) * ratio;
    double y = p1.cy + (p2.cy - p1.cy) * ratio;
    return QPointF(x, y);
}