#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <atomic>
#include <vector>
#include <string>
#include "DetectionUtils.h"

using namespace std;
using namespace cv;

const vector<string> class_names = {
    "Angular Leafspot", "Anthracnose Fruit Rot", "Blossom Blight",
    "Gray Mold", "Leaf Spot", "Powdery Mildew Fruit",
    "Powdery Mildew Leaf", "ripe", "unripe"
};

atomic<bool> running(true);

struct StreamContext {
    VideoCapture* cap;
    dnn::Net* net;
    int width;
    int height;
    int fps;
    guint64 frame_count;
    vector<string>* class_names;
};

bool push_frame_to_appsrc(GstElement* appsrc, StreamContext* ctx) {
    Mat orig_frame;
    if (!ctx->cap->read(orig_frame)) {
        cerr << "Failed to read frame!" << endl;
        return false;
    }

    Mat infer_frame = orig_frame.clone();
    vector<DetectionResult> detections = runDetection(*(ctx->net), infer_frame, 0.4, 0.3, Size(416, 416));
    drawDetections(orig_frame, detections, *(ctx->class_names));

    Mat yuv_frame;
    cvtColor(orig_frame, yuv_frame, COLOR_BGR2YUV_I420);

    int size = yuv_frame.total() * yuv_frame.elemSize();
    GstBuffer* buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, yuv_frame.data, size);
    gst_buffer_unmap(buffer, &map);

    GST_BUFFER_PTS(buffer) = gst_util_uint64_scale(ctx->frame_count, GST_SECOND, ctx->fps);
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, ctx->fps);
    ctx->frame_count++;

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if (ret != GST_FLOW_OK) {
        cerr << "Failed to push buffer to appsrc: " << ret << endl;
        return false;
    }
    return true;
}

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

static void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data) {
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    GstElement *appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "video_src");
    StreamContext* ctx = static_cast<StreamContext*>(user_data);

    // 상태 초기화
    gst_element_set_state(appsrc, GST_STATE_READY);
    gst_element_set_state(appsrc, GST_STATE_PLAYING);

    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "I420",
        "width", G_TYPE_INT, ctx->width,
        "height", G_TYPE_INT, ctx->height,
        "framerate", GST_TYPE_FRACTION, ctx->fps, 1,
        NULL
    );
    gst_app_src_set_caps(GST_APP_SRC(appsrc), caps);
    gst_caps_unref(caps);

    ctx->frame_count = 0;
    running = true;

    g_signal_connect(appsrc, "need-data", G_CALLBACK(on_need_data), ctx);
    gst_object_unref(appsrc);
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

    gst_rtsp_media_factory_set_launch(factory,
        "( appsrc name=video_src is-live=true format=time "
        "! videoconvert ! x264enc tune=zerolatency "
        "! rtph264pay name=pay0 pt=96 )"
    );
    gst_rtsp_media_factory_set_shared(factory, TRUE);

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
