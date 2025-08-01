#ifndef RTSPSERVER_H
#define RTSPSERVER_H
#include <atomic>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <queue>
#include <map>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "DetectionUtils.h"

using namespace cv;
using namespace std;
using namespace std::chrono;

// 지연 버퍼 관련 전역 변수
inline static std::deque<std::pair<guint64, cv::Mat>> frame_buffer;
inline static const int BUFFER_DELAY_FRAMES = 9;
inline static std::mutex buffer_mutex;

// 박스 없는 원본 프레임 (감지용)
inline static cv::Mat latest_raw_frame;
inline static std::mutex raw_mutex;
// tracked 보호용
inline static std::mutex track_mutex;

inline static Mat latest_frame;
inline static guint64 latest_pts = 0;
inline static std::mutex frame_mutex;

inline static atomic<bool> running(true);

extern const vector<string> class_names;

struct StreamContext
{
    VideoCapture *cap;
    dnn::Net *net;
    vector<string> *class_names;
    int width, height, fps;
    guint64 frame_count;
    int frame_counter;
    vector<DetectionResult> last_detections;
    map<int, DetectionResult> tracked;
    int next_id;
    Mat background_model;
    steady_clock::time_point last_snapshot_time = steady_clock::now() - seconds(60);
};

GstRTSPServer *setupRtspServer(StreamContext &ctx);

void inferenceLoop(StreamContext* ctx);
void detectionLoop(StreamContext* ctx);

#endif