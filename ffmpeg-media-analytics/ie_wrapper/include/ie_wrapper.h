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
typedef struct _FFInferenceParam FFInferenceParam;
typedef struct _ProcessedFrame ProcessedFrame;
typedef struct _InputPreproc   InputPreproc;
typedef struct _OutputPostproc OutputPostproc;
typedef struct _InputPreproc         ModelInputPreproc;
typedef struct _ModelOutputPostproc  ModelOutputPostproc;

struct _FFInferenceParam {
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
    float threshold;
    ModelInputPreproc   *model_preproc;
    ModelOutputPostproc *model_postproc;
};

struct _ProcessedFrame {
    AVFrame *frame;
    void   **ie_output_blobs;
    int      blob_num;
};

/* model preproc */
struct _InputPreproc {
    int   color_format;     ///<! input data format
    char *layer_name;       ///<! layer name of input
    char *object_class;     ///<! interested object class
};

/* model postproc */
struct _OutputPostproc {
    char  *layer_name;
    char  *converter;
    char  *attribute_name;
    char  *method;
    double threshold;
    double tensor2text_scale;
    int    tensor2text_precision;
    AVBufferRef *labels;
};

#define MAX_MODEL_OUTPUT 4
struct _ModelOutputPostproc {
    OutputPostproc procs[MAX_MODEL_OUTPUT];
};

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