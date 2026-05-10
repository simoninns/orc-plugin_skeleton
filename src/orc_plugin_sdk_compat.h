#pragma once

#include <cstdint>
#include <memory>

#if __has_include(<orc/plugin/orc_plugin_sdk.h>)
#include <orc/plugin/orc_plugin_sdk.h>
#elif __has_include(<orc_plugin_sdk.h>)
#include <orc_plugin_sdk.h>
#else
#error "Unable to locate decode-orc plugin SDK headers"
#endif

