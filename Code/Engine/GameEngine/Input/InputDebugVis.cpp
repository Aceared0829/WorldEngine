#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Math/Rect.h>
#include <GameEngine/Input/InputDebugVis.h>
#include <RendererCore/Debug/DebugRenderer.h>

void WInputDebugVis::DebugRender(const WDebugRendererContext& context, const WVec2& vResolution, const WVirtualThumbStick& stick)
{
  if (!stick.IsEnabled())
    return;

  const bool bActive = stick.IsActive();

  WVec2 vLL, vUR;
  stick.GetInputArea(vLL, vUR);
  const WVec2 vAreaSize = vUR - vLL;

  const WVec2 vCenter = stick.GetCurrentCenter();
  const WVec2 vTouchPos = stick.GetCurrentTouchPos();
  const float fRadius = stick.GetThumbstickRadius();
  const float fStrength = stick.GetInputStrength();
  const float fAspect = stick.GetInputCoordinateAspectRatio();


  WRectFloat area;
  area.x = vLL.x * vResolution.x;
  area.y = vLL.y * vResolution.y;
  area.width = vAreaSize.x * vResolution.x;
  area.height = vAreaSize.y * vResolution.y;

  WDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? WColor::Yellow : WColor::Grey);

  area.x = (vCenter.x - fRadius) * vResolution.x;
  area.y = (vCenter.y * vResolution.y) - (fRadius * vResolution.y * fAspect);
  area.width = fRadius * 2 * vResolution.x;
  area.height = fRadius * 2 * vResolution.y * fAspect;
  WDebugRenderer::Draw2DLineRectangle(context, area, 0.0f, bActive ? WColor::GreenYellow : WColor::Yellow);

  if (bActive)
  {
    const float size = 0.03f;
    area.x = (vTouchPos.x - size) * vResolution.x;
    area.y = (vTouchPos.y * vResolution.y) - (size * vResolution.y * fAspect);
    area.width = size * 2 * vResolution.x;
    area.height = size * 2 * vResolution.y * fAspect;
    WDebugRenderer::Draw2DRectangle(context, area, 0.0f, WColor::OrangeRed);


    WVec2I32 pos;
    pos.x = WMath::RoundToInt(vCenter.x * vResolution.x);
    pos.y = WMath::RoundToInt(vCenter.y * vResolution.y);

    WDebugRenderer::Draw2DText(context, WFmt("{}", WArgF(fStrength, 2)), pos, WColor::OrangeRed, 16, WDebugTextHAlign::Center, WDebugTextVAlign::Center);
  }
}
