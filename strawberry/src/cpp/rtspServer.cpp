#include "rtspServer.h"
#include "jsonlParse.h"

const vector<string> class_names = {
    "Angular Leafspot", "Anthracnose Fruit Rot", "Blossom Blight",
    "Gray Mold", "Leaf Spot", "Powdery Mildew Fruit",
    "Powdery Mildew Leaf", "ripe", "unripe"
};
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
    GTlsCertificate *cert = g_tls_certificate_new_from_files("strawberry-cert.pem", "strawberry-key.pem", &error);
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
        "! v4l2convert "
        "! v4l2h264enc extra-controls=\"controls,repeat_sequence_header=1,"
        "video_bitrate=" +
        std::to_string(bitrate) + ",h264_i_frame_period=1,h264_profile=4\" "
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
    if (!gst_rtsp_server_attach(server, NULL)) {
        std::cerr << "RTSP 서버 attach 실패" << std::endl;
        return nullptr;
    }

    std::cout << "RTSPS 서버 실행 중: rtsps://192.168.0.170:8555/test" << std::endl;
    return server;
}

void inferenceLoop(StreamContext* ctx) {
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
        auto detections = runDetection(*ctx->net, raw_frame, 0.6f, 0.4f, Size(416, 416));

        // tracking (IOU 기반)
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

        // 박스 그리기 (저장/전송용)
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
        if (!detections.empty()) {
            vector<uchar> jpeg_buf;
            vector<int> jpeg_params = {IMWRITE_JPEG_QUALITY, 70};
            imencode(".jpg", boxed_frame, jpeg_buf, jpeg_params);
            
            for (const auto& det : detections) {
                const char* class_type = ctx->class_names->at(det.class_id).c_str();
                int conf = static_cast<int>(det.confidence * 100);
                int feature = det.feature; // 0: left, 1: right
                
                // 라벨 및 신뢰도 조건 확인
                if ((class_type != "unripe") && conf > 60) {
                    send_jsonl_event(
                        "strawberry_detected",
                        1, // has_file
                        class_type, 1,    // class_type, has_class_type
                        feature, 1,    // feature, has_feature
                        jpeg_buf.data(),
                        jpeg_buf.size(),
                        ".jpeg"
                    );
                }
            }
        }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}