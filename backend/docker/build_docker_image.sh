#!/bin/bash
# ==============================================================================
# Copyright (C) 2018-2019 Intel Corporation
#
# SPDX-License-Identifier: MIT
# ==============================================================================

tag=$1

if [ -z "$tag" ]; then
    tag=latest
fi

BASEDIR=$(dirname "$0")
sudo docker build -f ${BASEDIR}/Dockerfile --network=host -t ff-media-analytic:$tag \
    --build-arg http_proxy=${http_proxy} \
    --build-arg https_proxy=${https_proxy} \
    ${BASEDIR}/..
