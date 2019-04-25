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

#ifndef _IE_PLUGIN_WRAPPER_H_
#define _IE_PLUGIN_WRAPPER_H_

#include "ie_common_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * the API to allocate the Inference engine context
 * Input : the IEConfig as input
 * return: the void pointer to the IE context
 */
void * IEAllocateContextWithConfig(IEConfig * config);

/*
* the API to allocate the Inference engine context defaultly
* return : the void pointer to the IE context
*/
void * IEAllocateContext(void);

/*
 * the API to release the inferencce engine context
 * input: the pointer o inference engine context
 */
void IEFreeContext(void * contextPtr);

/*
* the API to load the model from IR format
* input: the pointer o inference engine context
* Input : the IEConfig as input with IR file name(absolute path)
* return: the void pointer to the IE context
*/
void IELoadModel(void * contextPtr, IEConfig * config);

/*
* the API to create the model/network and deploy it on the target device.
* this need to fill in the input info and output info before this API.
* input: the pointer o inference engine context
* Input : the IEConfig as input with IR file name(absolute path)
* return: the void pointer to the IE context
*/
void IECreateModel(void * contextPtr, IEConfig * config);

/*
 * the API to get the size of inference engine context
 * return the size of context
 */
int IESizeOfContext(void);

/*
* the API to get the info of model Input
* input: the pointer o inference engine context
* return:the input info of the model
*/
void IEGetModelInputInfo(void * contextPtr, IEInputOutputInfo * info);

/*
* the API to set the info of model Input
* input: the pointer o inference engine context
* return:the input info of the model
*/
void IESetModelInputInfo(void * contextPtr, IEInputOutputInfo * info);

/*
* the API to get the info of model output
* input: the pointer o inference engine context
* return:the output info of the model
*/
void IEGetModelOutputInfo(void * contextPtr, IEInputOutputInfo * info);

/*
* the API to set the info of model output
* input: the pointer o inference engine context
* return:the output info of the model
*/
void IESetModelOutputInfo(void * contextPtr, IEInputOutputInfo * info);

/*
* the API to execute the model in the sync/async mode. Call the IESetInput firstly.
* input: the pointer o inference engine context
* input: the sync or async mode. 1: async mode; 0: sync mode. default is 0
*/
void IEForward(void * contextPtr, IEInferMode mode);

/*
* the API to feed the input image to the model
* input: the pointer o inference engine context
* input: the index of input, idx can be get from the IEGetModelInputInfo()
* input: the image to be processed
*/
void IESetInput(void * contextPtr, unsigned int idx, IEData * data);

/*
* the API to set the output buffer of the model
* input: the pointer o inference engine context
* input: the output buffer
*/
//void IESetOutput(void * contextPtr, IEData * data);

/*
* the API to get the result pointer after the execution
* input: the pointer o inference engine context
* input: the index of output, idx can be get from the IEGetModelOutputInfo()
* output: the size of result
*/
void * IEGetResultSpace(void * contextPtr, unsigned int idx, unsigned int * size);

/*
* the API to print the log info after the execution
* input: the pointer o inference engine context
*/
void IEPrintLog(void * contextPtr, unsigned int flag);

/*
* the API to set batch size
* input: the batch size value to be set
*/
void IESetBatchSize(void *contextPtr, int size);

void IESetCpuThreadsNum(void *contextPtr, unsigned int num);

#ifdef __cplusplus
}
#endif
#endif
