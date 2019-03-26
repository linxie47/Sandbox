 // Licensed under the Apache License, Version 2.0 (the "License");
 // you may not use this file except in compliance with the License.
 // You may obtain a copy of the License at
 //
 //      http://www.apache.org/licenses/LICENSE-2.0
 //
 // Unless required by applicable law or agreed to in writing, software
 // distributed under the License is distributed on an "AS IS" BASIS,
 // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 // See the License for the specific language governing permissions and
 // limitations under the License.
#ifndef _IE_COMMON_WRAPPER_H_
#define _IE_COMMON_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

 /**
 * @enum TargetDevice
 * @brief Describes known device types
 */
typedef enum tagIETargetDeviceType {
    IE_Default = 0,
    IE_Balanced = 1,
    IE_CPU = 2,
    IE_GPU = 3,
    IE_FPGA = 4,
    IE_MYRIAD = 5,
    IE_HDDL = 6,
    IE_GNA = 7,
    IE_HETERO = 8
}IETargetDeviceType;

/**
* @enum Precision
* @brief Describes Precision types
*/
typedef enum tagIEPrecisinType {
    IE_UNSPECIFIED = 255, /**< Unspecified value. Used by default */
    IE_MIXED = 0,  /**< Mixed value. Can be received from network. No applicable for tensors */
    IE_FP32 = 10,  /**< 32bit floating point value */
    IE_FP16 = 11,  /**< 16bit floating point value */
    IE_Q78 = 20,   /**< 16bit specific signed fixed point precision */
    IE_I16 = 30,   /**< 16bit signed integer value */
    IE_U8 = 40,    /**< 8bit unsigned integer value */
    IE_I8 = 50,    /**< 8bit signed integer value */
    IE_U16 = 60,   /**< 16bit unsigned integer value */
    IE_I32 = 70,   /**< 32bit signed integer value */
    IE_CUSTOM = 80 /**< custom precision has it's own name and size of elements */
}IEPrecisionType;

/**
* @enum Layout
* @brief Layouts that the inference engine supports
*/
typedef enum tagIELayoutType {
    IE_ANY = 0,// "any" layout
    // I/O data layouts
    IE_NCHW = 1,
    IE_NHWC = 2,
    // weight layouts
    IE_OIHW = 64,
    // bias layouts
    IE_C = 96,
    // Single image layout (for mean image)
    IE_CHW = 128,
    // 2D
    IE_HW = 192,
    IE_NC = 193,
    IE_CN = 194,
    IE_BLOCKED = 200,
}IELayoutType;

/**
* @enum Memory Type
* @brief memory type that the inference engine supports?
*/
typedef enum tagIEMemoryType {
    IE_DEVICE_DEFAULT = 0,
    IE_DEVICE_HOST = 1,
    IE_DEVICE_GPU = 2,
    IE_DEVICE_MYRIAD = 3,
    IE_DEVICE_SHARED = 4,
}IEMemoryType;

/**
* @enum Image Channel order Type
* @brief Image Channel order now the BGR is used in the most model.
*/
typedef enum tagIEImageFormatType {
    IE_IMAGE_FORMAT_UNKNOWN = -1,
    IE_IMAGE_BGR_PACKED,
    IE_IMAGE_BGR_PLANAR,
    IE_IMAGE_RGB_PACKED,
    IE_IMAGE_RGB_PLANAR,
    IE_IMAGE_GRAY_PLANAR,
    IE_IMAGE_GENERIC_1D,
    IE_IMAGE_GENERIC_2D,
}IEImageFormatType;

/**
* @enum IE forward mode: sync/async
* @brief IE forward mode: sync/async
*/
typedef enum tagIEInferMode {
    IE_INFER_MODE_SYNC = 0,
    IE_INFER_MODE_ASYNC = 1,
}IEInferMode;

/**
* @enum IE data mode: image/non-image
* @brief IE data mode: image/non-image
*/
typedef enum tagIEDataType {
    IE_DATA_TYPE_NON_IMG = 0,
    IE_DATA_TYPE_IMG = 1,
}IEDataType;

/**
* @enum IE log level
* @brief IE log level
*/
typedef enum tagIELogLevel {
    IE_LOG_LEVEL_NONE = 0x0,
    IE_LOG_LEVEL_ENGINE = 0x1,
    IE_LOG_LEVEL_LAYER = 0x2,
}IELogLevel;

/**
* @enum Image Size Type
* @brief Image Size order now the BGR is used in the most model.
*/
typedef struct tagIEImageSize {
    unsigned int imageWidth;
    unsigned int imageHeight;
}IEImageSize;

/**
* @struct model input info
* @brief model input info
*/
#define IE_MAX_INPUT_OUTPUT 10
typedef struct tagIEInputOutputInfo {
    unsigned int width[IE_MAX_INPUT_OUTPUT];
    unsigned int height[IE_MAX_INPUT_OUTPUT];
    unsigned int channels[IE_MAX_INPUT_OUTPUT];
    IEPrecisionType precision[IE_MAX_INPUT_OUTPUT];
    IELayoutType layout[IE_MAX_INPUT_OUTPUT];
    IEDataType dataType[IE_MAX_INPUT_OUTPUT];
    unsigned int batch_size;
    unsigned int numbers;
}IEInputOutputInfo;

/**
* @struct inference engine image Input Data(BGR)
* @brief image input data for the inference engine supports
*/
typedef struct tagIEData {
    void * buffer;
    unsigned int size;  //byte size
    unsigned int width;
    unsigned int height;
    unsigned int widthStride;
    unsigned int heightStride;
    unsigned int channelNum;
    unsigned int batchIdx;
    IEPrecisionType precision; //IE_FP32:IE_FP16:IE_U8
    IEMemoryType memType;
    IEImageFormatType imageFormat;
    IEDataType dataType;
}IEData;

/**
* @struct inference engine Context Configuration
* @brief Configuration for the inference engine supports
*/
typedef struct tagIEConfig {
    IETargetDeviceType targetId;
    IEInputOutputInfo inputInfos;
    IEInputOutputInfo outputInfos;

    char * pluginPath;
    char * cpuExtPath;
    char * cldnnExtPath;
    char * modelFileName; // Bin file name
    char * outputLayerName;
    unsigned int  perfCounter;
    unsigned int inferReqNum;   // it work with async mode and value is 1 in default.
}IEConfig;

#ifdef __cplusplus
}
#endif
#endif
