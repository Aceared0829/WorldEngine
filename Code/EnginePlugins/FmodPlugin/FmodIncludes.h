#pragma once

#include <fmod_studio.hpp>

#define W_FMOD_ASSERT(func)                                                                         \
  {                                                                                                  \
    auto fmodErrorCode = func;                                                                       \
    if (fmodErrorCode != FMOD_OK)                                                                    \
      WLog::Error("FMOD call failed: '" W_PP_STRINGIFY(func) "' - Error code {0}", fmodErrorCode); \
  }
