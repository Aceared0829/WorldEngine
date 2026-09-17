#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

/// Describes how a color should be applied to another color.
struct WSetColorMode
{
  using StorageType = WUInt32;

  enum Enum
  {
    SetRGBA,    ///< Overrides all four RGBA values.
    SetRGB,     ///< Overrides the RGB values but leaves Alpha untouched.
    SetAlpha,   ///< Overrides Alpha, leaves RGB untouched.

    AlphaBlend, ///< Modifies the target RGBA values by interpolating from the previous color towards the incoming color using the incoming alpha value.
    Additive,   ///< Adds to the RGBA values.
    Modulate,   /// Multiplies the RGBA values.

    Default = SetRGBA
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WSetColorMode);

/// A message to modify the main color of some thing.
///
/// Components that handle this message use it to change their main color.
/// For instance a light component may change its light color, a mesh component will change the main mesh color.
struct W_CORE_DLL WMsgSetColor : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetColor, WMessage);

  /// The color to apply to the target.
  WColor m_Color;

  /// The mode with which to apply the color to the target.
  WEnum<WSetColorMode> m_Mode;

  /// Applies m_Color using m_Mode to the given color.
  void ModifyColor(WColor& ref_color) const;

  /// Applies m_Color using m_Mode to the given color.
  void ModifyColor(WColorGammaUB& ref_color) const;

  virtual void Serialize(WStreamWriter& inout_stream) const override;
  virtual void Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion) override;
};

/// Message to set custom float data on components.
///
/// Provides four float values that can be used to pass arbitrary data to components.
/// The interpretation of these values depends on the receiving component.
struct W_CORE_DLL WMsgSetCustomData : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgSetCustomData, WMessage);

  WVec4 m_vData;

  virtual void Serialize(WStreamWriter& inout_stream) const override;
  virtual void Deserialize(WStreamReader& inout_stream, WUInt8 uiTypeVersion) override;
};
