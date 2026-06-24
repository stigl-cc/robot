#pragma once
#include <sys/types.h>
#include <linux/videodev2.h>


class VideoCapture {
    protected:
    int fd;
    int index;

    void* video_buffer;
    size_t video_buffer_len;
    

    fd_set fds = { };
    timeval tv = { .tv_sec = 2 };

    const enum v4l2_buf_type v4l2_stream_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    public:
    VideoCapture(int index);
    ~VideoCapture();

    bool Open();
    void CaptureFrame();
    void Close();

    int GetVideoBuffer(const void** buffer_ptr);
};

