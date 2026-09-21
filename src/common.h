#pragma once
#ifdef _WIN32
inline constexpr const char *SHOWPOS_PLATFORM = "windows";
inline constexpr const char *SHOWPOS_PLATFORM_LABEL = "Windows x64";
inline constexpr const char *SHOWPOS_SERVER_MODULE = "server.dll";
#else
inline constexpr const char *SHOWPOS_PLATFORM = "linux";
inline constexpr const char *SHOWPOS_PLATFORM_LABEL = "Linux x86_64";
inline constexpr const char *SHOWPOS_SERVER_MODULE = "libserver.so";
#endif
#ifdef _WIN32
#pragma warning(disable: 4146 4244 4267 4018 4099 4005 5033)
#endif
#include <ISmmPlugin.h>
#include <cstdint>
#include "eiface.h"
#include "entity2/entitysystem.h"
#include "entity2/entityinstance.h"
#include "tier1/strtools.h"
static_assert(METAMOD_PLAPI_VERSION == 18, "Build with Metamod plugin API 18");
PLUGIN_GLOBALVARS();
using i32 = int32_t;
using f32 = float;
using f64 = double;
#define static_function static
