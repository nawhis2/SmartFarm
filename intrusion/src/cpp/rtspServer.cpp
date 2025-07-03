#include "rtspServer.h"

const vector<string> class_names = {"Deer", "Hog", "Raccoon", "person"};
// ----------------------------
// Processing functions
// ----------------------------
Mat updateBackground(const Mat &frame, Mat &background, double alpha)
{
    Mat f32;
    frame.convertTo(f32, CV_32F);
    accumulateWeighted(f32, background, alpha);
    Mat bg8u;
    background.convertTo(bg8u, CV_8U);
    return bg8u;
}

Mat getMotionMask(const Mat &frame, const Mat &bg8u, double diffThresh = 30.0, int blurSize = 15)
{
    Mat diff, gray, fg;
    absdiff(frame, bg8u, diff);
    cvtColor(diff, gray, COLOR_BGR2GRAY);
    threshold(gray, fg, diffThresh, 255, THRESH_BINARY);
    GaussianBlur(fg, fg, Size(blurSize, blurSize), 0);
    threshold(fg, fg, diffThresh, 255, THRESH_BINARY);
    return fg;
}

vector<Rect> extractMotionBoxes(const Mat &fgmask, double minArea = 500.0)
{
    vector<vector<Point>> contours;
    findContours(fgmask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<Rect> boxes;
    for (auto &c : contours)
    {
        if (contourArea(c) < minArea)
            continue;
        boxes.push_back(boundingRect(c));
    }
    return boxes;
}

float calculateIoU(const Rect &a, const Rect &b)
{
    int inter = (a & b).area();
    int uni = a.area() + b.area() - inter;
    return uni > 0 ? static_cast<float>(inter) / uni : 0.0f;
}

vector<DetectionResult> detectAndTrack(StreamContext &ctx, const Mat &frame)
{
    // Background & motion initialization on first frame
    if (ctx.frame_count == 0)
        frame.convertTo(ctx.background_model, CV_32F);
    Mat bg8u = updateBackground(frame, ctx.background_model, 0.01);
    Mat fg = getMotionMask(frame, bg8u);
    auto motion_boxes = extractMotionBoxes(fg);

    // Detection every 5 frames
    vector<DetectionResult> dets;
    if (++ctx.frame_counter % 5 == 0)
    {
        dets = runDetection(*ctx.net, frame, 0.6f, 0.4f, Size(640, 640));
        vector<DetectionResult> filtered;
        for (auto &d : dets)
        {
            string cls = (*ctx.class_names)[d.class_id];
            if (cls == "Hog" || cls == "person")
            {
                for (auto &mb : motion_boxes)
                {
                    if ((d.box & mb).area() > 0)
                    {
                        filtered.push_back(d);
                        break;
                    }
                }
            }
        }
        // IOU-based tracking
        map<int, DetectionResult> updated;
        for (auto &d : filtered)
        {
            bool matched = false;
            for (auto &tr : ctx.tracked)
            {
                if (calculateIoU(d.box, tr.second.box) > 0.3f)
                {
                    updated[tr.first] = d;
                    matched = true;
                    break;
                }
            }
            if (!matched)
                updated[ctx.next_id++] = d;
        }
        ctx.tracked = move(updated);
        ctx.last_detections = filtered;
    }
    else
    {
        dets = ctx.last_detections;
    }
    return dets;
}

bool pushFrame(GstElement *appsrc, StreamContext &ctx)
{
    Mat frame;
    if (!ctx.cap->read(frame))
    {
        this_thread::sleep_for(chrono::milliseconds(10));
        return true;
    }
    // Process detection and tracking
    detectAndTrack(ctx, frame);
    // Draw tracked results on frame
    for (auto &tr : ctx.tracked)
    {
        rectangle(frame, tr.second.box, Scalar(0, 255, 0), 2);
        string label = class_names[tr.second.class_id] + format(" ID %d", tr.first);
        putText(frame, label, tr.second.box.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0), 2);
    }
    // Push to GStreamer appsrc
    int size = frame.total() * frame.elemSize();
    GstBuffer *buf = gst_buffer_new_and_alloc(size);
    GstMapInfo info;
    gst_buffer_map(buf, &info, GST_MAP_WRITE);
    memcpy(info.data, frame.data, size);
    gst_buffer_unmap(buf, &info);
    GST_BUFFER_PTS(buf) = gst_util_uint64_scale(ctx.frame_count, GST_SECOND, ctx.fps);
    GST_BUFFER_DURATION(buf) = gst_util_uint64_scale_int(1, GST_SECOND, ctx.fps);
    gst_app_src_push_buffer(GST_APP_SRC(appsrc), buf);
    ctx.frame_count++;
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
    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554");
    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    long bitrate = calculateBitrate(ctx.width, ctx.height, ctx.fps, 0.08) / 1000;

    string launchDesc =
        "( appsrc name=video_src is-live=true format=time "
        "! videoconvert ! video/x-raw,format=I420 "
        "! x264enc tune=zerolatency bitrate=" +
        to_string(bitrate) +
        " speed-preset=superfast "
        "! rtph264pay name=pay0 pt=96 )";

    gst_rtsp_media_factory_set_launch(factory, launchDesc.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_configure), &ctx);
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);
    if (gst_rtsp_server_attach(server, nullptr) == 0)
    {
        cerr << "Failed to attach RTSP server" << endl;
        return nullptr;
    }
    return server;
}