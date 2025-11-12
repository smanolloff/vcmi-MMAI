#pragma once

#if defined(USING_EXECUTORCH)
#include "TorchModel_ET.h"
#elif defined(USING_LIBTORCH)
#include "TorchModel_LT.h"
#elif defined(USING_ONNX)
#include "TorchModel_onnx.h"
#endif
