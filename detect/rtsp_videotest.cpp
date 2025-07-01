#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    GMainLoop* loop = g_main_loop_new(NULL, FALSE);

    // RTSP 서버 생성
    GstRTSPServer* server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554"); // 포트 지정

    // mount points 가져오기
    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);

    // media factory 생성
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    // videotestsrc를 이용한 스트림 파이프라인
    gst_rtsp_media_factory_set_launch(factory,
        "( videotestsrc is-live=1 ! video/x-raw,width=640,height=480,framerate=15/1 "
        "! x264enc tune=zerolatency ! rtph264pay name=pay0 pt=96 )");

    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // /test 경로에 factory 등록
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

    g_object_unref(mounts);

    // 서버 실행
    if (gst_rtsp_server_attach(server, NULL) == 0) {
        std::cerr << "Failed to attach RTSP server" << std::endl;
        return -1;
    }

    std::cout << "Stream ready at rtsp://127.0.0.1:8554/test" << std::endl;

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(server);

    return 0;
}
