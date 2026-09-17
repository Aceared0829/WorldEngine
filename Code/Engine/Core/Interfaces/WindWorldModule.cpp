#include <Core/CorePCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WWindWorldModuleInterface, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_ENUM(WWindStrength, 1)
  W_ENUM_CONSTANTS(WWindStrength::None, WWindStrength::Calm, WWindStrength::LightBreeze, WWindStrength::GentleBreeze, WWindStrength::ModerateBreeze, WWindStrength::StrongBreeze, WWindStrength::Storm)
  W_ENUM_CONSTANTS(WWindStrength::WeakShockwave, WWindStrength::MediumShockwave, WWindStrength::StrongShockwave, WWindStrength::ExtremeShockwave)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

float WWindStrength::GetInMetersPerSecond(Enum strength)
{
  // inspired by the Beaufort scale
  // https://en.wikipedia.org/wiki/Beaufort_scale

  switch (strength)
  {
    case None:
      return 0.0f;

    case Calm:
      return 0.5f;

    case LightBreeze:
      return 2.0f;

    case GentleBreeze:
      return 5.0f;

    case ModerateBreeze:
      return 9.0f;

    case StrongBreeze:
      return 14.0f;

    case Storm:
      return 20.0f;

    case WeakShockwave:
      return 40.0f;

    case MediumShockwave:
      return 70.0f;

    case StrongShockwave:
      return 100.0f;

    case ExtremeShockwave:
      return 150.0f;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

WWindWorldModuleInterface::WWindWorldModuleInterface(WWorld* pWorld)
  : WWorldModule(pWorld)
{
}

WSimdVec4f WWindWorldModuleInterface::GetWindAtSimd(const WSimdVec4f& vPosition) const
{
  return WSimdConversion::ToVec3(GetWindAt(WSimdConversion::ToVec3(vPosition)));
}

WVec3 WWindWorldModuleInterface::ComputeWindFlutter(const WVec3& vWind, const WVec3& vObjectDir, float fFlutterSpeed, WUInt32 uiFlutterRandomOffset) const
{
  if (vWind.IsZero(0.001f))
    return WVec3::MakeZero();

  WVec3 windDir = vWind;
  const float fWindStrength = windDir.GetLengthAndNormalize();

  if (fWindStrength <= 0.01f)
    return WVec3::MakeZero();

  WVec3 mainDir = vObjectDir;
  mainDir.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

  WVec3 flutterDir = windDir.CrossRH(mainDir);
  flutterDir.NormalizeIfNotZero(WVec3::MakeAxisZ()).IgnoreResult();

  const float fFlutterOffset = (uiFlutterRandomOffset & 1023u) / 256.0f;

  const float fFlutter = WMath::Sin(WAngle::MakeFromRadian(fFlutterOffset + fFlutterSpeed * fWindStrength * GetWorld()->GetClock().GetAccumulatedTime().AsFloatInSeconds())) * fWindStrength;

  return flutterDir * fFlutter;
}

W_STATICLINK_FILE(Core, Core_Interfaces_WindWorldModule);
