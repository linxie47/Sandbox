#ifndef __FF_BASE_INFERENCE_H__
#define __FF_BASE_INFERENCE_H__

#include <stddef.h>

extern "C" {

#include <libavutil/frame.h>

#ifndef TRUE
/** The TRUE value of a UBool @stable ICU 2.0 */
#   define TRUE  1
#endif
#ifndef FALSE
/** The FALSE value of a UBool @stable ICU 2.0 */
#   define FALSE 0
#endif

typedef struct _FFBaseInference {
    // properties
    char  *model;
    char  *object_class;
    char  *model_proc;
    char  *device;
    size_t batch_size;
    size_t every_nth_frame;
    size_t nireq;
    char  *inference_id;
    char  *cpu_streams;
    char  *infer_config;
    char  *allocator_name;
    
    // other fields
    //GstVideoInfo *info;
    int   is_full_frame;
    void *inference;        // C++ type: InferenceImpl*
    void *pre_proc;         // C++ type: PostProcFunction
    void *post_proc;        // C++ type: PostProcFunction
    void *get_roi_pre_proc; // C++ type: GetROIPreProcFunction
    int   initialized;
} FFBaseInference;

typedef struct _FFVideoRegionOfInterestMeta {
    int roi_type;
    
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} FFVideoRegionOfInterestMeta;

typedef struct {
    AVFrame *frame;
    void    *out_blobs;
} ProcessedFrame;

const char* ff_base_inference_version(void);

int ff_base_inference_init(FFBaseInference *base_inference);

int ff_base_inference_set_model(FFBaseInference *base_inference, const char *model_path);

int ff_base_inference_set_model_proc(FFBaseInference *base_inference, const char *model_proc_path);

int ff_base_inference_release(FFBaseInference *base_inference);

int ff_base_inference_send(FFBaseInference *base_inference, AVFrame *frame_in);

int ff_base_inference_fetch(FFBaseInference *base_inference, ProcessedFrame *out_put);

}

#endif /* __FF_BASE_INFERENCE_H__ */