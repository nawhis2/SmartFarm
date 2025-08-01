#include "rtspStreaming.h"
#include "rtspServer.h"
#include "DetectionUtils.h"

int RtspStreaming(const int width, const int height, const int fps)
{
    string pipeline =
        "libcamerasrc ! "
        "video/x-raw,width=" +
        to_string(width) +
        ",height=" + to_string(height) +
        ",framerate=" + to_string(fps) + "/1 ! "
        "videoconvert ! videoscale ! "
        "queue max-size-buffers=1 leaky=downstream ! "
        "appsink caps=video/x-raw,format=BGR,width=" +
        to_string(width) + ",height=" + to_string(height);

    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened())
    {
        cerr << "Camera open failed" << endl;
        return -1;
    }

    // ✅ MOG2 모션 감지기 초기화
    initMotionDetector();

    StreamContext ctx;
    ctx.cap = &cap;
    ctx.width = width;
    ctx.height = height;
    ctx.fps = fps;
    ctx.frame_count = 0;
    ctx.last_snapshot_time = std::chrono::steady_clock::now();

    // 스레드 시작
    std::thread inference_thread(inferenceLoop, &ctx);
    std::thread detect_thread(detectionLoop, &ctx);

    // ⬇️ RTSP 서버 설정
    GstRTSPServer *server = setupRtspServer(ctx);
    if (!server)
    {
        running = false;
        inference_thread.join();
        detect_thread.join();
        return -1;
    }

    cout << "RTSP stream ready at rtsp://0.0.0.0:8554/test" << endl;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    // 종료 처리
    running = false;
    inference_thread.join();
    detect_thread.join();

    return 0;
}
