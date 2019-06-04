#include "proc_factory.h"

extern "C" {
    #include <libavutil/mem.h>
}

static void infer_detect_metadata_buffer_free(void *opaque, uint8_t *data)
{
    BBoxesArray *bboxes = ((InferDetectionMeta *)data)->bboxes;

    if (bboxes)
    {
        int i;
        for (i = 0; i < bboxes->num; i++)
        {
            InferDetection *p = bboxes->bbox[i];
            if (p->label_buf)
                av_buffer_unref(&p->label_buf);
            av_freep(&p);
        }
        av_freep(&bboxes);
    }

    av_free(data);
}

void ExtractBoundingBoxes(const std::map<std::string, InferenceBackend::OutputBlob::Ptr> &output_blobs,
                          std::vector<InferenceROI> frames, const std::map<std::string, void *> &model_proc,
                          const char *model_name, FFBaseInference *gva_base_inference)
{
    for (const auto &blob_desc : output_blobs)
    {
        InferenceBackend::OutputBlob::Ptr blob = blob_desc.second;
        if (blob == nullptr)
            throw std::runtime_error("Blob is empty. Cannot access to null object.");

        // const std::string &layer_name = blob_desc.first;

        const float *detections = (const float *)blob->GetData();
        auto dims = blob->GetDims();
        auto layout = blob->GetLayout();

        av_log(NULL, AV_LOG_DEBUG, "DIMS:\n");
        for (auto dim = dims.begin(); dim < dims.end(); dim++)
        {
            av_log(NULL, AV_LOG_DEBUG, "\t%lu\n", *dim);
        }

        int object_size = 0;
        int max_proposal_count = 0;
        switch (layout)
        {
        case InferenceBackend::OutputBlob::Layout::NCHW:
            object_size = dims[3];
            max_proposal_count = dims[2];
            break;
        default:
            av_log(NULL, AV_LOG_ERROR, "Unsupported output layout, boxes won't be extracted\n");
            continue;
        }
        if (object_size != 7)
        {   // SSD DetectionOutput format
            av_log(NULL, AV_LOG_ERROR, "Unsupported output dimensions, boxes won't be extracted\n");
            continue;
        }
#if 0
        // Read labels and roi_scale from GstStructure config
        GValueArray *labels = nullptr;
        double roi_scale = 1.0;
        auto post_proc = model_proc.find(layer_name);
        if (post_proc != model_proc.end()) {
            gst_structure_get_array(post_proc->second, "labels", &labels);
            gst_structure_get_double(post_proc->second, "roi_scale", &roi_scale);
        }
#endif
        float threshold = 0.5; //((GstGvaDetect *)gva_base_inference)->threshold;

        BBoxesArray **boxes = (BBoxesArray **)av_mallocz_array(frames.size(), sizeof(boxes[0]));
        if (boxes == nullptr)
            throw std::runtime_error("Cannot alloc space for bounding boxes.");

        for (int i = 0; i < max_proposal_count; i++)
        {
            int image_id = (int)detections[i * object_size + 0];
            int label_id = (int)detections[i * object_size + 1];
            float confidence = detections[i * object_size + 2];
            float x_min = detections[i * object_size + 3];
            float y_min = detections[i * object_size + 4];
            float x_max = detections[i * object_size + 5];
            float y_max = detections[i * object_size + 6];
            if (image_id < 0 || (size_t)image_id >= frames.size())
                break;

            if (confidence < threshold)
                continue;

            if (boxes[image_id] == nullptr)
            {
                boxes[image_id] = (BBoxesArray *)av_mallocz(sizeof(*boxes[image_id]));
                if (boxes[image_id] == nullptr)
                    throw std::runtime_error("Cannot alloc space for bounding boxes.");
            }

#if 0
            const char *label = NULL;
            if (labels && label_id >= 0 && label_id < (int)labels->n_values) {
                label = g_value_get_string(labels->values + label_id);
            }
#endif
            /* using integer to represent
            int width = frames[image_id].roi.w;
            int height = frames[image_id].roi.h;
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
            */
            InferDetection *new_bbox = (InferDetection *)av_mallocz(sizeof(*new_bbox));
            if (new_bbox == nullptr)
            {
                av_log(NULL, AV_LOG_ERROR, "Could not alloc bbox!\n");
                break;
            }

            new_bbox->x_min = x_min;
            new_bbox->y_min = y_min;
            new_bbox->x_max = x_max;
            new_bbox->y_max = y_max;
            new_bbox->confidence = confidence;
            new_bbox->label_buf = nullptr; // TODO
            new_bbox->label_id = label_id;

            av_dynarray_add(&boxes[image_id]->bbox, &boxes[image_id]->num, new_bbox);
        }

        for (size_t n = 0; n < frames.size(); n++)
        {
            InferDetectionMeta *detect_meta = (InferDetectionMeta *)av_malloc(sizeof(*detect_meta));
            if (!detect_meta)
                throw std::runtime_error("Cannot alloc space for detection meta.");

            detect_meta->bboxes = boxes[n];

            AVBufferRef *ref = av_buffer_create((uint8_t *)detect_meta, sizeof(*detect_meta),
                                                &infer_detect_metadata_buffer_free, NULL, 0);
            if (ref == nullptr)
            {
                infer_detect_metadata_buffer_free(NULL, (uint8_t *)detect_meta);
                throw std::runtime_error("Create buffer ref failed.");
            }

            AVFrame *av_frame = frames[n].frame;
            // add meta data to side data
            AVFrameSideData *sd = av_frame_new_side_data_from_buf(av_frame, AV_FRAME_DATA_INFERENCE_DETECTION, ref);
            if (sd == nullptr)
            {
                av_buffer_unref(&ref);
                throw std::runtime_error("Could not add new side data");
            }
            av_log(NULL, AV_LOG_DEBUG, "av_frame:%p sd:%d\n", av_frame, av_frame->nb_side_data);
        }

        av_free(boxes);
    }
}
