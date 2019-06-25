// Copyright (C) 2018-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <assert.h>
#include "ie_c_api.h"
#include "ie_api_impl.hpp"

namespace IEPY = InferenceEnginePython;

#ifdef __cplusplus
extern "C" {
#endif

struct ie_network {
    const char *name;
    size_t batch_size;
    void *plugin;
    void *object;
    std::unique_ptr<InferenceEnginePython::IEExecNetwork> ie_exec_network;

    std::map<std::string, InferenceEnginePython::InputInfo> inputs;
    std::map<std::string, InferenceEnginePython::OutputInfo> outputs;
};

struct ie_plugin {
    void *object;
    const char *device_name;
    const char *version;
    std::map<std::string, std::string> config;
};

namespace {

inline std::string fileNameNoExt(const std::string &filepath) {
    auto pos = filepath.rfind('.');
    if (pos == std::string::npos)
        return filepath;
    return filepath.substr(0, pos);
}

/* config string format: "A=1\nB=2\nC=3\n" */
inline std::map<std::string, std::string> String2Map(std::string const &s) {
    std::string key, val;
    std::istringstream iss(s);
    std::map<std::string, std::string> m;

    while (std::getline(std::getline(iss, key, '=') >> std::ws, val)) {
        m[key] = val;
    }

    return m;
}

ie_network_t *ie_network_create(ie_plugin_t *plugin, const char *model, const char *weights) {
    ie_network_t *network = (decltype(network))malloc(sizeof(*network));

    assert(plugin && model && network);
    std::string weights_file(weights);
    if (weights == nullptr)
        weights_file = fileNameNoExt(model) + ".bin";

    IEPY::IENetwork *ie_network_ptr = nullptr;
    try {
        ie_network_ptr = new IEPY::IENetwork(model, weights_file);
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error("Can not create network for model: " + std::string(model)));
    }

    network->name = ie_network_ptr->name.c_str();
    network->plugin = plugin;
    network->object = ie_network_ptr;
    network->inputs = ie_network_ptr->getInputs();
    network->outputs = ie_network_ptr->getOutputs();

    return network;
}

void ie_network_destroy(ie_network_t *network) {
    if (network == nullptr)
        return;

    if (network->object)
        delete reinterpret_cast<IEPY::IENetwork *>(network->object);

    free(network);
}

void ie_network_set_batch(ie_network_t *network, const size_t size) {
    if (network == nullptr)
        return;

    IEPY::IENetwork *network_impl = reinterpret_cast<IEPY::IENetwork *>(network->object);
    network_impl->setBatch(size);
    network->batch_size = size;
}

void ie_network_add_output(ie_network_t *network, const char *out_layer, const char *precision) {
    //TODO
}

ie_input_info_t ie_network_get_input(ie_network_t *network, const char *input_layer_name) {
    ie_input_info_t info = {};
    if (network == nullptr)
        return info;

    std::string input_layer_str = input_layer_name;
    auto it = (input_layer_name == nullptr) ? network->inputs.begin() : network->inputs.find(input_layer_str);
    if (it != network->inputs.end()) {
        info.name = it->first.c_str();
        IEPY::InputInfo *input_info = &it->second;
        info.ranks = input_info->dims.size();
        for (size_t i = 0; i < info.ranks; i++)
            info.dims[i] = input_info->dims[i];
        info.object = input_info;
    }
    return info;
}

ie_output_info_t ie_network_get_output(ie_network_t *network, const char *output_layer_name) {
    ie_output_info_t info = {};
    if (network == nullptr)
        return info;

    std::string output_layer_str = output_layer_name;
    auto it = (output_layer_name == nullptr) ? network->outputs.begin() : network->outputs.find(output_layer_str);
    if (it != network->outputs.end()) {
        info.name = it->first.c_str();
        IEPY::OutputInfo *output_info = &it->second;
        info.ranks = output_info->dims.size();
        for (size_t i = 0; i < info.ranks; i++)
            info.dims[i] = output_info->dims[i];
        info.object = output_info;
    }
    return info;
}

ie_plugin_t *ie_plugin_create(const char *device) {
    ie_plugin_t *plugin = (decltype(plugin))malloc(sizeof(*plugin));

    assert(device && plugin);

    IEPY::IEPlugin *ie_plugin_ptr = nullptr;
    std::string plugin_dir("");
    try {
        ie_plugin_ptr = new IEPY::IEPlugin(device, {plugin_dir});
    } catch (const std::exception &e) {
        std::throw_with_nested(std::runtime_error("Can not create plugin for device: " + std::string(device)));
    }

    plugin->device_name = ie_plugin_ptr->device_name.c_str();
    plugin->version = ie_plugin_ptr->version.c_str();
    plugin->object = ie_plugin_ptr;

    return plugin;
}

void ie_plugin_destroy(ie_plugin_t *plugin) {
    if (plugin == nullptr)
        return;

    if (plugin->object)
        delete reinterpret_cast<IEPY::IEPlugin *>(plugin->object);

    free(plugin);
}

void ie_plugin_set_config(ie_plugin *plugin, const char *ie_configs) {
    if (!plugin || !ie_configs)
        return;

    IEPY::IEPlugin *plugin_impl = reinterpret_cast<IEPY::IEPlugin *>(plugin->object);
    plugin->config = String2Map(ie_configs);
    plugin_impl->setConfig(plugin->config);
};

void ie_plugin_add_cpu_extension(ie_plugin_t *plugin, const char *ext_path) {
    if (!plugin || !ext_path)
        return;

    IEPY::IEPlugin *plugin_impl = reinterpret_cast<IEPY::IEPlugin *>(plugin->object);
    plugin_impl->addCpuExtension(ext_path);
}

}

#ifdef __cplusplus
}
#endif