/*******************************************************************************
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 ******************************************************************************/

#include "ff_proc_factory.h"
#include <libavutil/avassert.h>
#include <math.h>

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

static void infer_classify_metadata_buffer_free(void *opaque, uint8_t *data) {
    int i;
    InferClassificationMeta *meta = (InferClassificationMeta *)data;
    ClassifyArray *classes = meta->c_array;

    if (classes) {
        for (i = 0; i < classes->num; i++) {
            InferClassification *c = classes->classifications[i];
            av_buffer_unref(&c->label_buf);
            av_buffer_unref(&c->tensor_buf);
            av_freep(&c);
        }
        av_free(classes->classifications);
        av_freep(&classes);
    }

    av_free(data);
}

static int get_unbatched_size_in_bytes(OutputBlobContext *blob_ctx, size_t batch_size) {
    const OutputBlobMethod *blob = blob_ctx->output_blob_method;
    size_t size;
    Dimensions *dim = blob->GetDims(blob_ctx);

    if (dim->dims[0] != batch_size) {
        av_log(NULL, AV_LOG_ERROR, "Blob last dimension should be equal to batch size");
        av_assert0(0);
    }
    size = dim->dims[1];
    for (int i = 2; i < dim->num_dims; i++) {
        size *= dim->dims[i];
    }
    switch (blob->GetPrecision(blob_ctx)) {
    case II_FP32:
        size *= sizeof(float);
        break;
    case II_U8:
    default:
        break;
    }
    return size;
}

typedef struct DetectionObject {
    int xmin, ymin, xmax, ymax, class_id;
    float confidence;
} DetectionObject;

typedef struct DetectionObjectArray {
    DetectionObject **objects;
    int num_detection_objects;
} DetectionObjectArray;

static void DetectionObjectInit(DetectionObject *this, double x, double y, double h, double w, int class_id,
                                float confidence, float h_scale, float w_scale) {
    this->xmin = (int)((x - w / 2) * w_scale);
    this->ymin = (int)((y - h / 2) * h_scale);
    this->xmax = (int)(this->xmin + w * w_scale);
    this->ymax = (int)(this->ymin + h * h_scale);
    this->class_id = class_id;
    this->confidence = confidence;
}

static int DetectionObjectCompare(const void *p1, const void *p2) {
    return (*(DetectionObject **)p1)->confidence > (*(DetectionObject **)p2)->confidence ? 1 : -1;
}

static double IntersectionOverUnion(const DetectionObject *box_1, const DetectionObject *box_2) {
    double width_of_overlap_area = fmin(box_1->xmax, box_2->xmax) - fmax(box_1->xmin, box_2->xmin);
    double height_of_overlap_area = fmin(box_1->ymax, box_2->ymax) - fmax(box_1->ymin, box_2->ymin);
    double area_of_overlap, area_of_union;
    double box_1_area = (box_1->ymax - box_1->ymin) * (box_1->xmax - box_1->xmin);
    double box_2_area = (box_2->ymax - box_2->ymin) * (box_2->xmax - box_2->xmin);

    if (width_of_overlap_area < 0 || height_of_overlap_area < 0)
        area_of_overlap = 0;
    else
        area_of_overlap = width_of_overlap_area * height_of_overlap_area;
    area_of_union = box_1_area + box_2_area - area_of_overlap;
    return area_of_overlap / area_of_union;
}

static int EntryIndex(int side, int lcoords, int lclasses, int location, int entry) {
    int n = location / (side * side);
    int loc = location % (side * side);
    return n * side * side * (lcoords + lclasses + 1) + entry * side * side + loc;
}

#define YOLOV3_INPUT_SIZE 320 // TODO: add interface to get this info

static void ParseYOLOV3Output(OutputBlobContext *blob_ctx, int image_width, int image_height,
                              DetectionObjectArray *objects, const FFBaseInference *base) {
    const OutputBlobMethod *blob = blob_ctx->output_blob_method;
    Dimensions *dim = blob->GetDims(blob_ctx);
    const int out_blob_h = (int)dim->dims[2];
    const int out_blob_w = (int)dim->dims[3];
    const int coords = 4, num = 3, classes = 80;
    const float anchors[] = {10.0, 13.0, 16.0,  30.0,  33.0, 23.0,  30.0,  61.0,  62.0,
                             45.0, 59.0, 119.0, 116.0, 90.0, 156.0, 198.0, 373.0, 326.0};
    int side = out_blob_h;
    int anchor_offset = 0;
    int side_square = side * side;
    const float *output_blob = (const float *)blob->GetData(blob_ctx);

    av_assert0(out_blob_h == out_blob_w);
    switch (side) {
    case 13:
    case 10:
    case 19:
        anchor_offset = 2 * 6;
        break;
    case 26:
    case 20:
    case 38:
        anchor_offset = 2 * 3;
        break;
    case 52:
    case 40:
    case 76:
        anchor_offset = 2 * 0;
        break;
    default:
        av_log(NULL, AV_LOG_ERROR, "Invalid output size\n");
        return;
    }
    for (int i = 0; i < side_square; ++i) {
        int row = i / side;
        int col = i % side;
        for (int n = 0; n < num; ++n) {
            int obj_index = EntryIndex(side, coords, classes, n * side * side + i, coords);
            int box_index = EntryIndex(side, coords, classes, n * side * side + i, 0);
            double x, y, width, height;

            float scale = output_blob[obj_index];
            float threshold = base->param.threshold;
            if (scale < threshold)
                continue;

            x = (col + output_blob[box_index + 0 * side_square]) / side * YOLOV3_INPUT_SIZE;
            y = (row + output_blob[box_index + 1 * side_square]) / side * YOLOV3_INPUT_SIZE;
            width = exp(output_blob[box_index + 2 * side_square]) * anchors[anchor_offset + 2 * n];
            height = exp(output_blob[box_index + 3 * side_square]) * anchors[anchor_offset + 2 * n + 1];

            for (int j = 0; j < classes; ++j) {
                int class_index = EntryIndex(side, coords, classes, n * side_square + i, coords + 1 + j);
                float prob = scale * output_blob[class_index];
                DetectionObject *obj = NULL;
                if (prob < threshold)
                    continue;
                obj = av_mallocz(sizeof(*obj));
                av_assert0(obj);
                DetectionObjectInit(obj, x, y, height, width, j, prob,
                                    (float)(image_height) / (float)(YOLOV3_INPUT_SIZE),
                                    (float)(image_width) / (float)(YOLOV3_INPUT_SIZE));
                av_dynarray_add(&objects->objects, &objects->num_detection_objects, obj);
            }
        }
    }
}

static void ExtractYOLOV3BoundingBoxes(const OutputBlobArray *blob_array, InferenceROIArray *infer_roi_array,
                                       ModelOutputPostproc *model_postproc, const char *model_name,
                                       const FFBaseInference *ff_base_inference) {
    DetectionObjectArray obj_array = {};
    BBoxesArray *boxes;
    AVBufferRef *ref;
    AVFrame *av_frame;
    AVFrameSideData *side_data;
    InferDetectionMeta *detect_meta;

    av_assert0(blob_array->num_blobs == 3);           // This accepts networks with three layers
    av_assert0(infer_roi_array->num_infer_ROIs == 1); // YoloV3 cannot support batch mode

    for (int n = 0; n < blob_array->num_blobs; n++) {
        OutputBlobContext *blob_ctx = blob_array->output_blobs[n];
        av_assert0(blob_ctx);
        ParseYOLOV3Output(blob_ctx, infer_roi_array->infer_ROIs[0]->roi.w, infer_roi_array->infer_ROIs[0]->roi.h,
                          &obj_array, ff_base_inference);
    }

    qsort(obj_array.objects, obj_array.num_detection_objects, sizeof(DetectionObject *), DetectionObjectCompare);
    for (int i = 0; i < obj_array.num_detection_objects; ++i) {
        DetectionObjectArray *d = &obj_array;
        if (d->objects[i]->confidence == 0)
            continue;
        for (int j = i + 1; j < d->num_detection_objects; ++j)
            if (IntersectionOverUnion(d->objects[i], d->objects[j]) >= 0.4)
                d->objects[j]->confidence = 0;
    }

    boxes = av_mallocz(sizeof(*boxes));
    av_assert0(boxes);

    for (int i = 0; i < obj_array.num_detection_objects; ++i) {
        InferDetection *new_bbox = NULL;
        DetectionObject *object = obj_array.objects[i];
        if (object->confidence < ff_base_inference->param.threshold)
            continue;

        new_bbox = (InferDetection *)av_mallocz(sizeof(*new_bbox));
        av_assert0(new_bbox);

        new_bbox->x_min = object->xmin;
        new_bbox->y_min = object->ymin;
        new_bbox->x_max = object->xmax;
        new_bbox->y_max = object->ymax;
        new_bbox->confidence = object->confidence;
        new_bbox->label_id = object->class_id;

        //// TODO: handle label
        // if (labels)

        av_dynarray_add(&boxes->bbox, &boxes->num, new_bbox);
        av_log(NULL, AV_LOG_TRACE, "bbox %d %d %d %d\n", (int)new_bbox->x_min, (int)new_bbox->y_min,
               (int)new_bbox->x_max, (int)new_bbox->y_max);
    }

    detect_meta = av_mallocz(sizeof(*detect_meta));
    av_assert0(detect_meta);
    detect_meta->bboxes = boxes;

    ref = av_buffer_create((uint8_t *)detect_meta, sizeof(*detect_meta), &infer_detect_metadata_buffer_free, NULL, 0);
    if (!ref) {
        infer_detect_metadata_buffer_free(NULL, (uint8_t *)detect_meta);
        av_log(NULL, AV_LOG_ERROR, "Create buffer ref failed.\n");
        av_assert0(0);
    }

    av_frame = infer_roi_array->infer_ROIs[0]->frame;
    // add meta data to side data
    side_data = av_frame_new_side_data_from_buf(av_frame, AV_FRAME_DATA_INFERENCE_DETECTION, ref);
    av_assert0(side_data);

    // free all detection objects
    for (int n = 0; n < obj_array.num_detection_objects; n++)
        av_free(obj_array.objects[n]);
    av_free(obj_array.objects);
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

        if (model_postproc) {
            int idx = findModelPostProcByName(model_postproc, layer_name);
            if (idx != MAX_MODEL_OUTPUT)
                labels = model_postproc->procs[idx].labels;
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

static int CreateNewClassifySideData(AVFrame *frame, InferClassificationMeta *classify_meta) {
    AVBufferRef *ref;
    AVFrameSideData *new_sd;
    ref = av_buffer_create((uint8_t *)classify_meta, sizeof(*classify_meta), &infer_classify_metadata_buffer_free, NULL,
                           0);
    if (!ref)
        return AVERROR(ENOMEM);

    // add meta data to side data
    new_sd = av_frame_new_side_data_from_buf(frame, AV_FRAME_DATA_INFERENCE_CLASSIFICATION, ref);
    if (!new_sd) {
        av_buffer_unref(&ref);
        av_log(NULL, AV_LOG_ERROR, "Could not add new side data\n");
        return AVERROR(ENOMEM);
    }

    return 0;
}

static av_cold void dump_softmax(char *name, int label_id, float conf, AVBufferRef *label_buf) {
    LabelsArray *array = (LabelsArray *)label_buf->data;

    av_log(NULL, AV_LOG_DEBUG, "CLASSIFY META - Label id:%d %s:%s Conf:%f\n", label_id, name, array->label[label_id],
           conf);
}

static av_cold void dump_tensor_value(char *name, float value) {
    av_log(NULL, AV_LOG_DEBUG, "CLASSIFY META - %s:%1.2f\n", name, value);
}

static void find_max_element_index(const float *array, int len, int *index, float *value) {
    int i;
    *index = 0;
    *value = array[0];
    for (i = 1; i < len; i++) {
        if (array[i] > *value) {
            *index = i;
            *value = array[i];
        }
    }
}

static int attributes_to_text(FFVideoRegionOfInterestMeta *meta, OutputPostproc *post_proc, void *data, Dimensions *dim,
                              InferClassification *classification, InferClassificationMeta *classify_meta) {
    const float *blob_data = (const float *)data;
    uint32_t method_max, method_compound, method_index;

    method_max = !strcmp(post_proc->method, "max");
    method_compound = !strcmp(post_proc->method, "compound");
    method_index = !strcmp(post_proc->method, "index");

    if (!blob_data)
        return -1;

    if (method_max) {
        int index;
        float confidence;
        size_t n = dim->dims[1];

        find_max_element_index(data, n, &index, &confidence);

        classification->detect_id = meta->index;
        classification->name = post_proc->attribute_name;
        classification->label_id = index;
        classification->confidence = confidence;
        classification->label_buf = av_buffer_ref(post_proc->labels);

        if (classification->label_buf) {
            dump_softmax(classification->name, classification->label_id, classification->confidence,
                         classification->label_buf);
        }
    } else if (method_compound) {
        int i;
        double threshold = 0.5;
        float confidence = 0;
        char attributes[4096] = {};
        LabelsArray *array;

        if (post_proc->threshold != 0)
            threshold = post_proc->threshold;

        array = (LabelsArray *)post_proc->labels->data;
        for (i = 0; i < array->num; i++) {
            if (blob_data[i] >= threshold)
                strncat(attributes, array->label[i], (strlen(array->label[i]) + 1));
            if (blob_data[i] > confidence)
                confidence = blob_data[i];
        }

        classification->name = post_proc->attribute_name;
        classification->confidence = confidence;

        av_log(NULL, AV_LOG_DEBUG, "Attributes: %s\n", attributes);
    } else if (method_index) {
        int i;
        char attributes[1024] = {};
        LabelsArray *array;

        array = (LabelsArray *)post_proc->labels->data;
        for (i = 0; i < array->num; i++) {
            int value = blob_data[i];
            if (value < 0 || value >= array->num)
                break;
            strncat(attributes, array->label[value], (strlen(array->label[value]) + 1));
        }

        classification->name = post_proc->attribute_name;

        av_log(NULL, AV_LOG_DEBUG, "Attributes: %s\n", attributes);
    }

    return 0;
}

static int tensor_to_text(FFVideoRegionOfInterestMeta *meta, OutputPostproc *post_proc, void *data, Dimensions *dim,
                          InferClassification *classification, InferClassificationMeta *classify_meta) {
    // InferClassification *classify;
    const float *blob_data = (const float *)data;
    double scale = 1.0;

    if (!blob_data)
        return -1;

    if (post_proc->tensor2text_scale != 0)
        scale = post_proc->tensor2text_scale;

    classification->detect_id = meta->index;
    classification->name = post_proc->attribute_name;
    classification->value = *blob_data * scale;

    dump_tensor_value(classification->name, classification->value);
    return 0;
}

static void Blob2RoiMeta(const OutputBlobArray *blob_array, InferenceROIArray *infer_roi_array,
                         ModelOutputPostproc *model_postproc, const char *model_name,
                         const FFBaseInference *ff_base_inference) {
    int batch_size = infer_roi_array->num_infer_ROIs;

    for (int n = 0; n < blob_array->num_blobs; n++) {
        OutputBlobContext *ctx = blob_array->output_blobs[n];
        const OutputBlobMethod *blob;
        const char *layer_name;
        uint8_t *data = NULL;
        int size;
        OutputPostproc *post_proc = NULL;
        Dimensions *dimensions = NULL;

        av_assert0(ctx);

        blob = ctx->output_blob_method;
        layer_name = blob->GetOutputLayerName(ctx);
        data = (uint8_t *)blob->GetData(ctx);
        dimensions = blob->GetDims(ctx);
        size = get_unbatched_size_in_bytes(ctx, batch_size);

        if (model_postproc) {
            int proc_idx = findModelPostProcByName(model_postproc, layer_name);
            if (proc_idx != MAX_MODEL_OUTPUT)
                post_proc = &model_postproc->procs[proc_idx];
        }

        for (int b = 0; b < batch_size; b++) {
            FFVideoRegionOfInterestMeta *meta = &infer_roi_array->infer_ROIs[b]->roi;
            AVFrame *av_frame = infer_roi_array->infer_ROIs[b]->frame;
            AVFrameSideData *sd = NULL;
            InferClassificationMeta *classify_meta = NULL;
            InferClassification *classification = NULL;

            sd = av_frame_get_side_data(av_frame, AV_FRAME_DATA_INFERENCE_CLASSIFICATION);
            if (sd) {
                // append to exsiting side data
                classify_meta = (InferClassificationMeta *)sd->data;
                av_assert0(classify_meta);
            } else {
                ClassifyArray *classify_array = NULL;
                // new classification meta data
                classify_meta = av_mallocz(sizeof(*classify_meta));
                classify_array = av_mallocz(sizeof(*classify_array));
                av_assert0(classify_meta && classify_array);
                classify_meta->c_array = classify_array;
                av_assert0(0 == CreateNewClassifySideData(av_frame, classify_meta));
            }

            classification = av_mallocz(sizeof(*classification));
            av_assert0(classification);
            classification->layer_name = (char *)layer_name;
            classification->model = (char *)model_name;

            if (post_proc && post_proc->converter) {
                if (!strcmp(post_proc->converter, "tensor_to_label")) {
                    attributes_to_text(meta, post_proc, (void *)(data + b * size), dimensions, classification,
                                       classify_meta);
                } else if (!strcmp(post_proc->converter, "tensor_to_text")) {
                    tensor_to_text(meta, post_proc, (void *)(data + b * size), dimensions, classification,
                                   classify_meta);
                } else {
                    av_log(NULL, AV_LOG_ERROR, "Undefined converter:%s\n", post_proc->converter);
                    break;
                }
            } else {
                // copy data to tensor buffer
                classification->detect_id = meta->index;
                classification->name = (char *)"default";
                classification->tensor_buf = av_buffer_alloc(size);
                av_assert0(classification->tensor_buf);
                memcpy(classification->tensor_buf->data, data + b * size, size);
            }

            av_dynarray_add(&classify_meta->c_array->classifications, &classify_meta->c_array->num, classification);
        }
    }
}

PostProcFunction getPostProcFunctionByName(const char *name, const char *model) {
    if (name == NULL || model == NULL)
        return NULL;

    if (!strcmp(name, "ie_detect")) {
        if (strstr(model, "yolo"))
            return (PostProcFunction)ExtractYOLOV3BoundingBoxes;
        else
            return (PostProcFunction)ExtractBoundingBoxes;
    } else if (!strcmp(name, "ie_classify")) {
        return (PostProcFunction)Blob2RoiMeta;
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