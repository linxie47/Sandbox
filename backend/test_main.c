#include "ff_base_inference.h"
#include "image_inference.h"
#include <unistd.h>

#define CONFIGS "" //"CPU_EXTENSION=xxx\nCPU_THROUGHPUT_STREAMS=8\n"

static int g_width, g_height;

static void InferenceCallback(OutputBlobArray *Blobs, UserDataBuffers *frames) {
    printf("callback!\n");

    printf("blob num: %d\n", Blobs->num_blobs);

    for (int i = 0; i < Blobs->num_blobs; i++) {
        OutputBlobContext *blob = Blobs->output_blobs[i];
        if (blob) {
            Dimensions *dim;
            int object_size, max_proposal_count;
            float *detections = (float *)blob->output_blob_method->GetData(blob);
            printf("name: %s dims: ", blob->output_blob_method->GetOutputLayerName(blob));
            dim = blob->output_blob_method->GetDims(blob);
            for (size_t n = 0; n < dim->num_dims; n++)
                printf("[%lu]%lu ", n, dim->dims[n]);
            printf("\n");
            object_size = dim->dims[3];
            max_proposal_count = dim->dims[2];

            for (int i = 0; i < max_proposal_count; i++) {
                int image_id = (int)detections[i * object_size + 0];
                int label_id = (int)detections[i * object_size + 1];
                float confidence = detections[i * object_size + 2];
                float x_min = detections[i * object_size + 3] * g_width;
                float y_min = detections[i * object_size + 4] * g_height;
                float x_max = detections[i * object_size + 5] * g_width;
                float y_max = detections[i * object_size + 6] * g_height;

                if (image_id < 0)
                    break;
                if (confidence < 0.5)
                    continue;
                printf("id: %d conf:%f rect: %d %d %d %d\n", image_id, confidence, (int)x_min, (int)y_min, (int)x_max,
                       (int)y_max);
            }
        }
    }

    printf("frame num: %d\n", frames->num_buffers);
}

#include <libavutil/time.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "%s <model_name> <raw_image> <width> <height> \n", argv[0]);
        exit(-1);
    }

    FILE *f_img = fopen(argv[2], "rb");
    if (f_img == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[2]);
        exit(-1);
    }

    int width = g_width = atoi(argv[3]);
    int height = g_height = atoi(argv[4]);
    int plane_size = width * height;

    uint8_t *data = (uint8_t *)av_malloc(width * height * 3);
    fread(data, width * height * 3, 1, f_img);
    fclose(f_img);

    Image image = {};
    image.type = MEM_TYPE_SYSTEM;
    image.width = width;
    image.height = height;
    image.format = FOURCC_BGR;
    image.planes[0] = data;
    image.stride[0] = width * 3;

    const ImageInference *inference = image_inference_get_by_name("openvino");
    const OutputBlobMethod *method = output_blob_method_get_by_name("openvino");

    ImageInferenceContext *context = image_inference_alloc(inference, method, "test-infer");

    IFramePtr frame = &image;

    if (!context) {
        fprintf(stderr, "Fail to allocate inference context!\n");
        exit(-1);
    }

    printf("Create Image Inference[%s]:%s\n", inference->name, context->name);

    // create image inference instance
    context->inference->Create(context, MEM_TYPE_ANY, "CPU", argv[1], 1, 1, CONFIGS, NULL, InferenceCallback);

    context->inference->SubmitImage(context, &image, frame, NULL);

    context->inference->Flush(context);

    context->inference->Close(context);

    image_inference_free(context);

    printf("Pass!\n");

    FFBaseInference *infer_base = av_base_inference_create("ff_base_test");

    FFInferenceParam infer_param = {};

    infer_param.batch_size = 1;
    infer_param.nireq = 2;
    infer_param.model = argv[1];
    infer_param.device = "CPU";
    infer_param.every_nth_frame = 1;
    infer_param.is_full_frame = TRUE;
    infer_param.threshold = 0.5;

    av_base_inference_set_params(infer_base, &infer_param);

    AVFrame *av_frame = av_frame_alloc();
    av_frame->width = width;
    av_frame->height = height;
    av_frame->format = AV_PIX_FMT_BGR0;
    av_frame->data[0] = data;
    av_frame->linesize[0] = width * 3;

    av_base_inference_send_frame(NULL, infer_base, av_frame);

    AVFrame *out_frame = NULL;
    int ret = 0;
again:
    ret = av_base_inference_get_frame(NULL, infer_base, &out_frame);
    if (ret != 0) {
        av_usleep(10000);
        printf("wait 10ms\n");
        goto again;
    }

    if (out_frame) {
        printf("in:%p out:%p\n", av_frame, out_frame);
    }

    av_base_inference_release(infer_base);

    av_frame_free(&av_frame);

    free(data);

    return 0;
}