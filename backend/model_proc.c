/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "model_proc.h"
#include "libavutil/avassert.h"
#include "logger.h"
#include <json-c/json.h>

// helper functions
static void infer_labels_dump(uint8_t *data) {
    int i;
    LabelsArray *labels = (LabelsArray *)data;
    printf("labels: ");
    for (i = 0; i < labels->num; i++)
        printf("%s ", labels->label[i]);
    printf("\n");
}

void infer_labels_buffer_free(void *opaque, uint8_t *data) {
    int i;
    LabelsArray *labels = (LabelsArray *)data;

    for (i = 0; i < labels->num; i++)
        av_freep(&labels->label[i]);

    av_free(labels->label);

    av_free(data);
}

int model_proc_get_file_size(FILE *fp) {
    int file_size, current_pos;

    if (!fp)
        return -1;

    current_pos = ftell(fp);

    if (fseek(fp, 0, SEEK_END)) {
        fprintf(stderr, "Couldn't seek to the end of feature file.\n");
        return -1;
    }

    file_size = ftell(fp);

    fseek(fp, current_pos, SEEK_SET);

    return file_size;
}
/*
 * model proc parsing functions using JSON-c
 */
void *model_proc_read_config_file(const char *path) {
    int n, file_size;
    json_object *proc_config = NULL;
    uint8_t *proc_json = NULL;
    json_tokener *tok = NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "File open error:%s\n", path);
        return NULL;
    }

    file_size = model_proc_get_file_size(fp);

    proc_json = av_mallocz(file_size + 1);
    if (!proc_json)
        goto end;

    n = fread(proc_json, file_size, 1, fp);

    UNUSED(n);

    tok = json_tokener_new();
    proc_config = json_tokener_parse_ex(tok, proc_json, file_size);
    if (proc_config == NULL) {
        enum json_tokener_error jerr;
        jerr = json_tokener_get_error(tok);
        fprintf(stderr, "Error before: %s\n", json_tokener_error_desc(jerr));
        goto end;
    }

end:
    if (proc_json)
        av_freep(&proc_json);
    if (tok)
        json_tokener_free(tok);
    fclose(fp);
    return proc_config;
}

void model_proc_load_default_config_file(ModelInputPreproc *preproc, ModelOutputPostproc *postproc) {
    if (preproc) {
        /*
         * format is a little tricky, an ideal input format for IE is BGR planer
         * however, neither soft csc nor hardware vpp could support that format.
         * Here, we set a close soft format. The actual one coverted before sent
         * to IE will be decided by user config and hardware vpp used or not.
         */
        preproc->color_format = AV_PIX_FMT_BGR24;
        preproc->layer_name = NULL;
    }

    if (postproc) {
        // do nothing
    }
}

int model_proc_parse_input_preproc(const void *json, ModelInputPreproc *m_preproc) {
    json_object *jvalue, *preproc, *color, *layer, *object_class;
    int ret;

    ret = json_object_object_get_ex((json_object *)json, "input_preproc", &preproc);
    if (!ret) {
        VAII_DEBUG("No input_preproc.\n");
        return 0;
    }

    // not support multiple inputs yet
    av_assert0(json_object_array_length(preproc) <= 1);

    jvalue = json_object_array_get_idx(preproc, 0);

    ret = json_object_object_get_ex(jvalue, "color_format", &color);
    if (ret) {
        if (json_object_get_string(color) == NULL)
            return -1;

        VAII_LOGI("Color Format:\"%s\"\n", json_object_get_string(color));

        if (!strcmp(json_object_get_string(color), "BGR"))
            m_preproc->color_format = AV_PIX_FMT_BGR24;
        else if (!strcmp(json_object_get_string(color), "RGB"))
            m_preproc->color_format = AV_PIX_FMT_RGB24;
        else
            return -1;
    }

    ret = json_object_object_get_ex(jvalue, "object_class", &object_class);
    if (ret) {
        if (json_object_get_string(object_class) == NULL)
            return -1;

        VAII_LOGI("Object_class:\"%s\"\n", json_object_get_string(object_class));

        m_preproc->object_class = (char *)json_object_get_string(object_class);
    }

    ret = json_object_object_get_ex(jvalue, "layer_name", &layer);
    UNUSED(layer);

    return 0;
}

// For detection, we now care labels only.
// Layer name and type can be got from output blob.
int model_proc_parse_output_postproc(const void *json, ModelOutputPostproc *m_postproc) {
    json_object *jvalue, *postproc;
    json_object *attribute, *converter, *labels, *layer, *method, *threshold;
    json_object *tensor_to_text_scale, *tensor_to_text_precision;
    int ret;
    size_t jarraylen;

    ret = json_object_object_get_ex((json_object *)json, "output_postproc", &postproc);
    if (!ret) {
        VAII_DEBUG("No output_postproc.\n");
        return 0;
    }

    jarraylen = json_object_array_length(postproc);
    av_assert0(jarraylen <= MAX_MODEL_OUTPUT);

    for (int i = 0; i < jarraylen; i++) {
        OutputPostproc *proc = &m_postproc->procs[i];
        jvalue = json_object_array_get_idx(postproc, i);

#define FETCH_STRING(var, name)                                                                                        \
    do {                                                                                                               \
        ret = json_object_object_get_ex(jvalue, #name, &var);                                                          \
        if (ret)                                                                                                       \
            proc->name = (char *)json_object_get_string(var);                                                          \
    } while (0)
#define FETCH_DOUBLE(var, name)                                                                                        \
    do {                                                                                                               \
        ret = json_object_object_get_ex(jvalue, #name, &var);                                                          \
        if (ret)                                                                                                       \
            proc->name = (double)json_object_get_double(var);                                                          \
    } while (0)
#define FETCH_INTEGER(var, name)                                                                                       \
    do {                                                                                                               \
        ret = json_object_object_get_ex(jvalue, #name, &var);                                                          \
        if (ret)                                                                                                       \
            proc->name = (int)json_object_get_int(var);                                                                \
    } while (0)

        FETCH_STRING(layer, layer_name);
        FETCH_STRING(method, method);
        FETCH_STRING(attribute, attribute_name);
        FETCH_STRING(converter, converter);

        FETCH_DOUBLE(threshold, threshold);
        FETCH_DOUBLE(tensor_to_text_scale, tensor_to_text_scale);

        FETCH_INTEGER(tensor_to_text_precision, tensor_to_text_precision);

        // handle labels
        ret = json_object_object_get_ex(jvalue, "labels", &labels);
        if (ret) {
            json_object *label;
            size_t labels_num = json_object_array_length(labels);

            if (labels_num > 0) {
                AVBufferRef *ref = NULL;
                LabelsArray *larray = av_mallocz(sizeof(*larray));

                if (!larray)
                    return AVERROR(ENOMEM);

                for (int i = 0; i < labels_num; i++) {
                    char *copy = NULL;
                    label = json_object_array_get_idx(labels, i);
                    copy = av_strdup(json_object_get_string(label));
                    av_dynarray_add(&larray->label, &larray->num, copy);
                }

                ref = av_buffer_create((uint8_t *)larray, sizeof(*larray), &infer_labels_buffer_free, NULL, 0);

                proc->labels = ref;

                if (ref)
                    infer_labels_dump(ref->data);
            }
        }
    }

#undef FETCH_STRING
#undef FETCH_DOUBLE
#undef FETCH_INTEGER

    return 0;
}

void model_proc_release_model_proc(const void *json, ModelInputPreproc *preproc, ModelOutputPostproc *postproc) {
    size_t index = 0;

    if (!json)
        return;

    if (postproc) {
        for (index = 0; index < MAX_MODEL_OUTPUT; index++) {
            if (postproc->procs[index].labels)
                av_buffer_unref(&postproc->procs[index].labels);
        }
    }

    json_object_put((json_object *)json);
}