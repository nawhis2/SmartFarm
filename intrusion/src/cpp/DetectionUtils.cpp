#include "DetectionUtils.h"
#include "rtspServer.h"

// letterbox 좌표 → 원본 프레임 좌표로 변환하는 함수
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

cv::Mat letterbox(const cv::Mat &src, cv::Size target_size,
                  cv::Scalar pad_color, float *scale, cv::Point *pad)
{
    int src_w = src.cols;
    int src_h = src.rows;
    float r = std::min((float)target_size.width / src_w, (float)target_size.height / src_h);

    int new_unpad_w = int(round(src_w * r));
    int new_unpad_h = int(round(src_h * r));

    int dw = target_size.width - new_unpad_w;
    int dh = target_size.height - new_unpad_h;

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

// 감지 함수: letterbox → YOLO → 원본 좌표 복원
std::vector<DetectionResult> runDetection(
    cv::dnn::Net &net,
    const cv::Mat &frame,
    float conf_threshold,
    float nms_threshold,
    const cv::Size &model_size)
{
    float scale;
    cv::Point pad;
    const std::vector<std::string>& allowed_classes = {"person", "Hog"};
    // 1. Letterbox 전처리
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    cv::Mat input = letterbox(rgb, model_size, cv::Scalar(114, 114, 114), &scale, &pad);
    cv::Mat blob = cv::dnn::blobFromImage(input, 1.0 / 255.0, model_size, cv::Scalar(), false, false);

    // 2. YOLO 추론
    net.setInput(blob);
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    std::vector<DetectionResult> detections;
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    if (!outputs.empty())
    {
        cv::Mat det = outputs[0].reshape(1, outputs[0].size[1]);
        for (int i = 0; i < det.rows; ++i)
        {
            const float *data = det.ptr<float>(i);
            float obj_conf = data[4];
            int num_classes = det.cols - 5;
            const float *scores = data + 5;
            auto max_score_iter = std::max_element(scores, scores + num_classes);
            float class_conf = *max_score_iter;
            int class_id = std::distance(scores, max_score_iter);
            float confidence = obj_conf * class_conf;

            if (confidence > conf_threshold)
            {
                const std::string &cls_name = class_names[class_id];
                if (std::find(allowed_classes.begin(), allowed_classes.end(), cls_name) == allowed_classes.end())
                    continue;

                float cx = data[0];
                float cy = data[1];
                float w = data[2];
                float h = data[3];

                // letterbox 기준 박스를 원본 프레임 좌표로 복원
                cv::Rect2f letterbox_box(cx - w / 2, cy - h / 2, w, h);
                cv::Rect orig_box = convertBoxFromLetterbox(letterbox_box, scale, pad, frame.size());

                class_ids.push_back(class_id);
                confidences.push_back(confidence);
                boxes.push_back(orig_box);
            }
        }
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, conf_threshold, nms_threshold, indices);

    for (int idx : indices)
    {
        detections.push_back({class_ids[idx], confidences[idx], boxes[idx]});
    }

    return detections;
}