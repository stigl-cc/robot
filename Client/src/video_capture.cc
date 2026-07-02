#include <video_capture.hh>
#include <logger.hh>

#include <utility>
#include <fcntl.h>
#include <string>
#include <format>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libv4l2.h>

VideoCapture::VideoCapture(int index) :
    fd_(-1),
    index_(index),
    video_buffer_(nullptr),
    video_buffer_len_(0) { }

VideoCapture::VideoCapture(VideoCapture&& other) : fd_(-1) {
    fd_ = std::exchange(other.fd_, -1);
    index_ = std::exchange(other.index_, 0);
    video_buffer_ = std::exchange(other.video_buffer_, nullptr);
    video_buffer_len_ = std::exchange(other.video_buffer_len_, 0);
}

VideoCapture& VideoCapture::operator=(VideoCapture&& other) {
    if(this == &other)
        return *this;

    if(fd_ >= 0) close();
    fd_ = std::exchange(other.fd_, -1);
    index_ = std::exchange(other.index_, 0);
    video_buffer_ = std::exchange(other.video_buffer_, nullptr);
    video_buffer_len_ = std::exchange(other.video_buffer_len_, 0);
    return *this;
}

bool VideoCapture::open() {
    if(fd_ >= 0) {
        log_tag(LOG_WARN, "already opened");
        return false;
    }

    std::string device_file = std::format("/dev/video{}", index_);
    if((fd_ = v4l2_open(device_file.c_str(), O_RDWR)) < 0) {
        log_tag_no(LOG_ERR, "open /dev/videoX file");
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

    if(v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) < 0) {
        log_tag_no(LOG_ERR, "set pixel format");
        close();
        return false;
    }

    // Request v4l2 buffers
    struct v4l2_requestbuffers req = {
        .count = 1,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };

    if(v4l2_ioctl(fd_, VIDIOC_REQBUFS, &req) < 0) {
        log_tag_no(LOG_ERR, "request video buffer(s)");
        close();
        return false;
    }

    // Query those v4l2 buffers to get the memory offset
    struct v4l2_buffer buf = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };
    if(v4l2_ioctl(fd_, VIDIOC_QUERYBUF, &buf) < 0) {
        log_tag_no(LOG_ERR, "query video buffer(s)");
        close();
        return false;
    }

    void* buffer = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);

    if(buffer == MAP_FAILED) {
        log_tag_no(LOG_ERR, "mmap video buffer(s)");
        return false;
    }

    if(v4l2_ioctl(fd_, VIDIOC_STREAMON, &V4L2_STREAM_TYPE) < 0) {
        log_tag(LOG_ERR, "enable streaming");
        close();
        return false;
    }

    return true;
}


int VideoCapture::getVideoBuffer(const void** buffer_ptr) {
    *buffer_ptr = static_cast<const void*>(video_buffer_);
    return video_buffer_len_;
}

void VideoCapture::captureFrame() {
    // Enqueue the v4l2 buffer
    v4l2_buffer buf = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if(v4l2_ioctl(fd_, VIDIOC_QBUF, &buf) < 0) {
        log_tag(LOG_ERR, "enqueue buffer");
        close();
    }

    // Wait until a frame is available
    int ret;
    do {
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);

        ret = select(fd_ + 1, &fds, NULL, NULL, &tv);
    } while(ret == -1 && (errno != EINTR));

    if(ret == -1) {
        log_tag(LOG_ERR, "select'd the camera");
    }

    // Dequeue the v4l2 buffer
    buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if(v4l2_ioctl(fd_, VIDIOC_DQBUF, &buf) < 0) {
        log_tag(LOG_ERR, "dequeue buffer");
        close();
    }
}

void VideoCapture::close() {
    if(fd_ < 0)
        return;

    v4l2_ioctl(fd_, VIDIOC_STREAMOFF, &V4L2_STREAM_TYPE);
    v4l2_ioctl(fd_, VIDIOC_REQBUFS, 0); // free all buffers - https://www.kernel.org/doc/html/v4.10/media/uapi/v4l/vidioc-reqbufs.html

    if(video_buffer_) {
        v4l2_munmap(video_buffer_, video_buffer_len_);
        video_buffer_len_ = 0;
        video_buffer_ = nullptr;
    }

    v4l2_close(fd_);
    fd_ = -1;
}

VideoCapture::~VideoCapture() {
    close();
}
