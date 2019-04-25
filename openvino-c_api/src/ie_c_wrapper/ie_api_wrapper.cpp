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

#include "ie_api_wrapper.h"
#include "ie_context.h"


#ifdef __cplusplus
extern "C" {
#endif

using namespace std;
using namespace InferenceEngine;

int IESizeOfContext()
{
    return sizeof(InferenceEngine::CIEContext);
}

void * IEAllocateContext()
{
    InferenceEngine::CIEContext * context = new InferenceEngine::CIEContext();
    return (reinterpret_cast<void *>(context));
}

void * IEAllocateContextWithConfig(IEConfig * config)
{
    InferenceEngine::CIEContext * context = new InferenceEngine::CIEContext(config);
    return (reinterpret_cast<void *>(context));
}

void IEFreeContext(void * contextPtr)
{
    delete(reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr));
}

void IELoadModel(void * contextPtr, IEConfig * config)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->loadModel(config);
}

void IECreateModel(void * contextPtr, IEConfig * config)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->createModel(config);
}

void IEGetModelInputInfo(void * contextPtr, IEInputOutputInfo * info)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->getModelInputInfo(info);
}

void IESetModelInputInfo(void * contextPtr, IEInputOutputInfo * info)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->setModelInputInfo(info);
}

void IEGetModelOutputInfo(void * contextPtr, IEInputOutputInfo * info)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->getModelOutputInfo(info);
}

void IESetModelOutputInfo(void * contextPtr, IEInputOutputInfo * info)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->setModelOutputInfo(info);
}

void IEForward(void * contextPtr, IEInferMode aSyncMode)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);

    if (IE_INFER_MODE_SYNC == aSyncMode) {
        context->forwardSync();
    }
    else {
        context->forwardAsync();
    }
}

void IESetInput(void * contextPtr, unsigned int idx, IEData * data)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    context->addInput(idx, data);
}

void * IEGetResultSpace(void * contextPtr, unsigned int idx, unsigned int * size)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    return context->getOutput(idx, size);
}

void IEPrintLog(void * contextPtr, unsigned int flag)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    return context->printLog(flag);
}

void IESetBatchSize(void *contextPtr, int size)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    return context->setBatchSize(size);
}

void IESetCpuThreadsNum(void *contextPtr, unsigned int num)
{
    InferenceEngine::CIEContext * context = reinterpret_cast<InferenceEngine::CIEContext *>(contextPtr);
    return context->setCpuThreadsNum(num);
}

#ifdef __cplusplus
}
#endif
