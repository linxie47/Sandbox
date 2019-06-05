#include "proc_factory.h"

PostProcFunction getPostProcFunctionByName(const char *name)
{
    if (!strcmp(name, "ie_detect"))
        return (PostProcFunction)ExtractBoundingBoxes;
    return nullptr;
}

int findModelPostProcByName(ModelOutputPostproc *model_postproc, const char *layer_name)
{
    int proc_id;
    // search model postproc
    for (proc_id = 0; proc_id < MAX_MODEL_OUTPUT; proc_id++) {
        char *proc_layer_name = model_postproc->procs[proc_id].layer_name;
        // skip this output process
        if (!proc_layer_name)
            continue;
        if (!strcmp(layer_name, proc_layer_name))
            return proc_id;
    }

    av_log(NULL, AV_LOG_DEBUG, "Could not find proc:%s\n", layer_name);
    return proc_id;
}