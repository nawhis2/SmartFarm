#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <atomic>
#include <vector>
#include <string>
#include "DetectionUtils.h" // runDetection, drawDetections 등

using namespace std;
using namespace cv;

// 클래스 이름 정의 (예시)
const vector<string> class_names = {
    "Angular Leafspot", "Anthracnose Fruit Rot", "Blossom Blight",
    "Gray Mold", "Leaf Spot", "Powdery Mildew Fruit",
    "Powdery Mildew Leaf", "ripe", "unripe"
};

atomic<bool> running(true);

// 스트림 컨텍스트 구조체
struct StreamContext {
    VideoCapture* cap;
    dnn::Net* net;
    int width;
    int height;
    int fps;
    guint64 frame_count;
    vector<string>* class_names;
};

// appsrc로 프레임 push
bool push_frame_to_appsrc(GstElement* appsrc, StreamContext* ctx) {
    Mat orig_frame;
    if (!ctx->cap->read(orig_frame)) {
        cerr << "Failed to read frame!" << endl;
        return false;
    }

    // 1. 프레임 복제본(clone)에서 추론 및 박스 좌표 계산
    Mat infer_frame = orig_frame.clone();
    vector<DetectionResult> detections = runDetection(*(ctx->net), infer_frame, 0.4, 0.3, Size(416, 416));

    // 2. 원본 프레임에 박스 등 합성
    drawDetections(orig_frame, detections, *(ctx->class_names));

    // 3. GStreamer 버퍼로 변환 및 송출
    int size = orig_frame.total() * orig_frame.elemSize();
    GstBuffer* buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, orig_frame.data, size);
    gst_buffer_unmap(buffer, &map);

    GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(ctx->frame_count, GST_SECOND, ctx->fps);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, ctx->fps);
    ctx->frame_count++;

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        cerr << "Failed to push buffer to appsrc!" << endl;
        return false;
    }
    return true;
}

// need-data 콜백
static void on_need_data(GstElement* appsrc, guint, StreamContext* ctx) {
    if (!running) {
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
        return;
    }
    if (!push_frame_to_appsrc(appsrc, ctx)) {
        running = false;
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    }
}

// media-configure 콜백
static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");
    StreamContext* ctx = static_cast<StreamContext*>(user_data);

    // appsrc caps 명시
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, ctx->width,
        "height", G_TYPE_INT, ctx->height,
        "framerate", GST_TYPE_FRACTION, ctx->fps, 1,
        NULL
    );
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    // need-data 콜백 연결
    g_signal_connect(appsrc, "need-data", G_CALLBACK(on_need_data), ctx);
    gst_object_unref(pipeline);
}

int main() {
    const int width = 640, height = 480, fps = 15;
    const string pipeline =
        "libcamerasrc camera-name=/base/soc/i2c0mux/i2c@1/imx219@10 "
        "! video/x-raw,width=640,height=480,framerate=15/1 "
        "! videoconvert ! videoscale ! appsink caps=video/x-raw,format=BGR,width=640,height=480";

    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr << "Failed to open camera!" << endl;
        return -1;
    }

    dnn::Net net = dnn::readNetFromONNX("Model.onnx");
    if (net.empty()) {
        cerr << "Failed to load model!" << endl;
        return -1;
    }
    net.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(dnn::DNN_TARGET_CPU);

    gst_init(nullptr, nullptr);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554");
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // RTSP 파이프라인: appsrc → videoconvert → x264enc → rtph264pay
    gst_rtsp_media_factory_set_launch(factory,
        "( appsrc name=video_src is-live=true format=time "
        "! videoconvert ! x264enc tune=zerolatency "
        "! rtph264pay name=pay0 pt=96 )"
    );
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    // 스트림 컨텍스트 준비
    StreamContext ctx;
    ctx.cap = &cap;
    ctx.net = &net;
    ctx.width = width;
    ctx.height = height;
    ctx.fps = fps;
    ctx.frame_count = 0;
    ctx.class_names = const_cast<vector<string>*>(&class_names);

    g_signal_connect(factory, "media-configure", (GCallback)media_configure, &ctx);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    if (gst_rtsp_server_attach(server, NULL) == 0) {
        g_printerr("Failed to attach RTSP server\n");
        return -1;
    }

    g_print("Stream ready at rtsp://0.0.0.0:8554/test\n");
    g_main_loop_run(loop);

    running = false;
    return 0;
}
