#include <Mcp/McpPCH.h>

#include <Mcp/McpJsonWriter.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Uuid.h>

WMcpJsonWriter::WMcpJsonWriter()
  : m_Writer(&m_Storage)
{
  SetOutputStream(&m_Writer);

  // every character here is a token that the client pays for, and nothing reads this by eye
  SetWhitespaceMode(WJSONWriter::WhitespaceMode::None);
  SetArrayMode(WJSONWriter::ArrayMode::InOneLine);
}

WMcpJsonWriter::~WMcpJsonWriter() = default;

WStringView WMcpJsonWriter::GetResult()
{
  const WArrayPtr<const WUInt8> bytes = m_Storage.GetContiguousMemoryRange(0);

  m_sResult = WStringView(reinterpret_cast<const char*>(bytes.GetPtr()), bytes.GetCount());

  return m_sResult.GetView();
}

void WMcpJsonWriter::WriteVariant(const WVariant& value)
{
  switch (value.GetType())
  {
    case WVariant::Type::TypedPointer:
    {
      // Written as a description of the target rather than as its contents: the pointer may be null,
      // may point at a type with cycles, and the model can look the type up with the rtti tools anyway.
      const WTypedPointer ptr = value.Get<WTypedPointer>();

      const WRTTI* pType = ptr.m_pType;

      // The variant carries the *declared* pointer type, which for a reflected container is the base
      // class - an array of WPropertyAttribute* reports 'WPropertyAttribute' for every element,
      // losing which attribute it actually is. When the target derives from WReflectedClass it can
      // say so itself, and that is the name worth reporting.
      if (ptr.m_pObject != nullptr && pType != nullptr && pType->IsDerivedFrom<WReflectedClass>())
      {
        pType = static_cast<const WReflectedClass*>(ptr.m_pObject)->GetDynamicRTTI();
      }

      BeginObject();
      AddVariableString("$type", pType != nullptr ? pType->GetTypeName() : WStringView());

      if (ptr.m_pObject == nullptr)
      {
        AddVariableBool("$null", true);
      }

      EndObject();
      return;
    }

    case WVariant::Type::TypedObject:
    {
      // Only the type is written. Serialising the members would need the property system and could
      // recurse without bound, which is not what a JSON writer should be doing.
      const WRTTI* pType = value.GetReflectedType();

      BeginObject();
      AddVariableString("$type", pType != nullptr ? pType->GetTypeName() : WStringView());
      EndObject();
      return;
    }

    default:
      break;
  }

  // The base class ends in W_REPORT_FAILURE for a type its switch does not cover, and an assert here
  // is a dead editor in the middle of answering a tool call. Everything the enum currently defines is handled by
  // one of the two writers, so this only catches a type added later.
  const WVariant::Type::Enum type = value.GetType();

  const bool bBaseHandlesIt = (type > WVariant::Type::Invalid && type < WVariant::Type::LastStandardType) ||
                              type == WVariant::Type::VariantArray || type == WVariant::Type::VariantDictionary ||
                              type == WVariant::Type::Invalid;

  if (bBaseHandlesIt)
  {
    WStandardJSONWriter::WriteVariant(value);
    return;
  }

  BeginObject();
  AddVariableString("$type", value.GetReflectedType() != nullptr ? value.GetReflectedType()->GetTypeName() : WStringView());
  AddVariableUInt32("$variantType", static_cast<WUInt32>(type));
  AddVariableString("$note", "This value's type has no JSON representation, so only its type is reported.");
  EndObject();
}
