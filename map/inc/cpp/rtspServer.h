#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include <atomic>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <queue>
#include <map>
#include <mutex>
#include <condition_variable>

#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "mappingUtils.h"

using namespace cv;
using namespace std;

// 공유 프레임 버퍼 (thread-safe)
struct SharedFrameBuffer {
    Mat latest_frame;
    std::mutex frame_mutex;
    std::atomic<uint64_t> frame_seq{0};
    std::atomic<bool> frame_ready{false};
    
    // 최신 프레임 저장
    void update_frame(const Mat& frame) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        latest_frame = frame.clone();
        frame_seq++;
        frame_ready = true;
    }
    
    // 최신 프레임 복사
    bool get_frame(Mat& frame) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (!frame_ready || latest_frame.empty()) {
            return false;
        }
        frame = latest_frame.clone();
        return true;
    }
};

struct StreamContext {
    cv::VideoCapture *cap;
    int width, height, fps;
    guint64 frame_count;
    
    // 공유 데이터
    SharedFrameBuffer* frame_buffer;
};

// 전역 제어 변수
inline static std::atomic<bool> system_running{true};
inline static SharedFrameBuffer global_frame_buffer;
static bool detected_flag;
// 스레드 함수들
void capture_thread(StreamContext* ctx);
void mapping_thread(StreamContext* ctx);
void output_thread_handler(); // GStreamer 콜백에서 사용

// 기존 함수들
GstRTSPServer *setupRtspServer(StreamContext &ctx);
bool pushFrame(GstElement *appsrc, StreamContext &ctx);
void send_coordinate_event(const cv::Point& center,
                          const cv::Point& right,
                          const cv::Point& left,
                          long long t_ms);
#endif
