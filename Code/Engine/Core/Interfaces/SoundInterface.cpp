#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Singleton.h>

WResult WSoundInterface::PlaySound(WWorld* pWorld, WStringView sResourceID, const WTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (WSoundInterface* pSoundInterface = WSingletonRegistry::GetSingletonInstance<WSoundInterface>())
  {
    return pSoundInterface->OneShotSound(pWorld, sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return W_FAILURE;
}

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Sound, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(PlaySound, In, "World", In, "Resource", In, "GlobalPosition", In, "GlobalRotation", In, "Pitch", In, "Volume", In, "BlockToLoad")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WDefaultValueAttribute(1.0f)),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(1.0f)),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(true))
    ),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Sound"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void WScriptExtensionClass_Sound::PlaySound(WWorld* pWorld, WStringView sResourceID, const WVec3& vGlobalPos, const WQuat& qGlobalRot, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  WSoundInterface::PlaySound(pWorld, sResourceID, WTransform(vGlobalPos, qGlobalRot), fPitch, fVolume, bBlockIfNotLoaded).IgnoreResult();
}


W_STATICLINK_FILE(Core, Core_Interfaces_SoundInterface);
