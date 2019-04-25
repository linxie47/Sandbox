#!/bin/bash

# Copyright (c) 2019 Intel Corporation
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

if [[ -z "${INTEL_CVSDK_DIR}" ]]; then
    printf "\nINTEL_CVSDK_DIR environment variable is not set. Trying to find setupvars.sh to set it. \n"

    setvars_path=/opt/intel/computer_vision_sdk
    if [ -e "$setvars_path/inference_engine/bin/setvars.sh" ]; then # for Intel Deep Learning Deployment Toolkit package
        setvars_path="$setvars_path/inference_engine/bin/setvars.sh"
    elif [ -e "$setvars_path/bin/setupvars.sh" ]; then # for OpenVINO package
        setvars_path="$setvars_path/bin/setupvars.sh"
    else
        printf "Error: setupvars.sh is not found in hardcoded paths. \n\n"
        exit 1
    fi
    if ! source $setvars_path ; then
        printf "Unable to run ./setupvars.sh. Please check its presence. \n\n"
        exit 1
    fi
fi

if [[ -f /etc/centos-release ]]; then
    IE_PLUGINS_PATH=$INTEL_CVSDK_DIR/deployment_tools/inference_engine/lib/centos_7.4/intel64
elif [[ -f /etc/lsb-release ]]; then
    UBUNTU_VERSION=$(lsb_release -r -s)
    if [[ $UBUNTU_VERSION = "16.04" ]]; then
        IE_PLUGINS_PATH=$INTEL_CVSDK_DIR/deployment_tools/inference_engine/lib/ubuntu_16.04/intel64
    elif [[ $UBUNTU_VERSION = "18.04" ]]; then
        IE_PLUGINS_PATH=$INTEL_CVSDK_DIR/deployment_tools/inference_engine/lib/ubuntu_18.04/intel64
    elif cat /etc/lsb-release | grep -q "Yocto" ; then
        IE_PLUGINS_PATH=$INTEL_CVSDK_DIR/deployment_tools/inference_engine/lib/ubuntu_16.04/intel64
    fi
elif [[ -f /etc/os-release ]]; then
    OS_NAME=$(lsb_release -i -s)
    OS_VERSION=$(lsb_release -r -s)
    OS_VERSION=${OS_VERSION%%.*}
    if [ $OS_NAME = "Raspbian" ] && [ $OS_VERSION = "9" ]; then
        IE_PLUGINS_PATH=$INTEL_CVSDK_DIR/deployment_tools/inference_engine/lib/raspbian_9/armv7l
    fi
fi

echo "find InferenceEngine_DIR: $InferenceEngine_DIR, IE_PLUGINS_PATH: $IE_PLUGINS_PATH"

if ! command -v cmake &>/dev/null; then
    printf "\n\nCMAKE is not installed. It is required to build Inference Engine. Please install it. \n\n"
    exit 1
fi

sh ./clean_up.sh

BUILD_TYPE=Release
INSTALL_DIR="/usr/local"
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && make -j16 && sudo make install

# copy IE headers and libraries
cp -r $InferenceEngine_DIR/../include/* "$INSTALL_DIR/include/dldt" && cp -r $IE_PLUGINS_PATH/* "$INSTALL_DIR/lib"
