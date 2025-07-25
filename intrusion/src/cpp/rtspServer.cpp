#include "rtspServer.h"
#include "jsonlParse.h"

// ----------------------------
// Processing functions
// ----------------------------

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
    if (ret != GST_FLOW_OK)
    {
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

bool push_frame_to_appsrc(GstElement *appsrc, const cv::Mat &frame, int fps = 30)
{
    // 1. 프레임 유효성 검사
    if (frame.empty())
    {
        std::cerr << "[push_frame_to_appsrc] 빈 프레임입니다." << std::endl;
        return false;
    }

    // 2. GStreamer 버퍼 생성
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, frame.total() * frame.elemSize(), nullptr);
    if (!buffer)
    {
        std::cerr << "[push_frame_to_appsrc] GstBuffer 생성 실패" << std::endl;
        return false;
    }

    // 3. 버퍼에 데이터 복사
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE))
    {
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
    if (ret != GST_FLOW_OK)
    {
        std::cerr << "[push_frame_to_appsrc] 버퍼 푸시 실패: " << gst_flow_get_name(ret) << std::endl;
        return false;
    }

    return true;
}

static void push_dummy(GstElement *appsrc, StreamContext *ctx)
{
    // Dummy function to ensure appsrc is ready
    // This can be used to push an initial frame if needed

    std::cout << "[RTSP] Dummy push called" << std::endl;

    Mat dummy_frame(ctx->height, ctx->width, CV_8UC3, Scalar(255, 255, 255));
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

// 새로운 콜백 추가
static void on_media_prepared(GstRTSPMedia *media, gpointer user_data)
{
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");

    std::cout << "[RTSP] Client connected -> pushing dummy frames" << std::endl;

    running = true;
    static std::atomic<bool> dummy_pushed{false};
    if (!dummy_pushed.exchange(true))
    {
        push_dummy(appsrc, ctx);
    }

    gst_object_unref(appsrc);
    gst_object_unref(pipeline);
}

// media_configure 수정
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data)
{
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, ctx->width,
        "height", G_TYPE_INT, ctx->height,
        "framerate", GST_TYPE_FRACTION, ctx->fps, 1,
        NULL);
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    g_signal_connect(media, "prepared", G_CALLBACK(on_media_prepared), ctx);

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
    GTlsCertificate *cert = g_tls_certificate_new_from_files("server-cert.pem", "server-key.pem", &error);
    if (!cert)
    {
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
        "! v4l2convert "
        "! v4l2h264enc "
        "extra-controls=\"controls,repeat_sequence_header=1,"
        "video_bitrate=" +
        std::to_string(bitrate) +
        ",h264_i_frame_period=1,h264_profile=0\" "
        "! video/x-h264,level=(string)4 "
        "! h264parse "
        "! rtph264pay name=pay0 pt=96 config-interval=1 )";

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
    if (!gst_rtsp_server_attach(server, NULL))
    {
        std::cerr << "RTSP 서버 attach 실패" << std::endl;
        return nullptr;
    }

    std::cout << "RTSPS 서버 실행 중: rtsps://192.168.0.46:8555/test" << std::endl;
    return server;
}

void inferenceLoop(StreamContext *ctx)
{
    while (running)
    {
        Mat frame;
        if (!ctx->cap->read(frame))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // 1. 원본 프레임 보관
        {
            std::lock_guard<std::mutex> lock(raw_mutex);
            latest_raw_frame = frame.clone();
        }

        // 3. 송출용 프레임 설정
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            latest_frame = frame.clone();
            latest_pts = ctx->frame_count++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / ctx->fps));
    }
}

void detectionLoop(StreamContext *ctx)
{
    initMotionDetector();

    std::map<int, DetectionResult> tracked;
    std::map<int, int> appearCount;
    int next_id = 0;
    bool prev_had_motion = false;  // 🔸 이전 프레임에 감지된 객체가 있었는지

    while (running)
    {
        Mat raw_frame;
        {
            std::lock_guard<std::mutex> lock(raw_mutex);
            if (latest_raw_frame.empty())
                continue;
            raw_frame = latest_raw_frame.clone();
        }

        auto detections = detectMotion(raw_frame);
        runTracking(detections, tracked, appearCount, next_id);

        bool has_motion = false;
        for (const auto& [id, tr] : tracked)
        {
            if (appearCount[id] >= 3)  // 유효한 트래킹 대상이 있는지 확인
            {
                has_motion = true;
                break;
            }
        }

        // 박스만 그리기 (ID 표시 제거)
        Mat boxed_frame = raw_frame.clone();
        for (const auto& [id, tr] : tracked)
        {
            if (appearCount[id] < 3) continue;
            rectangle(boxed_frame, tr.box, Scalar(0, 0, 255), 2);
        }

        auto now = steady_clock::now();
        // 🔥 새로 모션이 발생한 경우에만 이미지 전송
        if (has_motion && !prev_had_motion &&
            duration_cast<seconds>(now - ctx->last_snapshot_time).count() >= 10)
        {
            vector<uchar> jpeg_buf;
            vector<int> jpeg_params = {IMWRITE_JPEG_QUALITY, 90};
            imencode(".jpg", boxed_frame, jpeg_buf, jpeg_params);

            send_jsonl_event("intrusion_detected", 1, "intrusion", 1, 0, 0,
                             jpeg_buf.data(), jpeg_buf.size(), ".jpeg");

            ctx->last_snapshot_time = steady_clock::now();
        }

        prev_had_motion = has_motion;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

