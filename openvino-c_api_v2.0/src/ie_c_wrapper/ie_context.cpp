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
#include "ie_context.h"

using namespace std;

namespace InferenceEngine {

CIEContext::CIEContext()
{
    bModelLoaded = false;
    bModelCreated = false;
    targetDevice = InferenceEngine::TargetDevice::eCPU;
}

CIEContext::~CIEContext()
{

}

CIEContext::CIEContext(IEConfig * config)
{
    bModelLoaded = false;
    bModelCreated = false;
    targetDevice = InferenceEngine::TargetDevice::eCPU;
    Init(config);
    bModelLoaded = true;
    bModelCreated = true;
}

void CIEContext::loadModel(IEConfig * config)
{
    if (bModelLoaded) return;

    std::string path("");
    if (config->pluginPath)
        path.assign(config->pluginPath);

    InferenceEngine::PluginDispatcher dispatcher({ path, "../../../lib/intel64", "" });
    targetDevice = getDeviceFromId(config->targetId);
    /** Loading plugin for device **/
    plugin = dispatcher.getPluginByDevice(getDeviceName(targetDevice));
    enginePtr = plugin;
    if (nullptr == enginePtr) {
        std::cout << "Plugin path is not found!" << std::endl;
        std::cout << "Plugin Path =" << path << std::endl;
    }
    std::cout << "targetDevice:" << getDeviceName(targetDevice) << std::endl;

    /*If CPU device, load default library with extensions that comes with the product*/
    if (config->targetId == IE_CPU) {
        /**
        * cpu_extensions library is compiled from "extension" folder containing
        * custom MKLDNNPlugin layer implementations. These layers are not supported
        * by mkldnn, but they can be useful for inferring custom topologies.
        **/
        plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());
    }

    if (nullptr != config->cpuExtPath) {
        std::string cpuExtPath(config->cpuExtPath);
        // CPU(MKLDNN) extensions are loaded as a shared library and passed as a pointer to base extension
        auto extensionPtr = InferenceEngine::make_so_pointer<InferenceEngine::IExtension>(cpuExtPath);
        plugin.AddExtension(extensionPtr);
        std::cout << "CPU Extension loaded: " << cpuExtPath << endl;
    }
    if (nullptr != config->cldnnExtPath) {
        std::string cldnnExtPath(config->cldnnExtPath);
        // clDNN Extensions are loaded from an .xml description and OpenCL kernel files
        plugin.SetConfig({ { InferenceEngine::PluginConfigParams::KEY_CONFIG_FILE, cldnnExtPath } });
        std::cout << "GPU Extension loaded: " << cldnnExtPath << endl;
    }

    /** Setting plugin parameter for collecting per layer metrics **/
    if (config->perfCounter > 0) {
        plugin.SetConfig({ { InferenceEngine::PluginConfigParams::KEY_PERF_COUNT, InferenceEngine::PluginConfigParams::YES } });
    }

    std::string modelFileName(config->modelFileName);
    if (modelFileName.empty()) {
        std::cout << "Model file name is empty!" << endl;
        return;
    }

    xmlFile = GetFileNameNoExt(config->modelFileName) + ".xml";
    std::string networkFileName(xmlFile);
    networkReader.ReadNetwork(networkFileName);

    binFile = GetFileNameNoExt(config->modelFileName) + ".bin";
    std::string weightFileName(binFile);
    networkReader.ReadWeights(weightFileName);

    network = networkReader.getNetwork();
    inputsInfo = network.getInputsInfo();
    outputsInfo = network.getOutputsInfo();

    bModelLoaded = true;
}

void CIEContext::createModel(IEConfig * config)
{
    if (!bModelLoaded) {
        std::cout << "Please load the model firstly!" << endl;
        return;
    }

    if (bModelCreated) return;

    // prepare the input and output Blob
    // Set the precision of intput/output data provided by the user, should be called before load of the network to the plugin

    setModelInputInfo(&config->inputInfos);
    setModelOutputInfo(&config->outputInfos);

    executeNetwork = plugin.LoadNetwork(network, networkConfig);
    inferRequest = executeNetwork.CreateInferRequest();
    bModelCreated = true;
}

void CIEContext::Init(IEConfig * config)
{
    if (bModelLoaded && bModelCreated) return;

    if (!bModelLoaded) {
        loadModel(config);
    }

    if (!bModelCreated) {
        createModel(config);
    }
}

void CIEContext::setTargetDevice(InferenceEngine::TargetDevice device)
{
    targetDevice = device;
}

void CIEContext::setCpuThreadsNum(const size_t num)
{
    networkConfig[PluginConfigParams::KEY_CPU_THREADS_NUM] = std::to_string(num);
}

void CIEContext::setBatchSize(const size_t size)
{
    network.setBatchSize(size);
}

size_t CIEContext::getBatchSize()
{
    return network.getBatchSize();
}

void CIEContext::forwardSync()
{
    inferRequest.Infer();
}

void CIEContext::forwardAsync()
{
    inferRequest.StartAsync();
    inferRequest.Wait(IInferRequest::WaitMode::RESULT_READY);
}

size_t CIEContext::getInputSize()
{
    return inputsInfo.size();
}

#if 0
void CIEContext::setInputPresion(unsigned int idx, IEPrecisionType precision)
{
    unsigned int id = 0;

    if (idx > inputsInfo.size()) {
        std::cout << "Input is out of range" << std::endl;
        return;
    }
    /** Iterating over all input blobs **/
    for (auto & item : inputsInfo) {
        if (id == idx) {
            /** Creating first input blob **/
            Precision inputPrecision = getPrecisionByEnum(precision);
            item.second->setPrecision(inputPrecision);
            break;
        }
        id++;
    }
}

void CIEContext::setOutputPresion(unsigned int idx, IEPrecisionType precision)
{
    unsigned int id = 0;

    if (idx > outputsInfo.size()) {
        std::cout << "Output is out of range" << std::endl;
        return;
    }
    /** Iterating over all output blobs **/
    for (auto & item : outputsInfo) {
        if (id == idx) {
            /** Creating first input blob **/
            Precision outputPrecision = getPrecisionByEnum(precision);
            item.second->setPrecision(outputPrecision);
            break;
        }
        id++;
    }
}

void CIEContext::setInputLayout(unsigned int idx, IELayoutType layout)
{
    unsigned int id = 0;

    if (idx > inputsInfo.size()) {
        std::cout << "Input is out of range" << std::endl;
        return;
    }
    /** Iterating over all input blobs **/
    for (auto & item : inputsInfo) {
        if (id == idx) {
            /** Creating first input blob **/
            Layout inputLayout = getLayoutByEnum(layout);
            item.second->setLayout(inputLayout);
            break;
        }
        id++;
    }

}

void CIEContext::setOutputLayout(unsigned int idx, IELayoutType layout)
{
    unsigned int id = 0;

    if (idx > outputsInfo.size()) {
        std::cout << "Output is out of range" << std::endl;
        return;
    }
    /** Iterating over all output blobs **/
    for (auto & item : outputsInfo) {
        if (id == idx) {
            Layout outputLayout = getLayoutByEnum(layout);
            item.second->setLayout(outputLayout);
            break;
        }
        id++;
    }

}
#endif

void CIEContext::getModelInputInfo(IEInputOutputInfo * info)
{
    if (!bModelLoaded) {
        std::cout << "Please load the model firstly!" << endl;
        return;
    }

    size_t id = 0;
    for (auto & item : inputsInfo) {
        auto dims = item.second->getDims();
        assert(dims.size() <= IE_MAX_DIMENSIONS);
        for (int i = 0; i < dims.size(); i++)
            info->tensorMeta[id].dims[i] = dims[i];

        std::string itemName = item.first;
        assert(itemName.length() < IE_MAX_NAME_LEN);
        size_t length = itemName.copy(info->tensorMeta[id].layer_name, itemName.length());
        info->tensorMeta[id].layer_name[length] = '\0';
        info->tensorMeta[id].precision = getEnumByPrecision(item.second->getPrecision());
        info->tensorMeta[id].layout = getEnumByLayout(item.second->getLayout());
        id++;
    }
    info->batch_size = getBatchSize();
    info->number = inputsInfo.size();
}

void CIEContext::setModelInputInfo(IEInputOutputInfo * info)
{
    if (!bModelLoaded) {
        std::cout << "Please load the model firstly!" << endl;
        return;
    }

    if (info->number != inputsInfo.size()) {
        std::cout << "Input size is not matched with model!" << endl;
        return;
    }

    size_t id = 0;
    for (auto & item : inputsInfo) {
        Precision precision = getPrecisionByEnum(info->tensorMeta[id].precision);
        Layout layout = getLayoutByEnum(info->tensorMeta[id].layout);
        item.second->setPrecision(precision);
        item.second->setLayout(layout);
        id++;
    }
}

void CIEContext::getModelOutputInfo(IEInputOutputInfo * info)
{
    if (!bModelLoaded) {
        std::cout << "Please load the model firstly!" << endl;
        return;
    }

    size_t id = 0;
    for (auto & item : outputsInfo) {
        auto dims = item.second->getDims();
        assert(dims.size() <= IE_MAX_DIMENSIONS);
        for (int i = 0; i < dims.size(); i++)
            info->tensorMeta[id].dims[i] = dims[i];

        std::string itemName = item.first;
        assert(itemName.length() < IE_MAX_NAME_LEN);
        size_t length = itemName.copy(info->tensorMeta[id].layer_name, itemName.length());
        info->tensorMeta[id].layer_name[length] = '\0';
        info->tensorMeta[id].precision = getEnumByPrecision(item.second->getPrecision());
        info->tensorMeta[id].layout = getEnumByLayout(item.second->getLayout());
        id++;
    }
    info->batch_size = getBatchSize();
    info->number = outputsInfo.size();
}

void CIEContext::setModelOutputInfo(IEInputOutputInfo * info)
{
}

void CIEContext::addInput(unsigned int idx, IEData * data)
{
    unsigned int id = 0;
    std::string itemName;

    if (idx > inputsInfo.size()) {
        std::cout << "Input is out of range" << std::endl;
        return;
    }
    /** Iterating over all input blobs **/
    for (auto & item : inputsInfo) {
        if (id == idx) {
            itemName = item.first;
            break;
        }
        id++;
    }

    if (itemName.empty()) {
        std::cout << "item name is empty!" << std::endl;
        return;
    }

    if (data->batchIdx > getBatchSize()) {
        std::cout << "Too many input, it is bigger than batch size!" << std::endl;
        return;
    }

    Blob::Ptr blob = inferRequest.GetBlob(itemName);
    if (data->precision == IE_FP32) {
        if(data->dataType == IE_DATA_TYPE_IMG)
            imageU8ToBlob<PrecisionTrait<Precision::FP32>::value_type>(data, blob, data->batchIdx);
        else
            nonImageToBlob<PrecisionTrait<Precision::FP32>::value_type>(data, blob, data->batchIdx);
    } else {
        if(data->dataType == IE_DATA_TYPE_IMG)
            imageU8ToBlob<uint8_t>(data, blob, data->batchIdx);
        else
            nonImageToBlob<uint8_t>(data, blob, data->batchIdx);
    }

#if 1 // TODO: remove when license-plate-recognition-barrier model will take one input
    if (inputsInfo.count("seq_ind")) {
        // 'seq_ind' input layer is some relic from the training
        // it should have the leading 0.0f and rest 1.0f
        Blob::Ptr seqBlob = inferRequest.GetBlob("seq_ind");
        int maxSequenceSizePerPlate = seqBlob->getTensorDesc().getDims()[0];
        float *blob_data = seqBlob->buffer().as<float *>();
        blob_data[0] = 0.0f;
        std::fill(blob_data + 1, blob_data + maxSequenceSizePerPlate, 1.0f);
    }
#endif
}

void * CIEContext::getOutput(unsigned int idx, unsigned int * size)
{
    unsigned int id = 0;
    std::string itemName;

    if (idx > outputsInfo.size()) {
        std::cout << "Output is out of range" << std::endl;
        return nullptr;
    }
    /** Iterating over all input blobs **/
    for (auto & item : outputsInfo) {
        if (id == idx) {
            itemName = item.first;
            break;
        }
        id++;
    }

    if (itemName.empty()) {
        std::cout << "item name is empty!" << std::endl;
        return nullptr;
    }

    const Blob::Ptr blob = inferRequest.GetBlob(itemName);
    float* outputResult = static_cast<PrecisionTrait<Precision::FP32>::value_type*>(blob->buffer());

    *size = blob->byteSize();
    return (reinterpret_cast<void *>(outputResult));
}

InferenceEngine::TargetDevice CIEContext::getDeviceFromString(const std::string &deviceName) {
    return InferenceEngine::TargetDeviceInfo::fromStr(deviceName);
}

InferenceEngine::TargetDevice CIEContext::getDeviceFromId(IETargetDeviceType device) {
    switch (device) {
    case IE_Default:
        return InferenceEngine::TargetDevice::eDefault;
    case IE_Balanced:
        return InferenceEngine::TargetDevice::eBalanced;
    case IE_CPU:
        return InferenceEngine::TargetDevice::eCPU;
    case IE_GPU:
        return InferenceEngine::TargetDevice::eGPU;
    case IE_FPGA:
        return InferenceEngine::TargetDevice::eFPGA;
    case IE_MYRIAD:
        return InferenceEngine::TargetDevice::eMYRIAD;
    case IE_HDDL:
        return InferenceEngine::TargetDevice::eHDDL;
    case IE_GNA:
        return InferenceEngine::TargetDevice::eGNA;
    case IE_HETERO:
        return InferenceEngine::TargetDevice::eHETERO;
    default:
        return InferenceEngine::TargetDevice::eCPU;
    }
}

InferenceEngine::Layout CIEContext::estimateLayout(const int chNum)
{
    if (chNum == 4)
        return InferenceEngine::Layout::NCHW;
    else if (chNum == 2)
        return InferenceEngine::Layout::NC;
    else if (chNum == 3)
        return InferenceEngine::Layout::CHW;
    else
        return InferenceEngine::Layout::ANY;
}

InferenceEngine::Layout CIEContext::getLayoutByEnum(IELayoutType layout)
{
    switch (layout) {
    case IE_NCHW:
        return InferenceEngine::Layout::NCHW;
    case IE_NHWC:
        return InferenceEngine::Layout::NHWC;
    case IE_OIHW:
        return InferenceEngine::Layout::OIHW;
    case IE_C:
        return InferenceEngine::Layout::C;
    case IE_CHW:
        return InferenceEngine::Layout::CHW;
    case IE_HW:
        return InferenceEngine::Layout::HW;
    case IE_NC:
        return InferenceEngine::Layout::NC;
    case IE_CN:
        return InferenceEngine::Layout::CN;
    case IE_BLOCKED:
        return InferenceEngine::Layout::BLOCKED;
    case IE_ANY:
        return InferenceEngine::Layout::ANY;
    default:
        return InferenceEngine::Layout::ANY;
    }

}

IELayoutType CIEContext::getEnumByLayout(InferenceEngine::Layout layout)
{
    switch (layout) {
    case InferenceEngine::Layout::NCHW:
        return IE_NCHW;
    case InferenceEngine::Layout::NHWC:
        return IE_NHWC;
    case InferenceEngine::Layout::OIHW:
        return IE_OIHW;
    case InferenceEngine::Layout::C:
        return IE_C;
    case InferenceEngine::Layout::CHW:
        return IE_CHW;
    case InferenceEngine::Layout::HW:
        return IE_HW;
    case InferenceEngine::Layout::NC:
        return IE_NC;
    case InferenceEngine::Layout::CN:
        return IE_CN;
    case InferenceEngine::Layout::BLOCKED:
        return IE_BLOCKED;
    case InferenceEngine::Layout::ANY:
        return IE_ANY;
    default:
        return IE_ANY;
    }
}

InferenceEngine::Precision CIEContext::getPrecisionByEnum(IEPrecisionType precision)
{
    switch (precision) {
    case IE_MIXED:
        return InferenceEngine::Precision::MIXED;
    case IE_FP32:
        return InferenceEngine::Precision::FP32;
    case IE_FP16:
        return InferenceEngine::Precision::FP16;
    case IE_Q78:
        return InferenceEngine::Precision::Q78;
    case IE_I16:
        return InferenceEngine::Precision::I16;
    case IE_U8:
        return InferenceEngine::Precision::U8;
    case IE_I8:
        return InferenceEngine::Precision::I8;
    case IE_U16:
        return InferenceEngine::Precision::U16;
    case IE_I32:
        return InferenceEngine::Precision::I32;
    case IE_CUSTOM:
        return InferenceEngine::Precision::CUSTOM;
    case IE_UNSPECIFIED:
        return InferenceEngine::Precision::UNSPECIFIED;
    default:
        return InferenceEngine::Precision::UNSPECIFIED;
    }
}

IEPrecisionType CIEContext::getEnumByPrecision(InferenceEngine::Precision precision)
{
    switch (precision) {
    case InferenceEngine::Precision::MIXED:
        return IE_MIXED;
    case InferenceEngine::Precision::FP32:
        return IE_FP32;
    case InferenceEngine::Precision::FP16:
        return IE_FP16;
    case InferenceEngine::Precision::Q78:
        return IE_Q78;
    case InferenceEngine::Precision::I16:
        return IE_I16;
    case InferenceEngine::Precision::U8:
        return IE_U8;
    case InferenceEngine::Precision::I8:
        return IE_I8;
    case InferenceEngine::Precision::U16:
        return IE_U16;
    case InferenceEngine::Precision::I32:
        return IE_I32;
    case InferenceEngine::Precision::CUSTOM:
        return IE_CUSTOM;
    case InferenceEngine::Precision::UNSPECIFIED:
        return IE_UNSPECIFIED;
    default:
        return IE_UNSPECIFIED;
    }
}

std::string CIEContext::GetFileNameNoExt(const std::string &filePath) {
    auto pos = filePath.rfind('.');
    if (pos == std::string::npos) return filePath;
    return filePath.substr(0, pos);
}

void CIEContext::getTopNResultWithClassfication(unsigned int idx, int topN)
{

    size_t batchSize = getBatchSize();
    unsigned int id = 0;
    std::string itemName;

    if (idx > outputsInfo.size()) {
        std::cout << "Output is out of range" << std::endl;
        return;
    }
    /** Iterating over all input blobs **/
    for (auto & item : outputsInfo) {
        if (id == idx) {
            itemName = item.first;
            break;
        }
        id++;
    }

    if (itemName.empty()) {
        std::cout << "item name is empty!" << std::endl;
        return;
    }

    /** Validating -nt value **/
    const auto outputBlob = inferRequest.GetBlob(itemName);
    const auto outputData = outputBlob->buffer().as<PrecisionTrait<Precision::FP32>::value_type*>();
    const int resultsCnt = outputBlob->size() / batchSize;

    if (topN > resultsCnt || topN < 1) {
        std::cout << "TopN " << topN << " is not available for this network (TopN should be less than " \
            << resultsCnt + 1 << " and more than 0)\n            will be used maximal value : " << resultsCnt <<std::endl;
        topN = resultsCnt;
    }

    /** This vector stores id's of top N results **/
    std::vector<unsigned> results;
    InferenceEngine::TopResults(topN, *outputBlob, results);

    std::cout << std::endl << "Top " << topN << " results:" << std::endl << std::endl;

    /** Read labels from file (e.x. AlexNet.labels) **/
    bool labelsEnabled = false;
    std::string labelFileName = GetFileNameNoExt(xmlFile) + ".labels";
    std::vector<std::string> labels;

    std::ifstream inputFile;
    inputFile.open(labelFileName, std::ios::in);
    if (inputFile.is_open()) {
        std::string strLine;
        while (std::getline(inputFile, strLine)) {
            labels.push_back(strLine);
        }
        labelsEnabled = true;
    }

    /** Print the result iterating over each batch **/
    for (int image_id = 0; image_id < batchSize; ++image_id) {
        std::cout << "Image " << image_id << std::endl << std::endl;
        for (size_t id = image_id * topN, cnt = 0; cnt < topN; ++cnt, ++id) {
            std::cout.precision(7);
            /** Getting probability for resulting class **/
            const auto result = outputData[results[id] + image_id * (outputBlob->size() / batchSize)];
            std::cout << std::left << std::fixed << results[id] << " " << result;
            if (labelsEnabled) {
                std::cout << " label " << labels[results[id]] << std::endl;
            }
            else {
                std::cout << " label #" << results[id] << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

template <typename T> void CIEContext::imageU8ToBlob(const IEData * data, Blob::Ptr& blob, int batchIndex)
{
    SizeVector blobSize = blob.get()->dims();
    const size_t width = blobSize[0];
    const size_t height = blobSize[1];
    const size_t channels = blobSize[2];
    const size_t imageSize = width * height;
    unsigned char * buffer = (unsigned char *)(data->data[0]);
    T* blob_data = blob->buffer().as<T*>();
    const float mean_val = 127.5f;
    const float std_val = 0.0078125f;

    if (width != data->width || height!= data->height) {
        std::cout << "Input Image size is not matched with model!" << endl;
        return;
    }

    int batchOffset = batchIndex * height * width * channels;

    if (IE_IMAGE_BGR_PLANAR == data->imageFormat) {
        if (data->linesize[0] == width && data->linesize[1] == width && data->linesize[2] == width) {
            for (size_t ch = 0; ch < channels; ch++)
                std::memcpy(blob_data + batchOffset + ch * imageSize, data->data[ch], imageSize);
        } else {
            for (size_t ch = 0; ch < channels; ch++)
                for (size_t h = 0; h < height; h++)
                    std::memcpy(blob_data + batchOffset + ch * imageSize + h * width, data->data[ch] + h * data->linesize[ch], width);
        }
    }
    else if (IE_IMAGE_BGR_PACKED == data->imageFormat) {
        size_t imageStrideSize = data->linesize[0];
        if (data->precision == IE_FP32) {
            /** Iterate over all pixel in image (b,g,r) **/
            for (size_t h = 0; h < height; h++)
                for (size_t w = 0; w < width; w++)
                    /** Iterate over all channels **/
                    for (size_t ch = 0; ch < channels; ++ch)
                        blob_data[batchOffset + ch * imageSize + h * width + w] = float((buffer[h * imageStrideSize + w * data->channelNum + ch] - mean_val) * std_val);
        } else {
            /** Iterate over all pixel in image (b,g,r) **/
            for (size_t h = 0; h < height; h++)
                for (size_t w = 0; w < width; w++)
                    /** Iterate over all channels **/
                    for (size_t ch = 0; ch < channels; ++ch)
                        blob_data[batchOffset + ch * imageSize + h * width + w] = buffer[h * imageStrideSize + w * data->channelNum + ch];
        }
    }
    else if (IE_IMAGE_RGB_PLANAR == data->imageFormat) {
        /* B */
        if (data->linesize[2] == width) {
            std::memcpy(blob_data + batchOffset, data->data[2], imageSize);
        } else {
            for (size_t h = 0; h < height; h++)
                std::memcpy(blob_data + batchOffset + h * width, data->data[2] + h * data->linesize[2], width);
        }
        /* G */
        if (data->linesize[1] == width) {
            std::memcpy(blob_data + batchOffset + imageSize, data->data[1], imageSize);
        } else {
            for (size_t h = 0; h < height; h++)
                std::memcpy(blob_data + batchOffset + imageSize + h * width, data->data[1] + h * data->linesize[1], width);
        }
        /* R */
        if (data->linesize[0] == width) {
            std::memcpy(blob_data + batchOffset + 2 * imageSize, data->data[0], imageSize);
        } else {
            for (size_t h = 0; h < height; h++)
                std::memcpy(blob_data + batchOffset + 2 * imageSize + h * width, data->data[0] + h * data->linesize[0], width);
        }
    }
    else if (IE_IMAGE_RGB_PACKED == data->imageFormat) {
        // R G B packed input image, wwitch the R and B packed value. TBD

    }
    else if (IE_IMAGE_GRAY_PLANAR == data->imageFormat) {

    }
}

template <typename T> void CIEContext::nonImageToBlob(const IEData * data, Blob::Ptr& blob, int batchIndex)
{
    SizeVector blobSize = blob.get()->dims();
    T * buffer = (T *)data->data[0];
    T* blob_data = blob->buffer().as<T*>();

    int batchOffset = batchIndex * data->size;

    memcpy(blob_data+batchOffset, buffer, data->size);
}

void CIEContext::printPerformanceCounts(const std::map<std::string, InferenceEngine::InferenceEngineProfileInfo>& performanceMap, std::ostream &stream, bool bshowHeader)
{
    long long totalTime = 0;
    // Print performance counts
    if (bshowHeader) {
        stream << std::endl << "performance counts:" << std::endl << std::endl;
    }
    for (const auto & it : performanceMap) {
        std::string toPrint(it.first);
        const int maxLayerName = 30;

        if (it.first.length() >= maxLayerName) {
            toPrint = it.first.substr(0, maxLayerName - 4);
            toPrint += "...";
        }


        stream << std::setw(maxLayerName) << std::left << toPrint;
        switch (it.second.status) {
        case InferenceEngine::InferenceEngineProfileInfo::EXECUTED:
            stream << std::setw(15) << std::left << "EXECUTED";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::NOT_RUN:
            stream << std::setw(15) << std::left << "NOT_RUN";
            break;
        case InferenceEngine::InferenceEngineProfileInfo::OPTIMIZED_OUT:
            stream << std::setw(15) << std::left << "OPTIMIZED_OUT";
            break;
        }
        stream << std::setw(30) << std::left << "layerType: " + std::string(it.second.layer_type) + " ";
        stream << std::setw(20) << std::left << "realTime: " + std::to_string(it.second.realTime_uSec);
        stream << std::setw(20) << std::left << " cpu: " + std::to_string(it.second.cpu_uSec);
        stream << " execType: " << it.second.exec_type << std::endl;
        if (it.second.realTime_uSec > 0) {
            totalTime += it.second.realTime_uSec;
        }
    }
    stream << std::setw(20) << std::left << "Total time: " + std::to_string(totalTime) << " microseconds" << std::endl;
}

void CIEContext::printLog(unsigned int flag)
{
    std::map<std::string, InferenceEngine::InferenceEngineProfileInfo> perfomanceMap;

    if (flag == IE_LOG_LEVEL_NONE)
        return;

    if (flag&IE_LOG_LEVEL_ENGINE) {
        enginePtr->GetPerformanceCounts(perfomanceMap, nullptr);
        printPerformanceCounts(perfomanceMap, std::cout, true);
    }

    if (flag&IE_LOG_LEVEL_LAYER) {
        perfomanceMap = inferRequest.GetPerformanceCounts();
        printPerformanceCounts(perfomanceMap, std::cout, true);
    }
}

}  // namespace InferenceEngine

