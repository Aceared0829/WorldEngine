#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/ParticleType.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleTypeFactory, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleType, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleType* WParticleTypeFactory::CreateType(WParticleSystemInstance* pOwner) const
{
  const WRTTI* pRtti = GetTypeType();

  WParticleType* pType = pRtti->GetAllocator()->Allocate<WParticleType>();
  pType->Reset(pOwner);

  CopyTypeProperties(pType, true);
  pType->CreateRequiredStreams();

  return pType;
}

WParticleType::WParticleType()
{
  m_uiLastExtractedFrame = 0;

  // run these as the last, after all the initializers and behaviors
  m_fPriority = +1000.0f;
}

WUInt32 WParticleType::ComputeSortingKey(WParticleTypeRenderMode::Enum mode, WUInt64 uiResource1Hash, WUInt64 uiResource2Hash)
{
  WUInt32 key = 0;

  switch (mode)
  {
    case WParticleTypeRenderMode::Additive:
      key = WParticleTypeSortingKey::Additive;
      break;

    case WParticleTypeRenderMode::Blended:
      key = WParticleTypeSortingKey::Blended;
      break;

    case WParticleTypeRenderMode::BlendedForeground:
      key = WParticleTypeSortingKey::BlendedForeground;
      break;

    case WParticleTypeRenderMode::BlendedBackground:
      key = WParticleTypeSortingKey::BlendedBackground;
      break;

    case WParticleTypeRenderMode::Opaque:
      key = WParticleTypeSortingKey::Opaque;
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  key <<= 32 - 3; // require 3 bits for the values above
  key |= WHashingUtils::StringHashTo32(uiResource1Hash) & 0x1FFFFFFFu;
  key |= WHashingUtils::StringHashTo32(uiResource2Hash) & 0x1FFFFFFFu;

  return key;
}

W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_ParticleType);
