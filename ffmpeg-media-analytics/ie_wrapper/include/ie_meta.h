#ifndef __IE_META_H__
#define __IE_META_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/frame.h>

#define FF_TENSOR_MAX_RANK 8

typedef enum {
    ANY  = 0,
    NCHW = 1,
    NHWC = 2,
} IELayout;

typedef enum {
    FP32 = 10,
    U8 = 40,
} IEPrecision;

typedef struct _IETensorMeta {
    IEPrecision precision;            /**< tensor precision (see FFPrecision) */
    int rank;                         /**< tensor rank */
    size_t dims[FF_TENSOR_MAX_RANK];  /**< array describing tensor's dimensions */
    IELayout layout;                  /**< tensor layout (see FFLayout) */
    char *layer_name;                 /**< tensor output layer name */
    char *model_name;                 /**< model name */
    void *data;                       /**< tensor data */
    size_t total_bytes;               /**< tensor size in bytes */
    const char *element_id;           /**< id of pipeline element that produced current tensor */
} IETensorMeta;

/* dynamic labels array */
typedef struct _LabelsArray {
    char **label;
    int    num;
} LabelsArray;

typedef struct _InferDetection {
    float   x_min;
    float   y_min;
    float   x_max;
    float   y_max;
    float   confidence;
    int     label_id;
    int     object_id;
    AVBufferRef *label_buf;
} InferDetection;

/* dynamic bounding boxes array */
typedef struct _BBoxesArray {
    InferDetection **bbox;
    int              num;
} BBoxesArray;

typedef struct _InferDetectionMeta {
    BBoxesArray *bboxes;
} InferDetectionMeta;


#ifdef __cplusplus
}
#endif

#endif