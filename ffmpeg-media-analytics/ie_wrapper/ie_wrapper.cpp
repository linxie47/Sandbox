#include <sstream>

#include "ie_wrapper.h"
#include "ff_base_inference.h"

#define IE_WRAPPER_VERSION_MAJOR 1
#define IE_WRAPPER_VERSION_MINOR 1
#define IE_WRAPPER_VERSION_PATCH 0

extern "C" {

using namespace std;

const char* IEGetVersion(void)
{
    std::ostringstream ostr;
    ostr << IE_WRAPPER_VERSION_MAJOR << "."
         << IE_WRAPPER_VERSION_MINOR << "."
         << IE_WRAPPER_VERSION_PATCH << std::ends;
    std::string version = ostr.str();
    return version.c_str();
}

FFBaseInference *IECreateInference(FFInferenceParam *param)
{
    if (!param)
        return nullptr;

    FFBaseInference *base = new FFBaseInference();
#define INIT_BASE_STR(x) base->x = (param->x) ? strdup(param->x) : strdup("")
    INIT_BASE_STR(model);
    INIT_BASE_STR(object_class);
    INIT_BASE_STR(model_proc);
    INIT_BASE_STR(device);
    INIT_BASE_STR(inference_id);
    INIT_BASE_STR(cpu_streams);
    INIT_BASE_STR(infer_config);
    base->nireq = param->nireq;
    base->batch_size = param->batch_size;
    base->every_nth_frame = param->every_nth_frame;
    base->is_full_frame = param->is_full_frame;
#undef INIT_BASE_STR

    ff_base_inference_init(base);

    return base->initialized ? base : nullptr;
}

void IEReleaseInference(FFBaseInference *base)
{
    if (!base)
        return;
    ff_base_inference_release(base);
    delete base;
}

int IESendFrame(FFBaseInference *base, AVFrame *in)
{
    return ff_base_inference_send(base, in);
}

int IEGetProcesedFrame(FFBaseInference *base, ProcessedFrame *out)
{
    return ff_base_inference_fetch(base, out);
}

int IEOutputFrameQueueEmpty(FFBaseInference *base)
{
    return ff_base_inference_ouput_frame_queue_size(base) == 0;
}

void IESendEvent(FFBaseInference *base, int event)
{
    return ff_base_inference_sink_event(base, event);
}

} /* extern "C" */
