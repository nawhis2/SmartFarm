#include <opencv2/opencv.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

/* CSV 한 줄을 담는 구조체 – mappingUtils.h 의 TimedPoint와 동일 */
struct TimedPoint {
    long long t_ms;
    Point     pt;
    Point     right_pt;
    Point     left_pt;
};

/* CSV 파일 로드 */
bool loadCSV(const string& path, vector<TimedPoint>& pts)
{
    ifstream fin(path);
    if (!fin.is_open()) {
        cerr << "파일 열기 실패: " << path << endl;
        return false;
    }
    string line;
    getline(fin, line);               // 헤더 스킵
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        TimedPoint tp;
        char comma;
        ss >> tp.t_ms >> comma
           >> tp.pt.x      >> comma >> tp.pt.y
           >> comma
           >> tp.right_pt.x>> comma >> tp.right_pt.y
           >> comma
           >> tp.left_pt.x >> comma >> tp.left_pt.y;
        pts.push_back(tp);
    }
    return !pts.empty();
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "사용법: " << argv[0] << " track_YYYYMMDD_HHMMSS.csv\n";
        return -1;
    }
    string csvPath = argv[1];
    vector<TimedPoint> points;
    if (!loadCSV(csvPath, points)) {
        cerr << "CSV 파싱 실패 또는 데이터 없음\n";
        return -1;
    }

    /* 캔버스 크기 계산 – 좌표 최대값 기준 여유 여백 50 px */
    int maxX = 0, maxY = 0;
    for (const auto& p : points) {
        maxX = max({maxX, p.pt.x, p.right_pt.x, p.left_pt.x});
        maxY = max({maxY, p.pt.y, p.right_pt.y, p.left_pt.y});
    }
    const int margin = 50;
    Size canvasSize(maxX + margin, maxY + margin);
    Mat canvas(canvasSize, CV_8UC3, Scalar::all(30));          // 어두운 배경

    /* 경로 및 포인트 그리기 */
    for (size_t i = 1; i < points.size(); ++i)
        line(canvas, points[i-1].pt, points[i].pt, Scalar(0, 200, 255), 2); // 주황 경로

    for (const auto& p : points) {
        circle(canvas, p.pt,        4, Scalar(0,255,0),  -1); // 중심(녹색)
        circle(canvas, p.right_pt,  3, Scalar(255,0,255), 1); // 오른쪽(핑크)
        circle(canvas, p.left_pt,   3, Scalar(255,255,0),1);  // 왼쪽(하늘)
    }

    /* 윈도우 출력 */
    const string win = "Trajectory Map";
    imshow(win, canvas);
    cout << "s : PNG 저장 |  ESC / q : 종료\n";
    while (true) {
        int key = waitKey(0) & 0xFF;
        if (key == 's') {
            string out = csvPath.substr(0, csvPath.find_last_of('.')) + ".png";
            imwrite(out, canvas);
            cout << "저장 완료: " << out << endl;
        } else if (key == 27 || key == 'q')
            break;
    }
    destroyAllWindows();
    return 0;
}