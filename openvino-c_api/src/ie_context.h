// Copyright (c) 2018 Intel Corporation
//
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

/**
 * @brief A header file that provides wrapper for IE context object
 * @file ie_context.h
 */

#ifndef _IE_CONTEXT_H_
#define _IE_CONTEXT_H_

#pragma once


#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <utility>
#include <memory>
#include <iomanip>
#include <inference_engine.hpp>
#include <ext_list.hpp>
#include "ie_common_wrapper.h"

namespace InferenceEngine {

/**
 * @brief This class contains all the information about the Inference Engine Context
 */
class CIEContext {
public:
    /**
    * @brief A default constructor
    */
    CIEContext();

    /**
    * @brief A default constructor
    */
    ~CIEContext();

    /**
    * @brief A constructor with IE configuration
    */
    CIEContext(IEConfig * config);

    /**
    * @brief load the model with graph and weight Model IR file
    */
    void loadModel(IEConfig * config);

    /**
    * @brief create the model/network with Model IR
    */
    void createModel(IEConfig * config);

    /**
    * @brief A init function withou model load
    */
    void Init(IEConfig * config);

    /**
    * @brief set target device
    */
    void setTargetDevice(InferenceEngine::TargetDevice device);

    void setCpuThreadsNum(const size_t num);

    /**
    * @brief set batch size device
    */
    void setBatchSize(const size_t size);

    /**
    * @brief set batch size device
    */
    size_t getBatchSize();

    /**
    * @brief check initialized
    */
    bool isContextInitailized() {
        return (bModelLoaded && bModelCreated);
    };

    /**
    * @brief inference on the input data
    */
    void forwardSync();

    /**
    * @brief async inference on the input data
    */
    void forwardAsync();

    /**
    * @brief get input size
    */
    size_t getInputSize();

    /**
    * @brief set input precision
    */
    void setInputPresion(unsigned int idx, IEPrecisionType precision);

    /**
    * @brief set output precision
    */
    void setOutputPresion(unsigned int idx, IEPrecisionType precision);

    /**
    * @brief set input precision
    */
    void setInputLayout(unsigned int idx, IELayoutType layout);

    /**
    * @brief set output precision
    */
    void setOutputLayout(unsigned int idx, IELayoutType layout);

    /**
    * @brief get model input image size
    */
    void getModelInputSize(IEImageSize * imageSize);

    /**
    * @brief get model input info
    */
    void getModelInputInfo(IEInputOutputInfo * info);

    /**
    * @brief set model input info
    */
    void setModelInputInfo(IEInputOutputInfo * info);

    /**
    * @brief get model output info
    */
    void getModelOutputInfo(IEInputOutputInfo * info);

    /**
    * @brief set model output info
    */
    void setModelOutputInfo(IEInputOutputInfo * info);

    /**
    * @brief add video input
    */
    void addInput(unsigned int idx, IEData * data);

    /**
    * @brief get output result
    */
    void * getOutput(unsigned int idx, unsigned int * size);

    /**
    * @brief Print the performance counter info
    */
    void printLog(unsigned int flag);

protected:

    /*
    * @brief Converts string to TargetDevice
    */
    InferenceEngine::TargetDevice getDeviceFromString(const std::string &deviceName);

    /*
    * @brief Converts enum value to TargetDevice
    */
    InferenceEngine::TargetDevice getDeviceFromId(IETargetDeviceType device);

    /*
    * @brief estimate the data layout according to the channel number
    */
    InferenceEngine::Layout estimateLayout(const int chNum);

    /*
    * @brief Converts enum value to Layout
    */
    InferenceEngine::Layout getLayoutByEnum(IELayoutType layout);

    /*
    * @brief Converts lauout to enum value
    */
    IELayoutType getEnumByLayout(InferenceEngine::Layout layout);

    /*
    * @brief Converts enum value to Precision
    */
    InferenceEngine::Precision getPrecisionByEnum(IEPrecisionType precision);

    /*
    * @brief Converts precision to enum value
    */
    IEPrecisionType getEnumByPrecision(InferenceEngine::Precision precision);

    /**
    * @brief remove the file extension and keep the whole path
    */
    std::string GetFileNameNoExt(const std::string &filePath);

    /**
    * @brief get top N result
    */
    void getTopNResultWithClassfication(unsigned int idx, int topN);

    /**
    * @brief Convert image U8 to blob
    */
    template <typename T> void imageU8ToBlob(const IEData * data, Blob::Ptr& blob, int batchIndex);

    /**
    * @brief Convert Non image to blob
    */
    template <typename T> void nonImageToBlob(const IEData * data, Blob::Ptr& blob, int batchIndex);

    /**
    * @brief Print the performance counter info
    */
    void printPerformanceCounts(const std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>& performanceMap, std::ostream &stream, bool bshowHeader);

protected:

    InferenceEngine::InferenceEnginePluginPtr enginePtr;
    InferenceEngine::InferencePlugin plugin;
    InferenceEngine::CNNNetReader networkReader;
    InferenceEngine::CNNNetwork network;
    InferenceEngine::ExecutableNetwork executeNetwork;
    InferenceEngine::InferRequest inferRequest;

    InferenceEngine::TargetDevice targetDevice;

    InferenceEngine::InputsDataMap inputsInfo;
    InferenceEngine::OutputsDataMap outputsInfo;

    std::string xmlFile;
    std::string binFile;

    IEImageSize modelInputImageSize;

    std::vector<InferenceEngine::CNNLayerPtr> layers;

    std::map<std::string, std::string> networkConfig;

    bool bModelLoaded;
    bool bModelCreated;

};

}  // namespace InferenceEngine

#endif
