#include "rtspServer.h"
#include "jsonlParse.h" 

const vector<string> class_names = {"fire", "smoke"};
extern bool ledState; // LED 상태

// ----------------------------
// Processing functions
// ----------------------------
Mat updateBackground(const Mat &frame, Mat &background, double alpha)
{
    Mat f32;
    frame.convertTo(f32, CV_32F);
    accumulateWeighted(f32, background, alpha);
    Mat bg8u;
    background.convertTo(bg8u, CV_8U);
    return bg8u;
}

Mat getMotionMask(const Mat &frame, const Mat &bg8u, double diffThresh = 30.0, int blurSize = 15)
{
    Mat diff, gray, fg;
    absdiff(frame, bg8u, diff);
    cvtColor(diff, gray, COLOR_BGR2GRAY);
    threshold(gray, fg, diffThresh, 255, THRESH_BINARY);
    GaussianBlur(fg, fg, Size(blurSize, blurSize), 0);
    threshold(fg, fg, diffThresh, 255, THRESH_BINARY);
    return fg;
}

vector<Rect> extractMotionBoxes(const Mat &fgmask, double minArea = 500.0)
{
    vector<vector<Point>> contours;
    findContours(fgmask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<Rect> boxes;
    for (auto &c : contours)
    {
        if (contourArea(c) < minArea)
            continue;
        boxes.push_back(boundingRect(c));
    }
    return boxes;
}

float calculateIoU(const Rect &a, const Rect &b)
{
    int inter = (a & b).area();
    int uni = a.area() + b.area() - inter;
    return uni > 0 ? static_cast<float>(inter) / uni : 0.0f;
}

bool pushFrame(GstElement *appsrc, StreamContext &ctx)
{
    Mat frame;
    guint64 pts;

    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (latest_frame.empty())
        {
            std::cout << "[RTSP] appsrc에 푸쉬 안됨\n";
            return false;
        }

        frame = latest_frame.clone();
        pts = latest_pts;
    }

    // Push to GStreamer appsrc
    int size = frame.total() * frame.elemSize();
    GstBuffer *buf = gst_buffer_new_and_alloc(size);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_WRITE);
    memcpy(info.data, frame.data, size);
    gst_buffer_unmap(buf, &info);

    GST_BUFFER_PTS(buf) = gst_util_uint64_scale(pts, GST_SECOND, ctx.fps);
    GST_BUFFER_DURATION(buf) = gst_util_uint64_scale_int(1, GST_SECOND, ctx.fps);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf);
    if (ret != GST_FLOW_OK) {
        cerr << "Failed to push buffer to appsrc!" << endl;
        return false;
    }

    return true;
}

static void onNeedData(GstElement *appsrc, guint, gpointer user_data)
{
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);
    if (!running)
    {
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    }
    else
    {
        pushFrame(appsrc, *ctx);
    }
}

bool push_frame_to_appsrc(GstElement *appsrc, const cv::Mat &frame, int fps = 30) {
    // 1. 프레임 유효성 검사
    if (frame.empty()) {
        std::cerr << "[push_frame_to_appsrc] 빈 프레임입니다." << std::endl;
        return false;
    }

    // 2. GStreamer 버퍼 생성
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, frame.total() * frame.elemSize(), nullptr);
    if (!buffer) {
        std::cerr << "[push_frame_to_appsrc] GstBuffer 생성 실패" << std::endl;
        return false;
    }

    // 3. 버퍼에 데이터 복사
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::cerr << "[push_frame_to_appsrc] 버퍼 매핑 실패" << std::endl;
        gst_buffer_unref(buffer);
        return false;
    }
    memcpy(map.data, frame.data, map.size);
    gst_buffer_unmap(buffer, &map);

    // 4. 타임스탬프 설정 (appsrc가 is-live=true, do-timestamp=true일 경우)
    static GstClockTime pts = 0;
    GST_BUFFER_PTS(buffer) = pts;
    GST_BUFFER_DTS(buffer) = pts;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, fps);
    pts += GST_BUFFER_DURATION(buffer);

    // 5. appsrc에 푸시
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        std::cerr << "[push_frame_to_appsrc] 버퍼 푸시 실패: " << gst_flow_get_name(ret) << std::endl;
        return false;
    }

    return true;
}

static void push_dummy(GstElement* appsrc, StreamContext *ctx)
{
    // Dummy function to ensure appsrc is ready
    // This can be used to push an initial frame if needed
    std::cout << "[RTSP] Dummy push called" << std::endl;

    Mat dummy_frame(ctx->height, ctx->width, CV_8UC3, Scalar(255,255,255));
    Mat dummy_frameRGB;
    cvtColor(dummy_frame, dummy_frameRGB, COLOR_BGR2RGB);

    for (int i = 0; i < 5; i++)
    {
        if (!push_frame_to_appsrc(appsrc, dummy_frameRGB))
        {
            std::cerr << "[RTSP] Dummy push failed" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    
}

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
{
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);

    // 상태 초기화
    // gst_element_set_state(appsrc, GST_STATE_READY);
    // gst_element_set_state(appsrc, GST_STATE_PLAYING);

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, ctx->width,
        "height", G_TYPE_INT, ctx->height,
        "framerate", GST_TYPE_FRACTION, ctx->fps, 1,
        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    push_dummy(appsrc, ctx);

    ctx->frame_count = 0;
    running = true;

    g_signal_connect(appsrc, "need-data", G_CALLBACK(onNeedData), ctx);
    gst_object_unref(appsrc);
    gst_object_unref(pipeline);
}

long calculateBitrate(int width,
                      int height,
                      int fps,
                      double bitsPerPixel = 0.1)
{
    // 초당 총 픽셀 수 × 픽셀당 비트수 = 초당 비트 수
    double bps = static_cast<double>(width) * height * fps * bitsPerPixel;
    return static_cast<long>(bps);
}

GstRTSPServer *setupRtspServer(StreamContext &ctx)
{
    gst_init(nullptr, nullptr);

    // RTSP 서버 생성 및 포트 설정
    GstRTSPServer *server = gst_rtsp_server_new();
    g_object_set(server, "service", "8555", NULL);

    // TLS 인증서 로딩
    GError *error = nullptr;
    GTlsCertificate *cert = g_tls_certificate_new_from_files("fire-cert.pem", "fire-key.pem", &error);
    if (!cert) {
        std::cerr << "TLS 인증서 로딩 실패: " << error->message << std::endl;
        g_error_free(error);
        return nullptr;
    }

    // 인증 객체 생성 및 인증서 설정
    GstRTSPAuth *auth = gst_rtsp_auth_new();
    gst_rtsp_server_set_auth(server, auth);
    gst_rtsp_auth_set_tls_certificate(auth, cert);

    // default_token 설정 (비밀번호 없이 접근 허용)
    GstRTSPToken *token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "anonymous", NULL);
    gst_rtsp_auth_set_default_token(auth, token);
    gst_rtsp_token_unref(token);
    g_object_unref(auth);
    g_object_unref(cert);

    // 미디어 factory 설정
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    long bitrate = calculateBitrate(ctx.width, ctx.height, ctx.fps, 0.08) / 1000;
    std::string launch_desc =
    "( appsrc name=video_src is-live=true do-timestamp=true format=time "
    "! videoconvert "
    "! video/x-raw,format=NV12 "
    "! queue "
    "! v4l2convert "
    "! queue "
    "! v4l2h264enc "
    "extra-controls=\"controls,repeat_sequence_header=1,"
    "video_bitrate=" + std::to_string(bitrate) + 
    ",h264_i_frame_period=1,h264_profile=0\" "
    "! video/x-h264,level=(string)4 "
    "! h264parse "
    "! rtph264pay name=pay0 pt=96 config-interval=1 )";

    gst_rtsp_media_factory_set_launch(factory, launch_desc.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_latency(factory, 200);
    gst_rtsp_media_factory_set_enable_rtcp(factory, TRUE);
    
    // ✔ appsrc 콜백 설정
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), &ctx);
    
    // 권한 설정 (default-permissions 미사용)
    GstRTSPPermissions *fac_perms = gst_rtsp_permissions_new();
    gst_rtsp_permissions_add_role(fac_perms, "anonymous",
        GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
        GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE,
        NULL);
    gst_rtsp_media_factory_set_permissions(factory, fac_perms);
    gst_rtsp_permissions_unref(fac_perms);

    // mount point 등록
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // attach
    if (!gst_rtsp_server_attach(server, NULL)) {
        std::cerr << "RTSP 서버 attach 실패" << std::endl;
        return nullptr;
    }

    std::cout << "RTSPS 서버 실행 중: rtsps://192.168.0.119:8555/test" << std::endl;
    return server;
}

void inferenceLoop(StreamContext *ctx)
{
    while (running) {
        Mat frame;
        if (!ctx->cap->read(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 1. 원본 프레임 보관 (박스 없음)
        {
            std::lock_guard<std::mutex> lock(raw_mutex);
            latest_raw_frame = frame.clone();
        }

        // 2. 버퍼에 저장
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            frame_buffer.push_back({ctx->frame_count, frame.clone()});
            if (frame_buffer.size() > 10)
                frame_buffer.pop_front();
        }

        // 3. 지연된 프레임 꺼내기
        Mat delayed_frame;
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            if (frame_buffer.size() > BUFFER_DELAY_FRAMES)
                delayed_frame = frame_buffer[frame_buffer.size() - BUFFER_DELAY_FRAMES - 1].second.clone();
            else
                delayed_frame = frame.clone();
        }

        // 4. 박스 그리기 (tracked 기반)
        {
            std::lock_guard<std::mutex> lock(track_mutex);
            for (const auto& [id, d] : ctx->tracked) {
                rectangle(delayed_frame, d.box, Scalar(0, 255, 0), 2);
                string label = (*ctx->class_names)[d.class_id] + format(" ID %d", id);
                putText(delayed_frame, label, d.box.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
            }
        }

        // 5. 송출용 프레임으로 설정
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = delayed_frame.clone();
            latest_pts = ctx->frame_count++;
        }

        //std::cout << "[IL] InferenceLoop 실행중 ..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / ctx->fps));
    }
}

void detectionLoop(StreamContext* ctx) {
    while (running) {
        Mat raw_frame;
        {
            std::lock_guard<std::mutex> lock(raw_mutex);
            if (latest_raw_frame.empty()) continue;
            raw_frame = latest_raw_frame.clone();
        }

        // 배경 모델 업데이트 + 모션 박스 추출
        if (ctx->frame_count == 0)
            raw_frame.convertTo(ctx->background_model, CV_32F);

        Mat bg8u = updateBackground(raw_frame, ctx->background_model, 0.01);
        Mat fg = getMotionMask(raw_frame, bg8u);
        auto motion_boxes = extractMotionBoxes(fg);

        // 감지 주기 제한
        if (++ctx->frame_counter % 5 != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // YOLO 감지
        auto detections = runDetection(*ctx->net, raw_frame, 0.3f, 0.4f, Size(640, 640));
        // 여기에 로그 출력 추가
        for (const auto& det : detections) {
            std::string label = (*ctx->class_names)[det.class_id];
            int conf = static_cast<int>(det.confidence * 100);
            if (label == "fire" || label == "smoke") {
                std::cout << "[DETECT] " << label << " detected with confidence " << conf << "%" << std::endl;
            }
        }


        // IOU 기반 tracking
        map<int, DetectionResult> updated;
        {
            std::lock_guard<std::mutex> lock(track_mutex);
            for (const auto& d : detections) {
                bool matched = false;
                for (const auto& [id, prev] : ctx->tracked) {
                    if ((d.box & prev.box).area() > 0 &&
                        static_cast<float>((d.box & prev.box).area()) / (d.box | prev.box).area() > 0.3f) {
                        updated[id] = d;
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    updated[ctx->next_id++] = d;
                }
            }
            ctx->tracked = std::move(updated);
        }

        // 박스 그리기
        Mat boxed_frame = raw_frame.clone();
        {
            std::lock_guard<std::mutex> lock(track_mutex);
            for (const auto& [id, tr] : ctx->tracked) {
                rectangle(boxed_frame, tr.box, Scalar(0, 0, 255), 2);
                string label = (*ctx->class_names)[tr.class_id] + format(" ID %d", id);
                putText(boxed_frame, label, tr.box.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 0, 255), 2);
            }
        }

        // 이벤트 전송
        auto now = steady_clock::now();
        if (!ctx->tracked.empty() &&
            duration_cast<seconds>(now - ctx->last_snapshot_time).count() >= 10) 
        {

            vector<uchar> jpeg_buf;
            vector<int> jpeg_params = {IMWRITE_JPEG_QUALITY, 90};
            imencode(".jpg", boxed_frame, jpeg_buf, jpeg_params);

            // 감지된 객체들 중 fire/smoke에 대해 전송
            for (const auto& det : detections) {
                std::string label = (*ctx->class_names)[det.class_id];
                int conf = static_cast<int>(det.confidence * 100);
                
                if ((label == "fire" || label == "smoke") && ledState && conf > 1) {
                    std::string event_type = "fire_detected";

                    send_jsonl_event(
                        event_type.c_str(),  // "fire_detected" 고정
                        1,
                        label.c_str(),       // "fire" 또는 "smoke"
                        1,
                        conf,                // confidence × 100
                        1,
                        jpeg_buf.data(),
                        jpeg_buf.size(),
                        ".jpeg"
                    );
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "[DL] detectionLoop 실행 중..." << std::endl;

    }
}