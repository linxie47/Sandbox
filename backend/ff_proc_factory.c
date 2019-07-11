/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_proc_factory.h"
#include <libavutil/avassert.h>

static void infer_detect_metadata_buffer_free(void *opaque, uint8_t *data) {
    BBoxesArray *bboxes = ((InferDetectionMeta *)data)->bboxes;

    if (bboxes) {
        int i;
        for (i = 0; i < bboxes->num; i++) {
            InferDetection *p = bboxes->bbox[i];
            if (p->label_buf)
                av_buffer_unref(&p->label_buf);
            av_freep(&p);
        }
        av_free(bboxes->bbox);
        av_freep(&bboxes);
    }

    av_free(data);
}

static void ExtractBoundingBoxes(const OutputBlobArray *blob_array, InferenceROIArray *infer_roi_array,
                                 ModelOutputPostproc *model_postproc, const char *model_name,
                                 const FFBaseInference *ff_base_inference) {
    for (int n = 0; n < blob_array->num_blobs; n++) {
        AVBufferRef *labels = NULL;
        BBoxesArray **boxes = NULL;
        OutputBlobContext *ctx = blob_array->output_blobs[n];
        const OutputBlobMethod *blob = ctx->output_blob_method;

        const char *layer_name = blob->GetOutputLayerName(ctx);
        const float *detections = (const float *)blob->GetData(ctx);

        Dimensions *dim = blob->GetDims(ctx);
        IILayout layout = blob->GetLayout(ctx);

        int object_size = 0;
        int max_proposal_count = 0;
        float threshold = ff_base_inference->param.threshold;

        switch (layout) {
        case II_LAYOUT_NCHW:
            object_size = dim->dims[3];
            max_proposal_count = dim->dims[2];
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "Unsupported output layout, boxes won't be extracted\n");
            continue;
        }

        if (object_size != 7) { // SSD DetectionOutput format
            av_log(NULL, AV_LOG_ERROR, "Unsupported output dimensions, boxes won't be extracted\n");
            continue;
        }

        if (ff_base_inference->param.model_postproc) {
            int idx = findModelPostProcByName(ff_base_inference->param.model_postproc, layer_name);
            if (idx != MAX_MODEL_OUTPUT)
                labels = ff_base_inference->param.model_postproc->procs[idx].labels;
        }

        boxes = (BBoxesArray **)av_mallocz_array(infer_roi_array->num_infer_ROIs, sizeof(boxes[0]));
        av_assert0(boxes);

        for (int i = 0; i < max_proposal_count; i++) {
            int image_id = (int)detections[i * object_size + 0];
            int label_id = (int)detections[i * object_size + 1];
            float confidence = detections[i * object_size + 2];
            float x_min = detections[i * object_size + 3];
            float y_min = detections[i * object_size + 4];
            float x_max = detections[i * object_size + 5];
            float y_max = detections[i * object_size + 6];
            if (image_id < 0 || (size_t)image_id >= infer_roi_array->num_infer_ROIs)
                break;

            if (confidence < threshold)
                continue;

            if (boxes[image_id] == NULL) {
                boxes[image_id] = (BBoxesArray *)av_mallocz(sizeof(*boxes[image_id]));
                av_assert0(boxes[image_id]);
            }

            /* using integer to represent */
            {
                FFVideoRegionOfInterestMeta *roi = &infer_roi_array->infer_ROIs[image_id]->roi;
                InferDetection *new_bbox = (InferDetection *)av_mallocz(sizeof(*new_bbox));

                int width = roi->w;
                int height = roi->h;
                int ix_min = (int)(x_min * width + 0.5);
                int iy_min = (int)(y_min * height + 0.5);
                int ix_max = (int)(x_max * width + 0.5);
                int iy_max = (int)(y_max * height + 0.5);

                if (ix_min < 0)
                    ix_min = 0;
                if (iy_min < 0)
                    iy_min = 0;
                if (ix_max > width)
                    ix_max = width;
                if (iy_max > height)
                    iy_max = height;

                av_assert0(new_bbox);
                new_bbox->x_min = ix_min;
                new_bbox->y_min = iy_min;
                new_bbox->x_max = ix_max;
                new_bbox->y_max = iy_max;
                new_bbox->confidence = confidence;
                new_bbox->label_id = label_id;
                if (labels)
                    new_bbox->label_buf = av_buffer_ref(labels);
                av_dynarray_add(&boxes[image_id]->bbox, &boxes[image_id]->num, new_bbox);
            }
        }

        for (int n = 0; n < infer_roi_array->num_infer_ROIs; n++) {
            AVBufferRef *ref;
            AVFrame *av_frame;
            AVFrameSideData *sd;

            InferDetectionMeta *detect_meta = (InferDetectionMeta *)av_malloc(sizeof(*detect_meta));
            av_assert0(detect_meta);

            detect_meta->bboxes = boxes[n];

            ref = av_buffer_create((uint8_t *)detect_meta, sizeof(*detect_meta), &infer_detect_metadata_buffer_free,
                                   NULL, 0);
            if (ref == NULL) {
                infer_detect_metadata_buffer_free(NULL, (uint8_t *)detect_meta);
                av_assert0(ref);
            }

            av_frame = infer_roi_array->infer_ROIs[n]->frame;
            // add meta data to side data
            sd = av_frame_new_side_data_from_buf(av_frame, AV_FRAME_DATA_INFERENCE_DETECTION, ref);
            if (sd == NULL) {
                av_buffer_unref(&ref);
                av_assert0(sd);
            }
            av_log(NULL, AV_LOG_DEBUG, "av_frame:%p sd:%d\n", av_frame, av_frame->nb_side_data);
        }

        av_free(boxes);
    }
}

static void ExtractYOLOV3BoundingBoxes(const OutputBlobArray *output_blobs, InferenceROIArray *infer_roi_array,
                                       ModelOutputPostproc *model_postproc, const char *model_name,
                                       const FFBaseInference *ff_base_inference) {
}

PostProcFunction getPostProcFunctionByName(const char *name, const char *model) {
    if (name == NULL || model == NULL)
        return NULL;

    if (!strcmp(name, "ie_detect")) {
        if (strstr(model, "yolo"))
            return (PostProcFunction)ExtractYOLOV3BoundingBoxes;
        else
            return (PostProcFunction)ExtractBoundingBoxes;
    }
    return NULL;
}

int findModelPostProcByName(ModelOutputPostproc *model_postproc, const char *layer_name) {
    int proc_id;
    // search model postproc
    for (proc_id = 0; proc_id < MAX_MODEL_OUTPUT; proc_id++) {
        char *proc_layer_name = model_postproc->procs[proc_id].layer_name;
        // skip this output process
        if (!proc_layer_name)
            continue;
        if (!strcmp(layer_name, proc_layer_name))
            return proc_id;
    }

    av_log(NULL, AV_LOG_DEBUG, "Could not find proc:%s\n", layer_name);
    return proc_id;
}