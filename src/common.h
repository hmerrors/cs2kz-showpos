#pragma once
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
