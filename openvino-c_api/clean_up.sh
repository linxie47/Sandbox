#!/bin/bash

echo "remove libraries......"

for LIBRAY_PATH in /usr/lib/x86_64-linux-gnu /usr/local/lib ; do \
    cd $LIBRAY_PATH;
    rm -rf \
    cldnn_global_custom_kernels    \
    hddl_perfcheck                 \
    myriad_*                       \
    mvnc                           \
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
    libhddlapi.so                  \
    libHDDLPlugin.so               \
    libmyriadPlugin.so             \
    libinference_engine_c_wrapper.* \
    libinference_engine_s.a          \
    libinference_engine.so           \
    libmkldnn.a                      \
    libMKLDNNPlugin.so               \
    libpugixml.*                     \
    libstb_image.a;                  \
    rm -rf $LIBRAY_PATH/pkgconfig/dldt.pc;
done;


echo "remove header files ......"

for INCLUDE_PATH in /usr/include/dldt /usr/local/include/dldt; do \
    rm -rf $INCLUDE_PATH;
done;

echo "clean up done ......"
