#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

namespace WBakingInternal
{
  struct Volume
  {
    WSimdMat4f m_GlobalToLocalTransform;
  };
} // namespace WBakingInternal
