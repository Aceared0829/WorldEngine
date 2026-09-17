#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/Types/Variant.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace WRmlUiConversionUtils
{
  WVariant ToVariant(const Rml::Variant& value, WVariant::Type::Enum targetType /*= WVariant::Type::Invalid*/)
  {
    WVariant result;

    switch (value.GetType())
    {
      case Rml::Variant::BOOL:
        result = value.Get<bool>();
        break;

      case Rml::Variant::CHAR:
        result = value.Get<char>();
        break;

      case Rml::Variant::BYTE:
        result = value.Get<Rml::byte>();
        break;

      case Rml::Variant::INT:
        result = value.Get<int>();
        break;

      case Rml::Variant::INT64:
        result = value.Get<WInt64>();
        break;

      case Rml::Variant::FLOAT:
        result = value.Get<float>();
        break;

      case Rml::Variant::DOUBLE:
        result = value.Get<double>();
        break;

      case Rml::Variant::STRING:
        result = ToString(value.Get<Rml::String>());
        break;

      default:
        break;
    }

    if (targetType != WVariant::Type::Invalid && result.IsValid())
    {
      WResult conversionResult = W_SUCCESS;
      result = result.ConvertTo(targetType, &conversionResult);

      if (conversionResult.Failed())
      {
        WLog::Warning("Failed to convert rml variant to target type '{}'", targetType);
      }
    }

    return result;
  }

  Rml::Variant ToVariant(const WVariant& value)
  {
    switch (value.GetType())
    {
      case WVariant::Type::Invalid:
        return Rml::Variant("&lt;Invalid&gt;");

      case WVariant::Type::Bool:
        return Rml::Variant(value.Get<bool>());

      case WVariant::Type::Int8:
        return Rml::Variant(value.Get<WInt8>());

      case WVariant::Type::UInt8:
        return Rml::Variant(value.Get<WUInt8>());

      case WVariant::Type::Int16:
      case WVariant::Type::UInt16:
      case WVariant::Type::Int32:
        return Rml::Variant(value.ConvertTo<int>());

      case WVariant::Type::UInt32:
      case WVariant::Type::Int64:
        return Rml::Variant(static_cast<int64_t>(value.ConvertTo<WInt64>()));

      case WVariant::Type::Float:
        return Rml::Variant(value.Get<float>());

      case WVariant::Type::Double:
        return Rml::Variant(value.Get<double>());

      case WVariant::Type::String:
        return Rml::Variant(value.Get<WString>());

      case WVariant::Type::HashedString:
        return Rml::Variant(value.Get<WHashedString>().GetString());

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        return Rml::Variant();
    }
  }

} // namespace WRmlUiConversionUtils
