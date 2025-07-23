#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <glib.h>
#include <string>
#include <iostream>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // RTSP 서버 생성
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8555");  // TLS는 보통 8555 포트를 많이 씀

    // 인증 객체 생성
    GstRTSPAuth *auth = gst_rtsp_auth_new();
    GError *error = nullptr;

    // TLS 인증서 로드 (PEM 형식)
    GTlsCertificate *cert = g_tls_certificate_new_from_files("server-cert.pem", "server-key.pem", &error);
    if (!cert) {
        std::cerr << "TLS 인증서 로딩 실패: " << error->message << std::endl;
        g_error_free(error);
        return -1;
    }
    gst_rtsp_auth_set_tls_certificate(auth, cert);

    // 인증 객체 생성 후, 익명(비밀번호 없는) 접근 토큰 설정
    GstRTSPToken *token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "user", NULL);
    gst_rtsp_auth_set_default_token(auth, token);
    gst_rtsp_token_unref(token);

    gst_rtsp_server_set_auth(server, auth);

    // 파이프라인 설정
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    std::string launch_desc =
        "( rtspsrc location=rtsp://admin:Veda3rd1@192.168.0.28/profile2/media.smp latency=100 ! "
        "rtph264depay ! avdec_h264 ! videoconvert ! "
        "x264enc tune=zerolatency bitrate=4096 speed-preset=ultrafast ! "
        "rtph264pay name=pay0 pt=96 )";

    gst_rtsp_media_factory_set_launch(factory, launch_desc.c_str());
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    gst_rtsp_media_factory_add_role(factory, "user",
        GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
        GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE,
    NULL);

    g_object_unref(mounts);

    // 서버 실행
    gst_rtsp_server_attach(server, nullptr);
    g_print("RTSPS 중계 서버 실행 중: rtsps://192.168.0.119:8555/test\n");

    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    return 0;
}
