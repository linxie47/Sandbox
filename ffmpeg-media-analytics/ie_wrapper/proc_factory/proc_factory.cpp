#include "proc_factory.h"

PostProcFunction getPostProcFunctionByName(const char *name)
{
    if (!strcmp(name, "ie_detect"))
        return (PostProcFunction)ExtractBoundingBoxes;
    return nullptr;
}
