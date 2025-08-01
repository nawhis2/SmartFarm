#include "rtspServer.h"
#include "jsonlParse.h"
#include "clientUtil.h"

// 캡처 전용 스레드
void capture_thread(StreamContext *ctx)
{
    int consecutive_failures = 0;
    const int MAX_CONSECUTIVE_FAILURES = 10;
    int reconnect_delay_ms = 1000;

    std::cout << "[CAPTURE] 캡처 스레드 시작" << std::endl;

    while (system_running)
    {
        cv::Mat frame;

        // 프레임 읽기 시도
        if (!ctx->cap->read(frame))
        {
            consecutive_failures++;
            std::cout << "[CAPTURE] 프레임 읽기 실패 " << consecutive_failures << "회" << std::endl;

            if (consecutive_failures >= MAX_CONSECUTIVE_FAILURES)
            {
                std::cout << "[CAPTURE] 카메라 재연결 시도..." << std::endl;

                // 기존 연결 해제
                ctx->cap->release();
                std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay_ms));

                // 재연결 시도
                std::string pipeline =
                    "rtspsrc location=rtsp://admin:Veda3rd1@192.168.0.28/profile2/media.smp "
                    "latency=100 protocols=tcp ! "
                    "rtph264depay ! "
                    "h264parse ! "
                    "avdec_h264 ! "
                    "videoconvert ! "
                    "video/x-raw,format=BGR ! "
                    "appsink sync=false max-buffers=1 drop=true";

                ctx->cap->open(pipeline, cv::CAP_GSTREAMER);

                if (ctx->cap->isOpened())
                {
                    consecutive_failures = 0;
                    reconnect_delay_ms = 1000;
                    std::cout << "[CAPTURE] 카메라 재연결 성공" << std::endl;
                }
                else
                {
                    reconnect_delay_ms = std::min(reconnect_delay_ms * 2, 10000);
                    std::cout << "[CAPTURE] 카메라 재연결 실패, " << reconnect_delay_ms << "ms 후 재시도" << std::endl;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        consecutive_failures = 0;

        // 프레임 유효성 검사
        if (frame.empty() || frame.cols == 0 || frame.rows == 0)
        {
            continue;
        }

        // 공유 버퍼에 최신 프레임 저장
        ctx->frame_buffer->update_frame(frame);

        // 캡처 속도 제어 (30fps 기준)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    std::cout << "[CAPTURE] 캡처 스레드 종료" << std::endl;
}

// 분석(맵핑) 전용 스레드
void mapping_thread(StreamContext *ctx)
{
    std::cout << "[MAPPING] 분석 스레드 시작" << std::endl;

    // 맵핑 초기화
    mapping_reset();

    while (system_running)
    {
        // 객체 감지 모드 (start 수신 시)
        if (mapping_is_detecting())
        {
            Mat analysis_frame;
            if (!ctx->frame_buffer->get_frame(analysis_frame))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            // 객체 검출 시도
            detected_flag = false;
            bool found;
            cv::Point center = mapping_detect_object(analysis_frame, &found);

            if (found)
            {
                // 객체 감지 성공 - ready 신호 전송
                sendFile("ready", "TEXT");
                mapping_set_tracking(true);
                mapping_set_detecting(false);
                std::cout << "[MAPPING] 객체 감지 성공 - ready 신호 전송" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // 트래킹 모드 (ready 신호 전송 후)
        else if (mapping_is_tracking())
        {
            Mat analysis_frame;
            if (!ctx->frame_buffer->get_frame(analysis_frame))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            // 객체 추적 및 좌표 전송
            bool found;
            cv::Point center = mapping_detect_object(analysis_frame, &found);

            if (found)
            {
                auto now = chrono::system_clock::now();
                long long t_ms = chrono::duration_cast<chrono::milliseconds>(
                                     now.time_since_epoch())
                                     .count();
                mapping_add_point(center, t_ms);
                std::cout << "[MAPPING] 객체 감지됨: ("
                          << center.x << ", " << center.y 
                          << ") at " << t_ms << "ms" << std::endl;
                const auto &timed = mapping_get_timed_points().back();
                send_coordinate_event(timed.pt, timed.right_pt, timed.left_pt, timed.t_ms);

                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 5 FPS
            }
            else
            {
                // 비활성 상태
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }
}

// 송출용 프레임 제공 함수 (GStreamer 콜백에서 호출)
bool pushFrame(GstElement *appsrc, StreamContext &ctx)
{
    Mat frame;

    // 공유 버퍼에서 최신 프레임 가져오기
    if (!ctx.frame_buffer->get_frame(frame))
    {
        std::cout << "[OUTPUT] 송출할 프레임 없음" << std::endl;
        return false;
    }

    // GStreamer 버퍼 생성 및 전송
    int size = frame.total() * frame.elemSize();
    GstBuffer *buf = gst_buffer_new_and_alloc(size);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_WRITE);
    memcpy(info.data, frame.data, size);
    gst_buffer_unmap(buf, &info);

    // 타임스탬프 설정
    static guint64 pts_counter = 0;
    GST_BUFFER_PTS(buf) = gst_util_uint64_scale(pts_counter++, GST_SECOND, ctx.fps);
    GST_BUFFER_DURATION(buf) = gst_util_uint64_scale_int(1, GST_SECOND, ctx.fps);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf);
    if (ret != GST_FLOW_OK)
    {
        std::cerr << "[OUTPUT] 버퍼 푸시 실패!" << std::endl;
        return false;
    }

    return true;
}

static void onNeedData(GstElement *appsrc, guint, gpointer user_data)
{
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);

    if (!system_running)
    {
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    }
    else
    {
        pushFrame(appsrc, *ctx);
    }
}

// 클라이언트 연결시 분석 시작 신호
static void on_media_prepared(GstRTSPMedia *media, gpointer user_data)
{
    StreamContext *ctx = reinterpret_cast<StreamContext *>(user_data);

    // std::cout << "[RTSP] 클라이언트 연결됨 - 분석 시작" << std::endl;
    // // 트래킹 활성화
    // mapping_set_tracking(true);
}

// 미디어 설정
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

    GstRTSPServer *server = gst_rtsp_server_new();
    g_object_set(server, "service", "8555", NULL);

    // TLS 설정 (기존과 동일)
    GError *error = nullptr;
    GTlsCertificate *cert = g_tls_certificate_new_from_files("mapping-cert.pem", "mapping-key.pem", &error);
    if (!cert)
    {
        std::cerr << "TLS 인증서 로딩 실패: " << error->message << std::endl;
        g_error_free(error);
        return nullptr;
    }

    GstRTSPAuth *auth = gst_rtsp_auth_new();
    gst_rtsp_server_set_auth(server, auth);
    gst_rtsp_auth_set_tls_certificate(auth, cert);

    GstRTSPToken *token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "anonymous", NULL);
    gst_rtsp_auth_set_default_token(auth, token);
    gst_rtsp_token_unref(token);
    g_object_unref(auth);
    g_object_unref(cert);

    // 미디어 팩토리 설정
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    long bitrate = calculateBitrate(ctx.width, ctx.height, ctx.fps, 0.08) / 1000;
    // appsrc 기반 파이프라인으로 변경
    std::string launch_desc =
        "( appsrc name=video_src is-live=true format=time do-timestamp=true ! "
        "videoconvert ! video/x-raw,format=I420 ! "
        "x264enc tune=zerolatency bitrate=" +
        std::to_string(bitrate) + " speed-preset=ultrafast ! "
                                  "rtph264pay config-interval=1 name=pay0 pt=96 )";

    gst_rtsp_media_factory_set_launch(factory, launch_desc.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_enable_rtcp(factory, TRUE);

    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), &ctx);

    // 권한 설정
    GstRTSPPermissions *fac_perms = gst_rtsp_permissions_new();
    gst_rtsp_permissions_add_role(fac_perms, "anonymous",
                                  GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
                                  GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
    gst_rtsp_media_factory_set_permissions(factory, fac_perms);
    gst_rtsp_permissions_unref(fac_perms);

    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    if (!gst_rtsp_server_attach(server, NULL))
    {
        std::cerr << "RTSP 서버 attach 실패" << std::endl;
        return nullptr;
    }

    std::cout << "RTSPS 서버 실행 중: rtsps://192.168.0.170:8555/test" << std::endl;
    return server;
}

void send_coordinate_event(const cv::Point &center,
                           const cv::Point &right,
                           const cv::Point &left,
                           long long t_ms)
{
    char buf[512];
    // 포맷 문자열: 필드 끝에는 쉼표X, 마지막은 "}\n"로 마감(닫힘 중괄호 하나만)
    int n = snprintf(buf, sizeof(buf),
                     "{"
                     "\"timestamp\":%lld,"
                     "\"center_x\":%d,"
                     "\"center_y\":%d,"
                     "\"right_x\":%d,"
                     "\"right_y\":%d,"
                     "\"left_x\":%d,"
                     "\"left_y\":%d"
                     "}\n",
                     t_ms,
                     center.x, center.y,
                     right.x, right.y,
                     left.x, left.y);
    // buf[sizeof(buf) - 1] = '\0'; // 혹시 모를 null-termination

    sendFile(buf, "MAP");
    printf("send!!!!!!!!!!!!!%s\n", buf);
}
