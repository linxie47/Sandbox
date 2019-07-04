#pragma once

#include <gflags/gflags.h>

DEFINE_string(i, "", "Required. Path to input video file");

//DEFINE_string(f, "", "Input format. Example, use v4l2 for streaming from web camera");

DEFINE_string(m, "", "Required. Path to IR .xml file");

//DEFINE_string(e, "", "Path layer extension plugin");

//DEFINE_string(decode_device, "", "Device for decoding, set to 'vaapi' for HW-accelerated decoding");

DEFINE_string(d, "CPU", "Device for inference, 'CPU' or 'GPU'");

DEFINE_int32(nireq, 1, "Inference request number");

//DEFINE_int32(queue, 0, "Inference queue size");

//DEFINE_double(thresh, .4, "Confidence threshold for bounding boxes 0-1");

//DEFINE_bool(no_render, false, "Disable rendering to screen");

#define RET_IF_FAIL(_FUNC)                                                                                             \
    {                                                                                                                  \
        int _ret = _FUNC;                                                                                              \
        if (_ret < 0) {                                                                                                \
            av_log(NULL, AV_LOG_ERROR, "Error %x calling " #_FUNC "\n", _ret);                                         \
            return _ret;                                                                                               \
        }                                                                                                              \
    }

#define RET_IF_ZERO(_PTR)                                                                                              \
    {                                                                                                                  \
        auto _value = (_PTR);                                                                                          \
        if (_value == 0) {                                                                                             \
            av_log(NULL, AV_LOG_ERROR, "Zero value " #_PTR "\n");                                                      \
            return -1;                                                                                                 \
        }                                                                                                              \
    }
