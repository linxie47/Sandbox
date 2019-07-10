/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "ff_base_inference.h"

PostProcFunction getPostProcFunctionByName(const char *name, const char *model);

int findModelPostProcByName(ModelOutputPostproc *model_postproc, const char *layer_name);