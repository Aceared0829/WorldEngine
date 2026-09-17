#pragma once

#include "../TestClass/TestClass.h"
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>
W_DEFINE_AS_POD_TYPE(WTestShaderData);

class WRendererTestUtils
{
public:
  struct ImgColor
  {
    W_DECLARE_POD_TYPE();
    WUInt8 b;
    WUInt8 g;
    WUInt8 r;
    WUInt8 a;
  };

  static WTransform CreateTransform(const WUInt32 uiColumns, const WUInt32 uiRows, WUInt32 x, WUInt32 y);

  static void FillStructuredBuffer(WDynamicArray<WTestShaderData>& ref_instanceData, WUInt32 uiColorOffset = 0, WUInt32 uiSlotOffset = 0);

  static void CreateImage(WImage& ref_image, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevelCount, bool bMipLevelIsBlue, WUInt8 uiFixedBlue = 0);
};
