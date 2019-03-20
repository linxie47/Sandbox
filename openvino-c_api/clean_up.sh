#!/bin/bash

echo "remove libraries......"

LIBRAY_PATH=/usr/lib/x86_64-linux-gnu
cd $LIBRAY_PATH;
rm -rf \
    cldnn_global_custom_kernels    \
    libade.a                       \
    libclDNN64.so                  \
    libcldnn_kernel_selector64.a   \
    libclDNNPlugin.so              \
    libcpu_extension.so            \
    libfluid.a                     \
    libformat_reader.so            \
    libgflags_nothreads.*          \
    libGNAPlugin.so                \
    libGNAPlugin_test_static.a     \
    libhelpers.a                   \
    libHeteroPlugin.so             \
    libinference_engine_c_wrapper.so \
    libinference_engine_s.a          \
    libinference_engine.so           \
    libmkldnn.a                      \
    libMKLDNNPlugin.so               \
    libpugixml.*                     \
    libstb_image.a

echo "remove header files ......"

INCLUDE_PATH=/usr/include/dldt
rm -rf $INCLUDE_PATH/*

echo "clean up done ......"
