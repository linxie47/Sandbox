#pragma once

#include "ie_meta.h"
#include "ie_wrapper.h"
#include "inference_impl.h"
#include "ff_base_inference.h"

void ExtractBoundingBoxes(const std::map<std::string, InferenceBackend::OutputBlob::Ptr> &output_blobs,
                          std::vector<InferenceROI> frames, const std::map<std::string, void *> &model_proc,
                          const char *model_name, FFBaseInference *gva_base_inference);

PostProcFunction getPostProcFunctionByName(const char *name);
int findModelPostProcByName(ModelOutputPostproc *model_postproc, const char *layer_name);