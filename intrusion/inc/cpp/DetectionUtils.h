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

cv::Mat letterbox(const cv::Mat &src, cv::Size target_size,
                  cv::Scalar pad_color = cv::Scalar(114,114,114),
                  float *scale = nullptr, cv::Point *pad = nullptr);

std::vector<DetectionResult> runDetection(
    cv::dnn::Net& net,
    const cv::Mat& frame,
    float conf_threshold,
    float nms_threshold,
    const cv::Size& model_size
);

cv::Rect convertBoxFromLetterbox(const cv::Rect2f& box_in_letterbox, float scale, const cv::Point& pad, const cv::Size& original_size);
