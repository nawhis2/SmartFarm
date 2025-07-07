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

#endif