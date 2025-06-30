#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>

struct DetectionResult {
    int class_id;
    float confidence;
    cv::Rect box;
};

std::vector<DetectionResult> runDetection(
    cv::dnn::Net& net,
    const cv::Mat& frame,
    float conf_threshold = 0.4,
    float nms_threshold = 0.3,
    const cv::Size& model_size = cv::Size(416, 416)
);

void drawDetections(
    cv::Mat& frame,
    const std::vector<DetectionResult>& detections,
    const std::vector<std::string>& class_names
);
