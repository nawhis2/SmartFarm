#include "rtspServer.h"
#include "jsonlParse.h" 

const vector<string> class_names = {"fire", "smoke"};
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

vector<DetectionResult> detectAndTrack(StreamContext &ctx, const Mat &frame)
{
    // Background & motion initialization on first frame
    if (ctx.frame_count == 0)
        frame.convertTo(ctx.background_model, CV_32F);
    Mat bg8u = updateBackground(frame, ctx.background_model, 0.01);
    Mat fg = getMotionMask(frame, bg8u);
    auto motion_boxes = extractMotionBoxes(fg);

    // Detection every 5 frames
    vector<DetectionResult> dets;
    if (++ctx.frame_counter % 5 == 0)
    {
        dets = runDetection(*ctx.net, frame, 0.6f, 0.4f, Size(640, 640));
        vector<DetectionResult> filtered;
        for (auto &d : dets)
        {
            string cls = (*ctx.class_names)[d.class_id];
            if (cls == "fire" || cls == "smoke")
            {
                for (auto &mb : motion_boxes)
                {
                    if ((d.box & mb).area() > 0)
                    {
                        filtered.push_back(d);
                        break;
                    }
                }
            }
        }
        // IOU-based tracking
        map<int, DetectionResult> updated;
        for (auto &d : filtered)
        {
            bool matched = false;
            for (auto &tr : ctx.tracked)
            {
                if (calculateIoU(d.box, tr.second.box) > 0.3f)
                {
                    updated[tr.first] = d;
                    matched = true;
                    break;
                }
            }
            if (!matched)
                updated[ctx.next_id++] = d;
        }
        ctx.tracked = move(updated);
        ctx.last_detections = filtered;
    }
    else
    {
        dets = ctx.last_detections;
    }
    return dets;
}

bool pushFrame(GstElement *appsrc, StreamContext &ctx)
{
    Mat frame;
    guint64 pts;

    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        if (latest_frame.empty())
            return false;

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

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
{
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);

    // 상태 초기화
    gst_element_set_state(appsrc, GST_STATE_READY);
    gst_element_set_state(appsrc, GST_STATE_PLAYING);

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, ctx->width,
        "height", G_TYPE_INT, ctx->height,
        "framerate", GST_TYPE_FRACTION, ctx->fps, 1,
        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

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
    "( appsrc name=video_src is-live=true format=time "
    "! videoconvert ! video/x-raw,format=NV12 "
    "! v4l2convert "
    "! v4l2h264enc extra-controls=\"controls,repeat_sequence_header=1,"
    "video_bitrate=20000000,h264_i_frame_period=1,h264_profile=4\" "
    "! video/x-h264,level=(string)4 "
    "! h264parse "
    "! rtph264pay name=pay0 pt=96 config-interval=1 )";
    
        // "( appsrc name=video_src is-live=true format=time "
        // "! videoconvert ! video/x-raw,format=I420 "
        // "! x264enc tune=zerolatency bitrate=" + std::to_string(bitrate) +
        // " speed-preset=superfast "
        // "! rtph264pay name=pay0 pt=96 )";

    gst_rtsp_media_factory_set_launch(factory, launch_desc.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
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

void inferenceLoop(StreamContext* ctx) {
    while (running) {
        Mat frame;
        if (!ctx->cap->read(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 객체 감지
        std::vector<DetectionResult> results = detectAndTrack(*ctx, frame);

        // 추적된 객체 표시
        for (auto& tr : ctx->tracked) {
            rectangle(frame, tr.second.box, Scalar(0, 255, 0), 2);
            string label = (*ctx->class_names)[tr.second.class_id] + format(" ID %d", tr.first);
            putText(frame, label, tr.second.box.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
        }
        
        // 10초에 한 번씩 이벤트 전송
        auto now = steady_clock::now();
        std::cout << results.size() << " objects detected at frame " << ctx->frame_count << std::endl;
        if (!results.empty() &&
        duration_cast<seconds>(now - ctx->last_snapshot_time).count() >= 10) {
            
            // 스냅샷 생성
            Mat snapshot = frame.clone();
            vector<uchar> jpeg_buf;
            vector<int> jpeg_params = {IMWRITE_JPEG_QUALITY, 90};
            imencode(".jpg", snapshot, jpeg_buf, jpeg_params);
            
            // 감지된 객체들 중 fire/smoke에 대해 전송
            for (const auto& det : results) {
                std::string label = (*ctx->class_names)[det.class_id];
                int conf = static_cast<int>(det.confidence * 100);
                
                if ((label == "fire" || label == "smoke") && conf > 1) {
                    std::string event_type = label + "_detected";

                    send_jsonl_event(
                        event_type.c_str(),  // "fire_detected" 또는 "smoke_detected"
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

            ctx->last_snapshot_time = now;
        }

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = frame.clone();
            latest_pts = ctx->frame_count++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(66));
    }
}
