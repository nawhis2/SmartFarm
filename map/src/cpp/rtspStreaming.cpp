#include "rtspStreaming.h"
#include "rtspServer.h"

int RtspStreaming(const int width, const int height, const int fps) {
    std::string pipeline =
    "rtspsrc location=rtsp://admin:Veda3rd1@192.168.0.28/profile2/media.smp "
    "latency=100 protocols=tcp ! "
    "rtph264depay ! "
    "h264parse ! "
    "avdec_h264 ! "
    "videoconvert ! "
    "video/x-raw,format=BGR ! "
    "appsink sync=false max-buffers=1 drop=true";

    // std::string pipeline =
    //     "rtspsrc location=rtsp://admin:Veda3rd1@192.168.0.28/profile2/media.smp "
    //     "latency=50 protocols=tcp udp-reconnect=true retry=3 timeout=5000000 ! "
    //     "queue max-size-buffers=3 leaky=downstream ! "
    //     "rtph264depay ! "
    //     "queue max-size-buffers=3 ! "
    //     "avdec_h264 max-threads=2 ! "
    //     "videoconvert ! videoscale method=0 ! "
    //     "video/x-raw,format=BGR,width=" + std::to_string(width) +
    //     ",height=" + std::to_string(height) + ",framerate=15/1 ! "
    //     "appsink sync=false max-buffers=3 drop=true emit-signals=true";
    
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if (!cap.isOpened()) {
        std::cerr << "Camera open failed" << std::endl;
        return -1;
    }
    
    // 스트리밍 컨텍스트 설정
    StreamContext ctx;
    ctx.cap = &cap;
    ctx.width = width;
    ctx.height = height;
    ctx.fps = fps;
    ctx.frame_count = 0;
    ctx.frame_buffer = &global_frame_buffer;
    
    std::cout << "=== 2-스레드 RTSP 스트리밍 시작 ===" << std::endl;
    
    // 2개의 독립 스레드 생성
    std::thread capture_th(capture_thread, &ctx);
    std::thread mapping_th(mapping_thread, &ctx);
    
    // RTSP 서버 구동 (송출 스레드는 GStreamer 콜백으로 처리)
    GstRTSPServer *server = setupRtspServer(ctx);
    if (!server) {
        std::cout << "RTSP 서버 초기화 실패" << std::endl;
        system_running = false;
        
        capture_th.join();
        mapping_th.join();
        return -1;
    }
    
    std::cout << "RTSP stream ready at rtsp://0.0.0.0:8555/test" << std::endl;
    
    // 메인 루프
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    
    g_main_loop_run(loop);
    
    // 종료 처리
    std::cout << "\n=== 종료 처리 시작 ===" << std::endl;
    system_running = false;
    
    // 모든 스레드 종료 대기
    capture_th.join();
    mapping_th.join();
    
    std::cout << "모든 스레드 종료 완료" << std::endl;
    return 0;
}
