#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Foundation/Types/Variant.h>
#include <RmlUi/Include/RmlUi/Core.h>

namespace WRmlUiConversionUtils
{
  W_RMLUIPLUGIN_DLL WVariant ToVariant(const Rml::Variant& value, WVariant::Type::Enum targetType = WVariant::Type::Invalid);
  W_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const WVariant& value);

  // Strings
  W_ALWAYS_INLINE WString ToString(const Rml::String& value)
  {
    return WStringView(value.c_str(), static_cast<WUInt32>(value.length()));
  }

  W_ALWAYS_INLINE WString ToString(const Rml::StringView& value)
  {
    return WStringView(value.begin(), static_cast<WUInt32>(value.size()));
  }

  W_ALWAYS_INLINE Rml::String ToString(const WString& sValue)
  {
    return Rml::String(sValue.GetData(), sValue.GetElementCount());
  }

  W_ALWAYS_INLINE Rml::String ToString(WStringView sValue)
  {
    return Rml::String(sValue.GetStartPointer(), sValue.GetElementCount());
  }

  W_ALWAYS_INLINE WStringView ToStringView(const Rml::StringView& value)
  {
    return WStringView(value.begin(), static_cast<WUInt32>(value.size()));
  }

  W_ALWAYS_INLINE Rml::StringView ToStringView(WStringView sValue)
  {
    return Rml::StringView(sValue.GetStartPointer(), sValue.GetElementCount());
  }

  // Math
  W_ALWAYS_INLINE WVec2 ToVec2(const Rml::Vector2f& value)
  {
    return WVec2(value.x, value.y);
  }

  W_ALWAYS_INLINE Rml::Vector2f ToVec2(const WVec2& value)
  {
    return Rml::Vector2f(value.x, value.y);
  }

  W_ALWAYS_INLINE WColor ToColor(const Rml::Colourb& value)
  {
    return reinterpret_cast<const WColorLinearUB&>(value);
  }

  W_ALWAYS_INLINE WColor ToColor(const Rml::ColourbPremultiplied& value)
  {
    return reinterpret_cast<const WColorLinearUB&>(value);
  }

} // namespace WRmlUiConversionUtils
