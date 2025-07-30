#include "drawMap.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QtDebug>
#include <algorithm>
#include <limits>

#define NOMINMAX
#undef min
#undef max


// ------------------ 클래스별 색상 테이블 ------------------
static QMap<QString, QColor> classColor = {
    { "Angular Leafspot",      QColor(85, 107, 47) },    // 진녹/올리브
    { "Anthracnose Fruit Rot", QColor(230, 126, 34) },   // 오렌지
    { "Blossom Blight",        QColor(238, 130, 238) },  // 연분홍
    { "Gray Mold",             QColor(128, 128, 128) },  // 회색
    { "Leaf Spot",             QColor(139, 69, 19) },    // 진갈색
    { "Powdery Mildew Fruit",  QColor(203, 195, 227) },  // 연보라
    { "Powdery Mildew Leaf",   QColor(144, 238, 144) },  // 연녹색
    { "ripe",                  QColor(220, 30, 70) },    // 진빨강
};

DrawMap::DrawMap(QGraphicsView *view_, QObject *parent)
    : QObject(parent), view(view_)
{
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 640, 480);
    scene->setBackgroundBrush(QBrush(QColor("#0c1919"))); // 어두운 남색 톤
    if (view)
        view->setScene(scene);
}

void DrawMap::feedLine(const QByteArray &lineRaw) {
    QString line = QString::fromUtf8(lineRaw).trimmed();

    if (line == "SPLIT") {
        section = BoundsSection;
        boundsData.clear();
        return;
    }
    if (line == "END") {
        // 큐에 등록: 데이터가 있을 때만 각 함수 실행
        QMetaObject::invokeMethod(this, [this] {
            if (!mapData.isEmpty())
                drawMap();
            if (!boundsData.isEmpty())
                drawBounds();
        }, Qt::QueuedConnection);
        section = None;
        return;
    }
    if (line.isEmpty()) return;

    // 섹션 초기값이 설정되어 있지 않으면 MAP 데이터로 간주
    if (section != BoundsSection)
        section = MapSection;

    if (section == MapSection) processMapLine(line.split('|'));
    else if (section == BoundsSection) processBoundLine(line.split('|'));
}


bool DrawMap::receiveMapData(SSL *ssl)
{
    if (!ssl) {
        qDebug() << "[DrawMap] SSL connection is null";
        return false;
    }

    mapData.clear();
    boundsData.clear();
    section = MapSection;

    QString recvBuf;
    char buffer[1024];
    bool finished = false;

    while (!finished) {
        int n = SSL_read(ssl, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            // 서버 종료 또는 오류
            break;
        }

        buffer[n] = '\0';
        recvBuf += QString::fromUtf8(buffer);

        int idx;
        // 한 번에 여러 줄이 들어올 수 있으니, '\n' 단위로 처리
        while ((idx = recvBuf.indexOf('\n')) >= 0) {
            QString line = recvBuf.left(idx).trimmed();
            recvBuf = recvBuf.mid(idx + 1);

            feedLine(line.toUtf8());

            if (line == QLatin1String("END")) {
                finished = true;
                break;  // 내부 while 탈출
            }
        }
    }

    // 남은 줄(END 이후 잔여 데이터)이 있을 경우 처리
    if (!recvBuf.trimmed().isEmpty() && !finished) {
        feedLine(recvBuf.toUtf8());
    }

    section = None;
    return !mapData.isEmpty();
}


void DrawMap::processMapLine(const QStringList &p) {
    if (p.size() != 7 && p.size() != 8) return;
    bool ok;
    MapPoint pt;
    pt.ts = p[0].toLongLong(&ok);  if (!ok) return;
    pt.cx = p[1].toDouble(&ok);    if (!ok) return;
    pt.cy = p[2].toDouble(&ok);    if (!ok) return;
    pt.rx = p[3].toDouble(&ok);    if (!ok) return;
    pt.ry = p[4].toDouble(&ok);    if (!ok) return;
    pt.lx = p[5].toDouble(&ok);    if (!ok) return;
    pt.ly = p[6].toDouble(&ok);    if (!ok) return;
    pt.createdAt = (p.size() == 8) ? p[7] : QString();

    mapData << pt;
}



void DrawMap::processBoundLine(const QStringList &p) {
    if (p.size() != 3) return;
    bool ok;
    BoundEvt e;
    e.ts = p[0].toLongLong(&ok); if (!ok) return;
    e.cls = p[1];
    e.feat = p[2];
    boundsData << e;
}

QPointF DrawMap::scale(double x, double y) const {
    constexpr double sceneW = 640, sceneH = 480;
    constexpr double pad = 50; // 상하좌우 모두에 동일하게 적용할 패딩
    const double effW = sceneW - 2 * pad;
    const double effH = sceneH - 2 * pad;

    double inW = maxX - minX;
    double inH = maxY - minY;

    // 실제 좌표계 폭/높이가 0에 가까우면 중앙에 찍기 (분모 방어)
    double sx = (inW > 1e-6) ? ((x - minX) / inW * effW) : effW / 2.0;
    double sy = (inH > 1e-6) ? ((y - minY) / inH * effH) : effH / 2.0;

    // 축소 스케일 추가 (예: 85%)
    constexpr double scaleFactor = 0.65;
    sx *= scaleFactor;
    sy *= scaleFactor;

    // 패딩만큼 평행이동
    return { sx + pad, sy + pad };
}

void DrawMap::drawBounds() {
    if (boundsData.isEmpty() || mapData.size() < 2) return;

    for (const auto &evt : boundsData) {
        // 1. 이벤트 ts에 해당하는 구간 탐색
        int i = 0;
        while (i < mapData.size() - 1 && evt.ts > mapData[i + 1].ts)
            ++i;
        if (i >= mapData.size() - 1) continue;

        const MapPoint &a = mapData[i];
        const MapPoint &b = mapData[i + 1];
        double ratio = (b.ts != a.ts) ? (evt.ts - a.ts) / double(b.ts - a.ts) : 0.0;

        double fx = 0, fy = 0;
        if (evt.feat == "0") { // 좌: 두 지점의 lx,ly를 보간
            fx = a.lx + (b.lx - a.lx) * ratio;
            fy = a.ly + (b.ly - a.ly) * ratio;
        } else if (evt.feat == "1") { // 우: 두 지점의 rx,ry를 보간
            fx = a.rx + (b.rx - a.rx) * ratio;
            fy = a.ry + (b.ry - a.ry) * ratio;
        } else {
            continue; // 혹시 feat 입력 잘못됐을 때 건너뜀
        }

        QPointF c = scale(fx, fy);
        QColor col = classColor.value(evt.cls, Qt::black);
        scene->addEllipse(c.x() - 7, c.y() - 7, 14, 14, QPen(Qt::black, 2), QBrush(col));
    }
}

void DrawMap::drawMap() {
    scene->clear();
    if (mapData.isEmpty()) return;

    // 1. 좌표 범위 계산
    minX = minY = std::numeric_limits<double>::max();
    maxX = maxY = -std::numeric_limits<double>::max();
    for (const auto &pt : mapData) {
        minX = std::min(minX, pt.cx);
        maxX = std::max(maxX, pt.cx);
        minY = std::min(minY, pt.cy);
        maxY = std::max(maxY, pt.cy);
    }
    if (minX == maxX) { minX -= 0.5; maxX += 0.5; }
    if (minY == maxY) { minY -= 0.5; maxY += 0.5; }

    // 2. (보간된) 경로선
    QPainterPath path;
    QPointF firstPt = scale(mapData[0].cx, mapData[0].cy);
    path.moveTo(firstPt);

    // 간단한 곡선 보간: quadTo(2차 베지어, 3점씩) - 짧으면 직선
    for (int i = 1; i < mapData.size() - 2; ++i) {
        QPointF p0 = scale(mapData[i].cx, mapData[i].cy);
        QPointF p1 = scale(mapData[i + 1].cx, mapData[i + 1].cy);
        QPointF mid = (p0 + p1) / 2;
        path.quadTo(p0, mid);
    }
    if (mapData.size() >= 2) {
        QPointF last = scale(mapData.last().cx, mapData.last().cy);
        path.lineTo(last); // 마지막 두점 연결
    }

    auto *pathItem = new QGraphicsPathItem(path);
    QPen pen(QColor("#00ffcc"));
    pen.setWidth(4);
    pen.setCapStyle(Qt::RoundCap);
    pathItem->setPen(pen);
    scene->addItem(pathItem);

    // 3. 시작/끝점 객체(Bold 마커+라벨)
    QPointF start = scale(mapData.first().cx, mapData.first().cy);
    QPointF end = scale(mapData.last().cx, mapData.last().cy);

    scene->addEllipse(start.x() - 10, start.y() - 10, 20, 20,
                      QPen(QColor("#00ffff"), 2), QBrush(QColor(0, 255, 255, 150)));
    QFont labelFont("Segoe UI", 12, QFont::Bold);
    auto *startTxt = scene->addText("START", labelFont);
    startTxt->setDefaultTextColor(QColor("#00ffff"));
    startTxt->setPos(start.x() - 24, start.y() - 28);

    scene->addEllipse(end.x() - 10, end.y() - 10, 20, 20,
                      QPen(QColor("#ff0066"), 2), QBrush(QColor(255, 0, 102, 150)));
    auto *endTxt = scene->addText("END", labelFont);
    endTxt->setDefaultTextColor(QColor("#ff0066"));
    endTxt->setPos(end.x() - 10, end.y() + 12);

    // 4. 범례(Bold, 작게, 좌상단)
    const int r = 8, lx = 6, ly = 6, spacing = 22;
    int idx = 0;
    QFont legendFont("Arial", 11, QFont::Bold);
    for (auto it = classColor.begin(); it != classColor.end(); ++it, ++idx) {
        QColor col = it.value();
        scene->addEllipse(lx, ly + idx * spacing, 2 * r, 2 * r,
                          QPen(Qt::black, 2), QBrush(col));
        auto *txt = scene->addText(it.key(), legendFont);
        txt->setDefaultTextColor(Qt::white);
        txt->setPos(lx + 2 * r + 8, ly + idx * spacing - 4);
    }

    // 5. 날짜(오른쪽 아래, yyyy-MM-dd만, Bold)
    QString timeText;
    const MapPoint& last = mapData.last();
    if (!last.createdAt.isEmpty()) {
        int spaceIdx = last.createdAt.indexOf(' ');
        timeText = (spaceIdx > 0) ? last.createdAt.left(spaceIdx) : last.createdAt;
    } else
        timeText = QString::number(last.ts);
    QFont dateFont("Consolas", 14, QFont::Bold);
    auto *tsItem = scene->addText(timeText, dateFont);
    tsItem->setDefaultTextColor(Qt::darkGray);

    constexpr double sceneW = 640, sceneH = 480;
    int charPixelW = 11, marginRight = 16, marginBottom = 16;
    double textX = sceneW - (timeText.size() * charPixelW) - marginRight;
    double textY = sceneH - marginBottom - 22;
    tsItem->setPos(textX, textY);

    // 6. (선택) 시작/끝점 좌표 콘솔 출력
    qDebug() << "[MAP] 시작점:" << mapData.first().cx << mapData.first().cy
             << "끝점:" << mapData.last().cx << mapData.last().cy;
}
