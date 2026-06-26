#include "logger.hh"
#include <cstring>
#include <video_capture.hh>

#include <fcntl.h>
#include <string>
#include <format>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <iostream>
#include <sys/mman.h>

#include <libv4l2.h>

VideoCapture::VideoCapture(int index) :
    fd(0),
    index(index),
    video_buffer(nullptr),
    video_buffer_len(0) { }

VideoCapture::~VideoCapture() {
    Close();
}

bool VideoCapture::Open() {
    if(fd > 0) {
        log_tag("W", "already opened");
        return false;
    }

    std::string device_file = std::format("/dev/video{}", index);
    if((fd = v4l2_open(device_file.c_str(), O_RDWR)) < 0) {
        log_tag_no("E", "open /dev/videoX file");
        return false;
    }

    // Set pixel format
    struct v4l2_format fmt = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .fmt = {
            .pix = {
                .width = 1280,
                .height = 720,
                .pixelformat = V4L2_PIX_FMT_RGB24
            }
        }
    };

    if(v4l2_ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        log_tag_no("E", "set pixel format");
        Close();
        return false;
    }

    // Request v4l2 buffers
    struct v4l2_requestbuffers req = {
        .count = 1,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if(v4l2_ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        log_tag_no("E", "request video buffer(s)");
        Close();
        return false;
    }

    // Query those v4l2 buffers to get the memory offset
    struct v4l2_buffer buf = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };
    if(v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        log_tag_no("E", "query video buffer(s)");
        Close();
        return false;
    }

    void* buffer = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    if(buffer == MAP_FAILED) {
        log_tag_no("E", "mmap video buffer(s)");
        return false;
    }

    if(v4l2_ioctl(fd, VIDIOC_STREAMON, &v4l2_stream_type) < 0) {
        log_tag("E", "enable streaming");
        Close();
        return false;
    }

    return true;
}

void VideoCapture::CaptureFrame() {
    // Enqueue the v4l2 buffer
    v4l2_buffer buf = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if(v4l2_ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        log_tag("E", "enqueue buffer");
        Close();
    }

    // Wait until a frame is available
    int ret;
    do {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        ret = select(fd + 1, &fds, NULL, NULL, &tv);
    } while(ret == -1 && (errno != EINTR));

    if(ret == -1) {
        log_tag("E", "select'd the camera");
    }

    // Dequeue the v4l2 buffer
    buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if(v4l2_ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        log_tag("E", "dequeue buffer");
        Close();
    }
}

void VideoCapture::Close() {
    if(!fd) return;

    v4l2_ioctl(fd, VIDIOC_STREAMOFF, &v4l2_stream_type);
    v4l2_ioctl(fd, VIDIOC_REQBUFS, 0); // free all buffers - https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/vidioc-reqbufs.html

    if(video_buffer) {
        v4l2_munmap(video_buffer, video_buffer_len);
        video_buffer_len = 0;
        video_buffer = nullptr;
    }

    v4l2_close(fd);
    fd = 0;
}

int VideoCapture::GetVideoBuffer(const void** buffer_ptr) {
    *buffer_ptr = static_cast<const void*>(video_buffer);
    return video_buffer_len;
}
