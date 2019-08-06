/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#pragma once

#include "ff_base_inference.h"

#define UNUSED(x) (void)(x)

void *model_proc_read_config_file(const char *path);

void model_proc_load_default_config_file(ModelInputPreproc *preproc, ModelOutputPostproc *postproc);

int model_proc_parse_input_preproc(const void *json, ModelInputPreproc *m_preproc);

int model_proc_parse_output_postproc(const void *json, ModelOutputPostproc *m_postproc);

void model_proc_release_model_proc(const void *json, ModelInputPreproc *preproc, ModelOutputPostproc *postproc);

int model_proc_get_file_size(FILE *fp);