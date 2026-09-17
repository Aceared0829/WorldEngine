#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WImageAddressMode, 1)
  W_ENUM_CONSTANT(WImageAddressMode::Repeat),
  W_ENUM_CONSTANT(WImageAddressMode::Clamp),
  W_ENUM_CONSTANT(WImageAddressMode::ClampBorder),
  W_ENUM_CONSTANT(WImageAddressMode::Mirror),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WTextureFilterSetting, 1)
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedNearest),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedBilinear),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedTrilinear),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedAnisotropic2x),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedAnisotropic4x),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedAnisotropic8x),
  W_ENUM_CONSTANT(WTextureFilterSetting::FixedAnisotropic16x),
  W_ENUM_CONSTANT(WTextureFilterSetting::LowestQuality),
  W_ENUM_CONSTANT(WTextureFilterSetting::LowQuality),
  W_ENUM_CONSTANT(WTextureFilterSetting::DefaultQuality),
  W_ENUM_CONSTANT(WTextureFilterSetting::HighQuality),
  W_ENUM_CONSTANT(WTextureFilterSetting::HighestQuality),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on


W_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageEnums);
