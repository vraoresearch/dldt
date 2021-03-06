// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "Python.h"

#include <iterator>
#include <string>
#include <utility>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <chrono>

#include <ie_extension.h>
#include "inference_engine.hpp"
#include "../../../../../src/inference_engine/ie_ir_reader.hpp"


typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::nanoseconds ns;

namespace InferenceEnginePython {
struct IENetLayer {
    InferenceEngine::CNNLayerPtr layer_ptr;
    InferenceEngine::CNNNetwork network_ptr;
    std::string name;
    std::string type;
    std::string precision;
    std::string shape;
    std::string layout;
    std::vector<std::string> children;
    std::vector<std::string> parents;
    std::string affinity;
    std::map<std::string, std::string> params;

    void setAffinity(const std::string &target_affinity);

    void setParams(const std::map<std::string, std::string> &params_map);

    std::map<std::string, InferenceEngine::Blob::Ptr> getWeights();

    void setPrecision(std::string precision);
};

struct InputInfo {
    InferenceEngine::InputInfo::Ptr actual;
    std::vector<size_t> dims;
    std::string precision;
    std::string layout;

    void setPrecision(std::string precision);

    void setLayout(std::string layout);
};

struct OutputInfo {
    InferenceEngine::DataPtr actual;
    std::vector<size_t> dims;
    std::string precision;
    std::string layout;

    void setPrecision(std::string precision);
};

struct ProfileInfo {
    std::string status;
    std::string exec_type;
    std::string layer_type;
    int64_t real_time;
    int64_t cpu_time;
    unsigned execution_index;
};

struct IENetwork {
    InferenceEngine::CNNNetwork actual;
    std::string name;
    std::size_t batch_size;
    std::string precision;

    void setBatch(const size_t size);

    void addOutput(const std::string &out_layer, size_t port_id);

    const std::vector<std::pair<std::string, InferenceEnginePython::IENetLayer>> getLayers();

    const std::map<std::string, InferenceEnginePython::InputInfo> getInputs();

    const std::map<std::string, InferenceEnginePython::OutputInfo> getOutputs();

    void reshape(const std::map<std::string, std::vector<size_t>> &input_shapes);

    void serialize(const std::string &path_to_xml, const std::string &path_to_bin);

    void setStats(const std::map<std::string, std::map<std::string, std::vector<float>>> &stats);

    const std::map<std::string, std::map<std::string, std::vector<float>>> getStats();

    void load_from_buffer(const char* xml, size_t xml_size, uint8_t* bin, size_t bin_size);

    IENetwork(const std::string &model, const std::string &weights, bool ngraph_compatibility);

    IENetwork(const InferenceEngine::CNNNetwork& cnn_network);

    IENetwork() = default;
};

struct InferRequestWrap {
    using cy_callback = void (*)(void*, int);

    InferenceEngine::IInferRequest::Ptr request_ptr;
    Time::time_point start_time;
    double exec_time;
    cy_callback user_callback;
    void *user_data;
    int status;

    void infer();

    void infer_async();

    int  wait(int64_t timeout);

    void setCyCallback(cy_callback callback, void *data);

    void getBlobPtr(const std::string &blob_name, InferenceEngine::Blob::Ptr &blob_ptr);

    void setBatch(int size);

    std::map<std::string, InferenceEnginePython::ProfileInfo> getPerformanceCounts();
};


struct IEExecNetwork {
    InferenceEngine::IExecutableNetwork::Ptr actual;
    std::vector<InferRequestWrap> infer_requests;
    std::string name;

    IEExecNetwork(const std::string &name, size_t num_requests);

    IENetwork GetExecGraphInfo();

    void infer();

    PyObject* getMetric(const std::string & metric_name);
    PyObject* getConfig(const std::string & metric_name);
};


struct IEPlugin {
    std::unique_ptr<InferenceEnginePython::IEExecNetwork> load(const InferenceEnginePython::IENetwork &net,
                                                               int num_requests,
                                                               const std::map<std::string, std::string> &config);

    std::string device_name;
    std::string version;

    void setConfig(const std::map<std::string, std::string> &);

    void addCpuExtension(const std::string &extension_path);

    void setInitialAffinity(const InferenceEnginePython::IENetwork &net);

    IEPlugin(const std::string &device, const std::vector<std::string> &plugin_dirs);

    IEPlugin() = default;

    std::set<std::string> queryNetwork(const InferenceEnginePython::IENetwork &net);

    InferenceEngine::InferencePlugin actual;
};

struct IECore {
    InferenceEngine::Core actual;
    explicit IECore(const std::string & xmlConfigFile = std::string());
    std::map<std::string, InferenceEngine::Version> getVersions(const std::string & deviceName);
    std::unique_ptr<InferenceEnginePython::IEExecNetwork> loadNetwork(IENetwork network, const std::string & deviceName,
            const std::map<std::string, std::string> & config, int num_requests);
    std::map<std::string, std::string> queryNetwork(IENetwork network, const std::string & deviceName,
                                       const std::map<std::string, std::string> & config);
    void setConfig(const std::map<std::string, std::string> &config, const std::string & deviceName = std::string());
    void registerPlugin(const std::string & pluginName, const std::string & deviceName);
    void unregisterPlugin(const std::string & deviceName);
    void registerPlugins(const std::string & xmlConfigFile);
    void addExtension(const std::string & ext_lib_path, const std::string & deviceName);
    std::vector<std::string> getAvailableDevices();
    PyObject* getMetric(const std::string & deviceName, const std::string & name);
    PyObject* getConfig(const std::string & deviceName, const std::string & name);
};

template<class T>
T *get_buffer(InferenceEngine::Blob &blob) {
    return blob.buffer().as<T *>();
}

template<class T, class... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::string get_version();
};  // namespace InferenceEnginePython
