#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/Basics/RendererTestUtils.h>

WTransform WRendererTestUtils::CreateTransform(const WUInt32 uiColumns, const WUInt32 uiRows, WUInt32 x, WUInt32 y)
{
  WTransform t = WTransform::MakeIdentity();
  t.m_vScale = WVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
  t.m_vPosition = WVec3(WMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), WMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
  if (WClipSpaceYMode::RenderToTextureDefault == WClipSpaceYMode::Flipped)
  {
    WTransform flipY = WTransform::MakeIdentity();
    flipY.m_vScale.y *= -1.0f;
    t = flipY * t;
  }
  return t;
}
void WRendererTestUtils::FillStructuredBuffer(WDynamicArray<WTestShaderData>& ref_instanceData, WUInt32 uiColorOffset, WUInt32 uiSlotOffset)
{
  ref_instanceData.SetCount(16);
  const WUInt32 uiColumns = 4;
  const WUInt32 uiRows = 2;

  for (WUInt32 x = 0; x < uiColumns; ++x)
  {
    for (WUInt32 y = 0; y < uiRows; ++y)
    {
      WTestShaderData& instance = ref_instanceData[uiSlotOffset + x * uiRows + y];
      const float fColorIndex = float(uiColorOffset + x * uiRows + y) / 32.0f;
      instance.InstanceColor = WColorScheme::LightUI(fColorIndex).GetAsVec4();
      WTransform t = CreateTransform(uiColumns, uiRows, x, y);
      instance.InstanceTransform = t;
    }
  }
}
void WRendererTestUtils::CreateImage(WImage& ref_image, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevelCount, bool bMipLevelIsBlue, WUInt8 uiFixedBlue)
{
  WImageHeader header;
  header.SetImageFormat(WImageFormat::B8G8R8A8_UNORM_SRGB);
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetNumMipLevels(uiMipLevelCount);

  ref_image.ResetAndAlloc(header);
  for (WUInt32 m = 0; m < uiMipLevelCount; m++)
  {
    const WUInt32 uiHeight = ref_image.GetHeight(m);
    const WUInt32 uiWidth = ref_image.GetWidth(m);

    const WUInt8 uiBlue = bMipLevelIsBlue ? static_cast<WUInt8>(255.0f * float(m) / (uiMipLevelCount - 1)) : uiFixedBlue;
    for (WUInt32 y = 0; y < uiHeight; y++)
    {
      const WUInt8 uiGreen = static_cast<WUInt8>(255.0f * float(y) / (uiHeight - 1));
      for (WUInt32 x = 0; x < uiWidth; x++)
      {
        ImgColor* pColor = ref_image.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
        pColor->a = 255;
        pColor->b = uiBlue;
        pColor->g = uiGreen;
        pColor->r = static_cast<WUInt8>(255.0f * float(x) / (uiWidth - 1));
      }
    }
  }
}
