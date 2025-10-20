// Profiling.hpp
#ifndef WORLDEXPLORER_CORE_PROFILING
#define WORLDEXPLORER_CORE_PROFILING

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#else
#define ZoneScopedNC(name, color)  // No-op macro when Tracy is disabled
#define ZoneScoped                 // Other Tracy macros as needed
#define ZoneScopedN(name)
#define FrameMark
// Add other Tracy macros you use
#endif
#endif  // WORLDEXPLORER_CORE_PROFILING
