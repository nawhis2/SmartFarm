#include "rtspStreaming.h"
#include "rtspServer.h"

int RtspStreaming(const int width, const int height, const int fps)
{
    string pipeline =
        "libcamerasrc camera-name=/base/soc/i2c0mux/i2c@1/imx219@10 ! "
        "video/x-raw,width=" +
        to_string(width) + ",height=" + to_string(height) + ",framerate=" + to_string(fps) + "/1 ! "
                                                                                    "videoconvert ! videoscale ! appsink caps=video/x-raw,format=BGR,width=" +
        to_string(height) + ",height=" + to_string(height);
    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened())
    {
        cerr << "Camera open failed" << endl;
        return -1;
    }
    dnn::Net net = dnn::readNetFromONNX("best.onnx");
    net.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(dnn::DNN_TARGET_CPU);

    StreamContext ctx;
    ctx.cap = &cap;
    ctx.net = &net;
    ctx.class_names = const_cast<vector<string> *>(&class_names);
    ctx.width = width;
    ctx.height = height;
    ctx.fps = fps;
    ctx.frame_count = 0;
    ctx.frame_counter = 0;
    ctx.next_id = 0;
    ctx.background_model = Mat();

    std::thread inference_thread(inferenceLoop, &ctx);

    // ⬇️ RTSP 서버 설정
    GstRTSPServer *server = setupRtspServer(ctx);
    if (!server)
    {
        running = false;
        inference_thread.join();
        return -1;
    }
    sleep(1);
    
    cout << "RTSP stream ready at rtsps://0.0.0.0:8555/test" << endl;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(loop);

    running = false;
    inference_thread.join();
    
    return 0;
}