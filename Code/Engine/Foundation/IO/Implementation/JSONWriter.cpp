#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

WJSONWriter::WJSONWriter() = default;
WJSONWriter::~WJSONWriter() = default;

void WJSONWriter::AddVariableBool(WStringView sName, bool value)
{
  BeginVariable(sName);
  WriteBool(value);
  EndVariable();
}

void WJSONWriter::AddVariableInt32(WStringView sName, WInt32 value)
{
  BeginVariable(sName);
  WriteInt32(value);
  EndVariable();
}

void WJSONWriter::AddVariableUInt32(WStringView sName, WUInt32 value)
{
  BeginVariable(sName);
  WriteUInt32(value);
  EndVariable();
}

void WJSONWriter::AddVariableInt64(WStringView sName, WInt64 value)
{
  BeginVariable(sName);
  WriteInt64(value);
  EndVariable();
}

void WJSONWriter::AddVariableUInt64(WStringView sName, WUInt64 value)
{
  BeginVariable(sName);
  WriteUInt64(value);
  EndVariable();
}

void WJSONWriter::AddVariableFloat(WStringView sName, float value)
{
  BeginVariable(sName);
  WriteFloat(value);
  EndVariable();
}

void WJSONWriter::AddVariableDouble(WStringView sName, double value)
{
  BeginVariable(sName);
  WriteDouble(value);
  EndVariable();
}

void WJSONWriter::AddVariableString(WStringView sName, WStringView value)
{
  BeginVariable(sName);
  WriteString(value);
  EndVariable();
}

void WJSONWriter::AddVariableNULL(WStringView sName)
{
  BeginVariable(sName);
  WriteNULL();
  EndVariable();
}

void WJSONWriter::AddVariableTime(WStringView sName, WTime value)
{
  BeginVariable(sName);
  WriteTime(value);
  EndVariable();
}

void WJSONWriter::AddVariableUuid(WStringView sName, WUuid value)
{
  BeginVariable(sName);
  WriteUuid(value);
  EndVariable();
}

void WJSONWriter::AddVariableAngle(WStringView sName, WAngle value)
{
  BeginVariable(sName);
  WriteAngle(value);
  EndVariable();
}

void WJSONWriter::AddVariableColor(WStringView sName, const WColor& value)
{
  BeginVariable(sName);
  WriteColor(value);
  EndVariable();
}

void WJSONWriter::AddVariableColorGamma(WStringView sName, const WColorGammaUB& value)
{
  BeginVariable(sName);
  WriteColorGamma(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec2(WStringView sName, const WVec2& value)
{
  BeginVariable(sName);
  WriteVec2(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec3(WStringView sName, const WVec3& value)
{
  BeginVariable(sName);
  WriteVec3(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec4(WStringView sName, const WVec4& value)
{
  BeginVariable(sName);
  WriteVec4(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec2I32(WStringView sName, const WVec2I32& value)
{
  BeginVariable(sName);
  WriteVec2I32(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec3I32(WStringView sName, const WVec3I32& value)
{
  BeginVariable(sName);
  WriteVec3I32(value);
  EndVariable();
}

void WJSONWriter::AddVariableVec4I32(WStringView sName, const WVec4I32& value)
{
  BeginVariable(sName);
  WriteVec4I32(value);
  EndVariable();
}

void WJSONWriter::AddVariableQuat(WStringView sName, const WQuat& value)
{
  BeginVariable(sName);
  WriteQuat(value);
  EndVariable();
}

void WJSONWriter::AddVariableMat3(WStringView sName, const WMat3& value)
{
  BeginVariable(sName);
  WriteMat3(value);
  EndVariable();
}

void WJSONWriter::AddVariableMat4(WStringView sName, const WMat4& value)
{
  BeginVariable(sName);
  WriteMat4(value);
  EndVariable();
}

void WJSONWriter::AddVariableDataBuffer(WStringView sName, const WDataBuffer& value)
{
  BeginVariable(sName);
  WriteDataBuffer(value);
  EndVariable();
}

void WJSONWriter::AddVariableVariant(WStringView sName, const WVariant& value)
{
  BeginVariable(sName);
  WriteVariant(value);
  EndVariable();
}

void WJSONWriter::WriteColor(const WColor& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WColor is not supported by this JSON writer.");
}

void WJSONWriter::WriteColorGamma(const WColorGammaUB& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WColorGammaUB is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec2(const WVec2& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec2 is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec3(const WVec3& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec3 is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec4(const WVec4& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec4 is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec2I32(const WVec2I32& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec2I32 is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec3I32(const WVec3I32& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec3I32 is not supported by this JSON writer.");
}

void WJSONWriter::WriteVec4I32(const WVec4I32& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WVec4I32 is not supported by this JSON writer.");
}

void WJSONWriter::WriteQuat(const WQuat& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WQuat is not supported by this JSON writer.");
}

void WJSONWriter::WriteMat3(const WMat3& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WMat3 is not supported by this JSON writer.");
}

void WJSONWriter::WriteMat4(const WMat4& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WMat4 is not supported by this JSON writer.");
}

void WJSONWriter::WriteDataBuffer(const WDataBuffer& value)
{
  W_IGNORE_UNUSED(value);
  W_REPORT_FAILURE("The complex data type WDateBuffer is not supported by this JSON writer.");
}

void WJSONWriter::WriteVariant(const WVariant& value)
{
  switch (value.GetType())
  {
    case WVariant::Type::Invalid:
      // W_REPORT_FAILURE("Variant of Type 'Invalid' cannot be written as JSON.");
      WriteNULL();
      return;
    case WVariant::Type::Bool:
      WriteBool(value.Get<bool>());
      return;
    case WVariant::Type::Int8:
      WriteInt32(value.Get<WInt8>());
      return;
    case WVariant::Type::UInt8:
      WriteUInt32(value.Get<WUInt8>());
      return;
    case WVariant::Type::Int16:
      WriteInt32(value.Get<WInt16>());
      return;
    case WVariant::Type::UInt16:
      WriteUInt32(value.Get<WUInt16>());
      return;
    case WVariant::Type::Int32:
      WriteInt32(value.Get<WInt32>());
      return;
    case WVariant::Type::UInt32:
      WriteUInt32(value.Get<WUInt32>());
      return;
    case WVariant::Type::Int64:
      WriteInt64(value.Get<WInt64>());
      return;
    case WVariant::Type::UInt64:
      WriteUInt64(value.Get<WUInt64>());
      return;
    case WVariant::Type::Float:
      WriteFloat(value.Get<float>());
      return;
    case WVariant::Type::Double:
      WriteDouble(value.Get<double>());
      return;
    case WVariant::Type::Color:
      WriteColor(value.Get<WColor>());
      return;
    case WVariant::Type::ColorGamma:
      WriteColorGamma(value.Get<WColorGammaUB>());
      return;
    case WVariant::Type::Vector2:
      WriteVec2(value.Get<WVec2>());
      return;
    case WVariant::Type::Vector3:
      WriteVec3(value.Get<WVec3>());
      return;
    case WVariant::Type::Vector4:
      WriteVec4(value.Get<WVec4>());
      return;
    case WVariant::Type::Vector2I:
      WriteVec2I32(value.Get<WVec2I32>());
      return;
    case WVariant::Type::Vector3I:
      WriteVec3I32(value.Get<WVec3I32>());
      return;
    case WVariant::Type::Vector4I:
      WriteVec4I32(value.Get<WVec4I32>());
      return;
    // The unsigned vectors have no Write function of their own, because nothing else needs to
    // distinguish them from the signed ones. Written directly so they do not end up at the failure
    // below - the values do not fit an WVec*I32 across their whole range.
    case WVariant::Type::Vector2U:
    {
      const WVec2U32 v = value.Get<WVec2U32>();
      BeginObject();
      AddVariableUInt32("x", v.x);
      AddVariableUInt32("y", v.y);
      EndObject();
      return;
    }
    case WVariant::Type::Vector3U:
    {
      const WVec3U32 v = value.Get<WVec3U32>();
      BeginObject();
      AddVariableUInt32("x", v.x);
      AddVariableUInt32("y", v.y);
      AddVariableUInt32("z", v.z);
      EndObject();
      return;
    }
    case WVariant::Type::Vector4U:
    {
      const WVec4U32 v = value.Get<WVec4U32>();
      BeginObject();
      AddVariableUInt32("x", v.x);
      AddVariableUInt32("y", v.y);
      AddVariableUInt32("z", v.z);
      AddVariableUInt32("w", v.w);
      EndObject();
      return;
    }
    case WVariant::Type::Transform:
    {
      const WTransform t = value.Get<WTransform>();
      BeginObject();
      AddVariableVec3("position", t.m_vPosition);
      AddVariableQuat("rotation", t.m_qRotation);
      AddVariableVec3("scale", t.m_vScale);
      EndObject();
      return;
    }
    case WVariant::Type::Quaternion:
      WriteQuat(value.Get<WQuat>());
      return;
    case WVariant::Type::Matrix3:
      WriteMat3(value.Get<WMat3>());
      return;
    case WVariant::Type::Matrix4:
      WriteMat4(value.Get<WMat4>());
      return;
    case WVariant::Type::String:
      WriteString(value.Get<WString>().GetData());
      return;
    case WVariant::Type::StringView:
    {
      WStringBuilder s = value.Get<WStringView>();
      WriteString(s.GetData());
      return;
    }
    case WVariant::Type::HashedString:
      WriteString(value.Get<WHashedString>().GetView());
      return;
    case WVariant::Type::TempHashedString:
      // Only the hash exists here - an WTempHashedString does not keep the text it was built from, so
      // this is as much as can be written, and it does not round trip back into a string.
      WriteUInt64(value.Get<WTempHashedString>().GetHash());
      return;
    case WVariant::Type::Time:
      WriteTime(value.Get<WTime>());
      return;
    case WVariant::Type::Uuid:
      WriteUuid(value.Get<WUuid>());
      return;
    case WVariant::Type::Angle:
      WriteAngle(value.Get<WAngle>());
      return;
    case WVariant::Type::DataBuffer:
      WriteDataBuffer(value.Get<WDataBuffer>());
      return;
    case WVariant::Type::VariantArray:
    {
      BeginArray();

      const auto& ar = value.Get<WVariantArray>();

      for (const auto& val : ar)
      {
        WriteVariant(val);
      }

      EndArray();
    }
      return;
    case WVariant::Type::VariantDictionary:
    {
      BeginObject();

      const auto& dict = value.Get<WVariantDictionary>();

      for (auto& kv : dict)
      {
        AddVariableVariant(kv.Key(), kv.Value());
      }
      EndObject();
    }
      return;

    default:
      break;
  }

  W_REPORT_FAILURE("The Variant Type {0} is not supported by WJSONWriter::WriteVariant.", value.GetType());
}


bool WJSONWriter::HadWriteError() const
{
  return m_bHadWriteError;
}

void WJSONWriter::SetWriteErrorState()
{
  m_bHadWriteError = true;
}
