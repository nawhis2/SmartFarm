#include "DetectionUtils.h"

using namespace cv;
using namespace std;

static Ptr<BackgroundSubtractorMOG2> bgsub;

void initMotionDetector() {
    bgsub = createBackgroundSubtractorMOG2(500, 16, false);
    bgsub->setDetectShadows(false);
}

std::vector<DetectionResult> detectMotion(const Mat& frame) {
    static cv::Mat prev_frame_gray;

    static const float MOTION_THRESHOLD_RATIO = 0.01;
    static const int MIN_CONTOUR_AREA = 5000;
    static const float MAX_AREA_RATIO = 0.6;
    static const int WARMUP_FRAMES = 30;
    static const double CAMERA_MOVE_THRESHOLD = 10.0; // 평균 차이 임계값
    static int frame_counter = 0;

    std::vector<DetectionResult> results;

    if (!bgsub) {
        initMotionDetector();
    }

    // 1. 카메라 움직임 탐지용 그레이 프레임
    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    if (!prev_frame_gray.empty()) {
        Mat diff;
        absdiff(prev_frame_gray, gray, diff);
        Scalar mean_diff = mean(diff);
        if (mean_diff[0] > CAMERA_MOVE_THRESHOLD) {
            // 큰 변화 → 카메라 이동 → 감지 생략
            prev_frame_gray = gray.clone();
            return results;
        }
    }
    prev_frame_gray = gray.clone();  // 업데이트

    // 2. MOG2 적용
    Mat fgmask, kernel;
    bgsub->apply(frame, fgmask);

    kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    morphologyEx(fgmask, fgmask, MORPH_OPEN, kernel);

    if (++frame_counter < WARMUP_FRAMES)
        return results;

    double motionRatio = countNonZero(fgmask) / (double)(fgmask.rows * fgmask.cols);
    if (motionRatio < MOTION_THRESHOLD_RATIO)
        return results;

    vector<vector<Point>> contours;
    findContours(fgmask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (const auto& c : contours) {
        double area = contourArea(c);
        if (area < MIN_CONTOUR_AREA) continue;

        Rect r = boundingRect(c);
        float aspect = (float)r.width / r.height;
        if (aspect < 0.2 || aspect > 5.0) continue;
        if ((float)r.area() / area > 2.0f) continue;
        if (r.width > frame.cols * MAX_AREA_RATIO || r.height > frame.rows * MAX_AREA_RATIO)
            continue;

        results.push_back({0, 1.0f, r});
    }

    return results;
}

void runTracking(const std::vector<DetectionResult>& detections,
                 std::map<int, DetectionResult>& tracked,
                 std::map<int, int>& appearCount,
                 int& next_id)
{
    std::map<int, DetectionResult> updated;

    for (const auto& d : detections) {
        bool matched = false;
        for (const auto& [id, prev] : tracked) {
            float iou = (float)(d.box & prev.box).area() / (float)(d.box | prev.box).area();
            if (iou > 0.3f) {
                updated[id] = d;
                appearCount[id]++;
                matched = true;
                break;
            }
        }
        if (!matched) {
            updated[next_id] = d;
            appearCount[next_id] = 1;
            next_id++;
        }
    }

    tracked = std::move(updated);
}
