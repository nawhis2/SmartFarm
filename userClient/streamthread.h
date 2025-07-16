#ifndef STREAMTHREAD_H
#define STREAMTHREAD_H

#include <QThread>
#include <gst/gst.h>

class StreamThread : public QThread {
    Q_OBJECT
public:
    StreamThread(GstElement *pipeline, GMainLoop *loop)
        : pipeline(pipeline), loop(loop) {}

protected:
    void run() override {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (loop && !g_main_loop_is_running(loop))
            g_main_loop_run(loop);
    }

private:
    GstElement *pipeline;
    GMainLoop *loop;
};

#endif // STREAMTHREAD_H
