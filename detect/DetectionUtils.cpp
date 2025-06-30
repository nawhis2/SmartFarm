#include "DetectionUtils.h"

std::vector<DetectionResult> runDetection(
    cv::dnn::Net& net,
    const cv::Mat& frame,
    float conf_threshold,
    float nms_threshold,
    const cv::Size& model_size
) {
    cv::Mat resized;
    cv::resize(frame, resized, model_size);

    cv::Mat rgb_frame;
    cv::cvtColor(resized, rgb_frame, cv::COLOR_BGR2RGB);

    cv::Mat blob = cv::dnn::blobFromImage(rgb_frame, 1.0/255.0, model_size, cv::Scalar(), false, false);
    net.setInput(blob);

    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    std::vector<DetectionResult> detections;
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    float scale_x = static_cast<float>(frame.cols) / model_size.width;
    float scale_y = static_cast<float>(frame.rows) / model_size.height;

    if (!outputs.empty()) {
        cv::Mat det = outputs[0].reshape(1, outputs[0].size[1]);
        for (int i = 0; i < det.rows; ++i) {
            const float* data = det.ptr<float>(i);
            float obj_conf = data[4];
            int num_classes = det.cols - 5;
            const float* scores = data + 5;

            auto max_score_iter = std::max_element(scores, scores + num_classes);
            float class_conf = *max_score_iter;
            int class_id = std::distance(scores, max_score_iter);
            float confidence = obj_conf * class_conf;

            if (confidence > conf_threshold) {
                float cx = data[0];
                float cy = data[1];
                float w  = data[2];
                float h  = data[3];

                int left   = std::max(0, static_cast<int>((cx - w/2) * scale_x));
                int top    = std::max(0, static_cast<int>((cy - h/2) * scale_y));
                int width  = static_cast<int>(w * scale_x);
                int height = static_cast<int>(h * scale_y);

                left = std::min(left, frame.cols - 1);
                top = std::min(top, frame.rows - 1);
                width = std::min(width, frame.cols - left);
                height = std::min(height, frame.rows - top);

                class_ids.push_back(class_id);
                confidences.push_back(confidence);
                boxes.emplace_back(left, top, width, height);
            }
        }
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);

    for (int idx : indices) {
        detections.push_back({class_ids[idx], confidences[idx], boxes[idx]});
    }
    return detections;
}

void drawDetections(
    cv::Mat& frame,
    const std::vector<DetectionResult>& detections,
    const std::vector<std::string>& class_names
) {
    for (const auto& det : detections) {
        cv::rectangle(frame, det.box, cv::Scalar(0,255,0), 2);
        std::string label = class_names[det.class_id] + ": " + cv::format("%.2f", det.confidence);
        cv::putText(frame, label, cv::Point(det.box.x, det.box.y - 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,255), 2);
    }
}
