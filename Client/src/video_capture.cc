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
        std::cerr << "W: Video Capture already opened" << std::endl;
        return false;
    }

    std::string device_file = std::format("/dev/video{}", index);
    if((fd = v4l2_open(device_file.c_str(), O_RDWR)) < 0) {
        std::cerr << "W: Could not open video file!" << strerror(errno) << std::endl;
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
        std::cerr << "E: Could not set video pixel format " << strerror(errno) << std::endl;
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
        std::cerr << "E: Could not request video buffer(s)" << strerror(errno) << std::endl;
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
        std::cerr << "E: Could not query video buffer(s)" << strerror(errno) << std::endl;
        Close();
        return false;
    }

    void* buffer = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

    if(buffer == MAP_FAILED) {
        std::cerr << "E: Could not mmap video buffer(s)" << strerror(errno) << std::endl;
        return false;
    }

    if(v4l2_ioctl(fd, VIDIOC_STREAMON, &v4l2_stream_type) < 0) {
        std::cerr << "Streaming could not be enabled" << std::endl;
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
        std::cerr << "Buffer could not be enqueued" << std::endl;
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
        std::cerr << "E: Camera could not be select'd!" << std::endl;
    }

    // Dequeue the v4l2 buffer
    buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if(v4l2_ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        std::cerr << "Buffer could not be dequeued" << std::endl;
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
