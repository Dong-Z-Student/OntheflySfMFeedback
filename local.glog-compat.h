#pragma once

// glog 0.7.1 declares a deprecated callback in its own public header.
// Limit this workaround to parsing that dependency; restore the caller's policy.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

#include <glog/logging.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
