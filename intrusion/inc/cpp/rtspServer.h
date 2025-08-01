#ifndef RTSPSERVER_H
#define RTSPSERVER_H
#include <atomic>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
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

#include <deque>

// 박스 없는 원본 프레임 (감지용)
inline static cv::Mat latest_raw_frame;
inline static std::mutex raw_mutex;

// tracked 보호용
inline static std::mutex track_mutex;

inline static Mat latest_frame;
inline static guint64 latest_pts = 0;
inline static std::mutex frame_mutex;

inline static atomic<bool> running(true);

struct StreamContext {
    int width;
    int height;
    int fps;
    int frame_count = 0;
    cv::VideoCapture* cap = nullptr;
    std::chrono::steady_clock::time_point last_snapshot_time = std::chrono::steady_clock::now();
};

GstRTSPServer *setupRtspServer(StreamContext &ctx);

void inferenceLoop(StreamContext* ctx);
void detectionLoop(StreamContext* ctx);

#endif