#ifndef MAPPINGUTILS_H
#define MAPPINGUTILS_H

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>  // KCF 트래커용 헤더 추가
#include <vector>

// 경로 및 방향 정보 구조체
struct TimedPoint {
    cv::Point pt;        // 중심 좌표
    long long t_ms;      // 타임스탬프(ms)
    cv::Point right_pt;  // 계산된 오른쪽 좌표
    cv::Point left_pt;   // 계산된 왼쪽 좌표
};

void mapping_init(int cam_index, int width, int height, int fps);

void mapping_set_detecting(bool active);
bool mapping_is_detecting();

// HSV 기반 객체 검출 함수 (KCF 초기화용)
cv::Point mapping_detect_object_yolo(const cv::Mat& frame, cv::Rect& detected_roi, bool* found = nullptr);

// 메인 객체 검출 함수 (KCF 기반)
cv::Point mapping_detect_object(const cv::Mat& frame, bool* found = nullptr);

void mapping_set_tracking(bool active);
bool mapping_is_tracking();
void mapping_add_point(const cv::Point& curr_center, long long t_ms);
const std::vector<TimedPoint>& mapping_get_timed_points();
void mapping_reset();
void save_timed_points_csv();

// KCF 트래커 상태 확인용 함수들 추가
bool mapping_is_tracker_initialized();
int mapping_get_track_fail_count();
cv::Rect mapping_get_current_roi();

#endif
