#pragma once

#include <string_view>
#include <sys/types.h>
#include <linux/videodev2.h>

class VideoCapture {
    protected:
    static constexpr std::string_view LOG_TAG = "VideoCapture: ";
    const enum v4l2_buf_type
        V4L2_STREAM_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    int fd_;
    int index_;

    void* video_buffer_;
    size_t video_buffer_len_;

    fd_set fds = { };
    timeval tv = { .tv_sec = 2 };

    public:
    VideoCapture(int index);

    VideoCapture(const VideoCapture&) = delete;
    VideoCapture& operator=(const VideoCapture&) = delete;

    VideoCapture(VideoCapture&&);
    VideoCapture& operator=(VideoCapture&&);

    bool open();

    int getVideoBuffer(const void** buffer_ptr);
    void captureFrame();

    void close();
    ~VideoCapture();
};
