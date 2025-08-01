#ifndef DETECTION_UTILS_H
#define DETECTION_UTILS_H

#include <opencv2/opencv.hpp>
#include <map>

struct DetectionResult {
    int class_id;              // 항상 0으로 고정 (motion 기반이라 class 없음)
    float confidence;          // 항상 1.0
    cv::Rect box;
};

void initMotionDetector();  // MOG2 초기화
std::vector<DetectionResult> detectMotion(const cv::Mat& frame);
void runTracking(const std::vector<DetectionResult>& detections,
                 std::map<int, DetectionResult>& tracked,
                 std::map<int, int>& appearCount,
                 int& next_id);

#endif
