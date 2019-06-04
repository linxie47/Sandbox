#ifndef __IE_WRAPPER_H__
#define __IE_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/frame.h>

typedef enum
{
    IE_EVENT_NONE,
    IE_EVENT_EOS,
} IE_EVENT;

typedef struct _FFBaseInference FFBaseInference;

typedef struct _FFInferenceParam {
    char *model;
    char *object_class;
    char *model_proc;
    char *device;
    int   batch_size;
    int   every_nth_frame;
    int   nireq;
    char *inference_id;
    char *cpu_streams;
    char *infer_config;
    int   is_full_frame;
} FFInferenceParam;

typedef struct _ProcessedFrame {
    AVFrame *frame;
    void   **ie_output_blobs;
    int      blob_num;
} ProcessedFrame;

const char* IEGetVersion(void);

FFBaseInference *IECreateInference(FFInferenceParam *param);

void IEReleaseInference(FFBaseInference *base);

int IESendFrame(FFBaseInference *base, AVFrame *in);

int IEGetProcesedFrame(FFBaseInference *base, ProcessedFrame *out);

int IEOutputFrameQueueEmpty(FFBaseInference *base);

void IESendEvent(FFBaseInference *base, int event);

#ifdef __cplusplus
}
#endif

#endif /* __IE_WRAPPER_H__ */