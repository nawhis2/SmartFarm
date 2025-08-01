#include "mappingUtils.h"
#include <chrono>
#include <opencv2/tracking.hpp>

static cv::dnn::Net yolo_net;
static bool yolo_initialized = false;
static float yolo_conf_threshold = 0.3f;
static float yolo_nms_threshold = 0.5f;
static int yolo_input_width = 640;
static int yolo_input_height = 640;

static std::string yolo_onnx_path = "mapping_model.onnx";

using namespace cv;
using namespace std;

// 내부 상태
static bool tracking_active = false;
static vector<TimedPoint> timed_points;
static cv::Point prev_center;
static bool has_prev = false;
static chrono::steady_clock::time_point t0;

// KCF 트래커 관련 추가 변수들
static Ptr<Tracker> tracker;
static Rect roi;
static bool tracker_initialized = false;
static int track_fail_count = 0;
static const int MAX_FAIL_COUNT = 5; // 연속 실패 허용 횟수

static bool detecting_active = false;

void mapping_set_detecting(bool active)
{
    detecting_active = active;
}

bool mapping_is_detecting()
{
    return detecting_active;
}

// 디버깅용 csv생성
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;

void save_timed_points_csv()
{
    fs::create_directories("./debug");
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream fname;
    fname << "./debug/track_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";

    std::ofstream fout(fname.str());
    fout << "timestamp_ms,center_x,center_y,right_x,right_y,left_x,left_y\n";

    for (const auto &p : timed_points)
    {
        fout << p.t_ms << ','
             << p.pt.x << ',' << p.pt.y << ','
             << p.right_pt.x << ',' << p.right_pt.y << ','
             << p.left_pt.x << ',' << p.left_pt.y << '\n';
    }
    fout.close();
}

// 디버깅 end
cv::Mat letterbox(const cv::Mat &src, cv::Size target_size, cv::Scalar pad_color, float *scale, cv::Point *pad)
{
    int src_w = src.cols, src_h = src.rows;
    float r = std::min(target_size.width / (float)src_w, target_size.height / (float)src_h);
    int new_unpad_w = int(round(src_w * r)), new_unpad_h = int(round(src_h * r));
    int dw = target_size.width - new_unpad_w, dh = target_size.height - new_unpad_h;
    dw /= 2;
    dh /= 2;
    cv::Mat resized;
    cv::resize(src, resized, cv::Size(new_unpad_w, new_unpad_h));
    cv::Mat output;
    cv::copyMakeBorder(resized, output, dh, dh, dw, dw, cv::BORDER_CONSTANT, pad_color);
    if (scale)
        *scale = r;
    if (pad)
        *pad = cv::Point(dw, dh);
    return output;
}

cv::Rect convertBoxFromLetterbox(const cv::Rect2f &box_in_letterbox, float scale, const cv::Point &pad, const cv::Size &original_size)
{
    float cx = box_in_letterbox.x + box_in_letterbox.width / 2.0f;
    float cy = box_in_letterbox.y + box_in_letterbox.height / 2.0f;
    float real_cx = (cx - pad.x) / scale;
    float real_cy = (cy - pad.y) / scale;
    float real_w = box_in_letterbox.width / scale;
    float real_h = box_in_letterbox.height / scale;
    int x = std::max(0, static_cast<int>(real_cx - real_w / 2.0f));
    int y = std::max(0, static_cast<int>(real_cy - real_h / 2.0f));
    int w = std::min(static_cast<int>(real_w), original_size.width - x);
    int h = std::min(static_cast<int>(real_h), original_size.height - y);
    return cv::Rect(x, y, w, h);
}

void mapping_init_yolo()
{
    if (!yolo_initialized)
    {
        yolo_net = cv::dnn::readNetFromONNX(yolo_onnx_path);
        if (yolo_net.empty())
        {
            std::cerr << "[YOLO] model load failed: " << yolo_onnx_path << std::endl;
            return;
        }
        yolo_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        yolo_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        yolo_initialized = true;
        std::cout << "[YOLO] ONNX loading complete" << std::endl;
    }
}

void mapping_init(int cam_index, int width, int height, int fps)
{
    mapping_init_yolo();
    tracking_active = false;
    tracker_initialized = false;
    track_fail_count = 0;
    timed_points.clear();
    has_prev = false;
    t0 = chrono::steady_clock::now();
    // KCF 트래커 초기화
    tracker = TrackerKCF::create();
}

void mapping_reset()
{
    timed_points.clear();
    has_prev = false;
    tracker_initialized = false;
    track_fail_count = 0;
    t0 = chrono::steady_clock::now();

    // 트래커 재생성
    tracker = TrackerKCF::create();
}

// KCF 기반 객체 추적
cv::Point mapping_detect_object(const Mat &frame, bool *found)
{

    // KCF 트래커가 초기화되지 않았거나 추적이 실패한 경우
    if (!tracker_initialized || track_fail_count >= MAX_FAIL_COUNT)
    {
        Rect detected_roi;
        Point yolo_center = mapping_detect_object_yolo(frame, detected_roi, found);

        if (*found)
        {
            // 객체를 찾았으므로 KCF 트래커를 초기화
            roi = detected_roi;
            tracker = TrackerKCF::create();
            tracker->init(frame, roi);
            tracker_initialized = true;
            track_fail_count = 0;

            return yolo_center;
        }
        else
        {
            if (found)
                *found = false;
            return Point(-1, -1);
        }
    }
    // KCF 트래커로 추적 시도
    bool tracking_ok = tracker->update(frame, roi);

    if (tracking_ok)
    {
        // 추적 성공
        track_fail_count = 0;
        Point center(roi.x + roi.width / 2, roi.y + roi.height / 2);
        if (found)
            *found = true;
        return center;
    }
    else
    {
        // 추적 실패
        track_fail_count++;

        // 즉시 재검출 시도
        Rect detected_roi;
        Point yolo_center = mapping_detect_object_yolo(frame, detected_roi, found);
        if (*found)
        {
            // 재검출 성공 - 트래커 재초기화
            roi = detected_roi;
            tracker = TrackerKCF::create();
            tracker->init(frame, roi);
            tracker_initialized = true;
            track_fail_count = 0;

            return yolo_center;
        }
        else
        {
            // 검출 실패
            if (found)
                *found = false;
            return Point(-1, -1);
        }
    }
}

cv::Point mapping_detect_object_yolo(const cv::Mat &frame,
                                     cv::Rect &detected_roi,
                                     bool *found)
{
    if (!yolo_initialized)
        mapping_init_yolo();
    if (!yolo_initialized)
    {
        if (found)
            *found = false;
        return {-1, -1};
    }

    /* ---------- 1. Letter-box 전처리 ---------- */
    float scale = 1.0f;
    cv::Point pad;
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    cv::Mat input = letterbox(rgb,
                              {yolo_input_width, yolo_input_height},
                              cv::Scalar(114, 114, 114),
                              &scale, &pad);

    cv::Mat blob = cv::dnn::blobFromImage(input, 1.0 / 255.0,
                                          {yolo_input_width, yolo_input_height},
                                          cv::Scalar(), false, false);
    yolo_net.setInput(blob);

    /* ---------- 2. 추론 ---------- */
    std::vector<cv::Mat> outs;
    yolo_net.forward(outs, yolo_net.getUnconnectedOutLayersNames());

    /* ---------- 3. 출력 텐서 해석 FIX ---------- */
    cv::Mat out = outs[0];
    if (out.dims != 3)
    {
        if (found)
            *found = false;
        return {-1, -1};
    }

    int rows = std::max(out.size[1], out.size[2]); // 항상 25200
    out = out.reshape(1, rows);                    // (25200,85)
    const int dims = out.cols;                     // 85

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> class_ids;

for (int i = 0; i < rows; ++i)
{
    const float *data = out.ptr<float>(i);
    float obj_conf = data[4];
    float cls_conf = data[5];
    float confidence = obj_conf * cls_conf;
    if (confidence < yolo_conf_threshold)
        continue;
    float cx = data[0], cy = data[1], w = data[2], h = data[3];
    cv::Rect2f lbox(cx - w / 2.0f, cy - h / 2.0f, w, h);
    cv::Rect box = convertBoxFromLetterbox(lbox, scale, pad, frame.size());

    boxes.push_back(box & cv::Rect(0, 0, frame.cols, frame.rows));
    confidences.push_back(confidence);
    class_ids.push_back(0); // 클래스 1개
}

    /* ---------- NMS ---------- */
    std::vector<int> nms_idx;
    cv::dnn::NMSBoxes(boxes, confidences,
                      yolo_conf_threshold, yolo_nms_threshold, nms_idx);

    if (nms_idx.empty())
    {
        if (found)
            *found = false;
        return {-1, -1};
    }

    int best = nms_idx[0];
    detected_roi = boxes[best];
    if (found)
        *found = true;

    return {detected_roi.x + detected_roi.width / 2,
            detected_roi.y + detected_roi.height / 2};
}

void mapping_set_tracking(bool active)
{
    tracking_active = active;
    if (!active)
    {
        // 추적 중단 시 트래커 상태 리셋
        tracker_initialized = false;
        track_fail_count = 0;
    }
}

bool mapping_is_tracking()
{
    return tracking_active;
}

void mapping_add_point(const Point &curr_center, long long t_ms)
{
    Point dir;
    if (has_prev)
    {
        dir = curr_center - prev_center;
        double len = norm(dir);
        if (len > 1.0)
        {
            dir.x = int(dir.x / len * 50);
            dir.y = int(dir.y / len * 50);
        }
        else
        {
            dir = Point(50, 0);
        }
    }
    else
    {
        dir = Point(50, 0);
    }
    Point perp(-dir.y, dir.x);
    Point right_pt = curr_center + perp;
    Point left_pt = curr_center - perp;

    timed_points.push_back({curr_center, t_ms, right_pt, left_pt});
    prev_center = curr_center;
    has_prev = true;
}

const vector<TimedPoint> &mapping_get_timed_points()
{
    return timed_points;
}

// KCF 트래커 상태 확인용 함수 (디버깅용)
bool mapping_is_tracker_initialized()
{
    return tracker_initialized;
}

int mapping_get_track_fail_count()
{
    return track_fail_count;
}

Rect mapping_get_current_roi()
{
    return roi;
}