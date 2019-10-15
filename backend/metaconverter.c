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

#include "metaconverter.h"
#include "libavutil/avassert.h"
#include "ff_base_inference.h"
#include "logger.h"

int convert_roi_detection(json_object *info_object, AVFrame *frame) {
    AVFrameSideData *sd;
    InferDetectionMeta *d_meta;
    BBoxesArray *boxes = NULL;

    if (!(sd = av_frame_get_side_data(frame, AV_FRAME_DATA_INFERENCE_DETECTION)))
        return 0;

    d_meta = (InferDetectionMeta *)sd->data;

    if (d_meta) {
        json_object *resolution_object, *objects;

        boxes = d_meta->bboxes;
        if (boxes == NULL)
            return 0;

        resolution_object = json_object_new_object();
        objects = json_object_new_array();

        json_object_object_add(resolution_object, "width", json_object_new_int(frame->width));
        json_object_object_add(resolution_object, "height", json_object_new_int(frame->height));
        json_object_object_add(info_object, "resolution", resolution_object);

        for (size_t i = 0; i < boxes->num; i++) {
            LabelsArray *array;
            int label_id;
            json_object *box_object, *bounding_box, *detection_object;

            if (!boxes->bbox[i]->label_buf) {
                VAII_ERROR("No model proc for this model\n");
                break;
            }

            array = (LabelsArray *)boxes->bbox[i]->label_buf->data;
            label_id = boxes->bbox[i]->label_id;
            box_object = json_object_new_object();
            bounding_box = json_object_new_object();
            detection_object = json_object_new_object();

            json_object_object_add(box_object, "x_min", json_object_new_int(boxes->bbox[i]->x_min));
            json_object_object_add(box_object, "y_min", json_object_new_int(boxes->bbox[i]->y_min));
            json_object_object_add(box_object, "x_max", json_object_new_int(boxes->bbox[i]->x_max));
            json_object_object_add(box_object, "y_max", json_object_new_int(boxes->bbox[i]->y_max));

            json_object_object_add(bounding_box, "bounding_box", box_object);
            json_object_object_add(detection_object, "detection", bounding_box);

            json_object_object_add(detection_object, "object_id", json_object_new_int(i));
            json_object_object_add(detection_object, "label", json_object_new_string(array->label[label_id]));
            json_object_object_add(detection_object, "label_id", json_object_new_int(label_id));
            json_object_object_add(detection_object, "confidence", json_object_new_double((double)boxes->bbox[i]->confidence));

            json_object_array_add(objects, detection_object);
        }

        json_object_object_add(info_object, "Detection_Objects", objects);
    }

    return 1;
}

int convert_roi_tensor(json_object *info_object, AVFrame *frame) {
    AVFrameSideData *sd;
    InferClassificationMeta *c_meta;

    if (!(sd = av_frame_get_side_data(frame, AV_FRAME_DATA_INFERENCE_CLASSIFICATION)))
        return 0;

    c_meta = (InferClassificationMeta *)sd->data;

    if (c_meta) {
        int i;
        const int meta_num = c_meta->c_array->num;
        json_object *objects = json_object_new_array();

        for (i = 0; i < meta_num; i++) {
            InferClassification *c = c_meta->c_array->classifications[i];
            json_object *classify_object, *item_object;

            if (!strcmp(c->name, "default")) {
                VAII_ERROR("No model proc for this model\n");
                break;
            }

            classify_object = json_object_new_object();
            item_object = json_object_new_object();

            json_object_object_add(item_object, "detect_id", json_object_new_int(c->detect_id));

            if (!strcmp(c->name, "age")) {
                json_object_object_add(item_object, "value", json_object_new_double((double)c->value));
            }

            if (!strcmp(c->name, "gender") || !strcmp(c->name, "emotion") || !strcmp(c->name, "color") ||
                        !strcmp(c->name, "type") || !strcmp(c->name, "face_id")) {
                LabelsArray *array = (LabelsArray *)c->label_buf->data;
                json_object_object_add(item_object, "label", json_object_new_string(array->label[c->label_id]));
                json_object_object_add(item_object, "label_id", json_object_new_int(c->label_id));
                json_object_object_add(item_object, "confidence", json_object_new_double((double)c->confidence));
            }

            if (!strcmp(c->name, "license_plate")) {
                json_object_object_add(item_object, "label", json_object_new_string(c->attributes));
            }

            if (!strcmp(c->name, "person-attributes")) {
                json_object_object_add(item_object, "confidence", json_object_new_double((double)c->confidence));
                json_object_object_add(item_object, "label", json_object_new_string(c->attributes));
            }

            json_object_object_add(classify_object, c->name, item_object);
            json_object_array_add(objects, classify_object);
        }

        json_object_object_add(info_object, "Classification_Objects", objects);
    }

    return 1;
}

int tensors_to_file(AVFilterContext *ctx, AVFrame *frame, json_object *info_object) {
    AVFrameSideData *sd;
    MetaConvertContext *s = ctx->priv;
    InferClassificationMeta *c_meta;

    static uint32_t frame_num = 0;

    if (!(sd = av_frame_get_side_data(frame, AV_FRAME_DATA_INFERENCE_CLASSIFICATION)))
        return 0;

    c_meta = (InferClassificationMeta *)sd->data;

    if (c_meta) {
        int i;
        uint32_t index = 0;
        char filename[1024] = {0};
        const int meta_num = c_meta->c_array->num;
        for (i = 0; i < meta_num; i++) {
            FILE *f = NULL;
            InferClassification *c = c_meta->c_array->classifications[i];
            //TODO:check model and layer
            if (!c->tensor_buf || !c->tensor_buf->data)
                continue;

            snprintf(filename, sizeof(filename), "%s/%s_frame_%u_idx_%u.tensor", s->location,
                    s->method, frame_num, index);
            f = fopen(filename, "wb");
            if (!f) {
                VAII_LOGW("Failed to open/create file: %s\n", filename);
            } else {
                fwrite(c->tensor_buf->data, sizeof(float), c->tensor_buf->size / sizeof(float), f);
                fclose(f);
            }
            index++;
        }
    }

    frame_num++;

    return 0;
}

int detection_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object) {
    int ret;

    ret = convert_roi_detection(info_object, frame);
    return ret;
}

int classification_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object) {
    int ret;

    ret = convert_roi_tensor(info_object, frame);
    return ret;
}

int all_to_json(AVFilterContext *ctx, AVFrame *frame, json_object *info_object) {
    int ret1, ret2;

    ret1 = convert_roi_detection(info_object, frame);
    ret2 = convert_roi_tensor(info_object, frame);
    return (ret1 + ret2);
}
