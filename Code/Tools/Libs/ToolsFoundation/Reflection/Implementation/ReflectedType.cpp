#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WAttributeHolder, WNoBase, 1, WRTTINoAllocator)
{
  flags.Add(WTypeFlags::Abstract);
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WAttributeHolder::WAttributeHolder() = default;

WAttributeHolder::WAttributeHolder(const WAttributeHolder& rhs)
{
  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

WAttributeHolder::~WAttributeHolder()
{
  for (auto pAttr : m_Attributes)
  {
    if (pAttr)
      pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
}

void WAttributeHolder::operator=(const WAttributeHolder& rhs)
{
  if (this == &rhs)
    return;

  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

WUInt32 WAttributeHolder::GetCount() const
{
  return WMath::Max(m_ReferenceAttributes.GetCount(), m_Attributes.GetCount());
}

const WPropertyAttribute* WAttributeHolder::GetValue(WUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void WAttributeHolder::SetValue(WUInt32 uiIndex, const WPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void WAttributeHolder::Insert(WUInt32 uiIndex, const WPropertyAttribute* value)
{
  m_Attributes.InsertAt(uiIndex, value);
}

void WAttributeHolder::Remove(WUInt32 uiIndex)
{
  m_Attributes.RemoveAtAndCopy(uiIndex);
}

////////////////////////////////////////////////////////////////////////
// WReflectedPropertyDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WReflectedPropertyDescriptor, WAttributeHolder, 2, WRTTIDefaultAllocator<WReflectedPropertyDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_MEMBER_PROPERTY("Category", WPropertyCategory, m_Category),
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Type", m_sType),
    W_BITFLAGS_MEMBER_PROPERTY("Flags", WPropertyFlags, m_Flags),
    W_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

class WReflectedPropertyDescriptorPatch_1_2 : public WGraphPatch
{
public:
  WReflectedPropertyDescriptorPatch_1_2()
    : WGraphPatch("WReflectedPropertyDescriptor", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    if (WAbstractObjectNode::Property* pProp = pNode->FindProperty("Flags"))
    {
      WStringBuilder sValue = pProp->m_Value.Get<WString>();
      WTempHybridArray<WStringView, 32> values;
      sValue.Split(false, values, "|");

      WStringBuilder sNewValue;
      for (WInt32 i = (WInt32)values.GetCount() - 1; i >= 0; i--)
      {
        if (values[i].IsEqual("WPropertyFlags::Constant"))
        {
          values.RemoveAtAndCopy(i);
        }
        else if (values[i].IsEqual("WPropertyFlags::EmbeddedClass"))
        {
          values[i] = WStringView("WPropertyFlags::Class");
        }
        else if (values[i].IsEqual("WPropertyFlags::Pointer"))
        {
          values.PushBack(WStringView("WPropertyFlags::Class"));
        }
      }
      for (WUInt32 i = 0; i < values.GetCount(); ++i)
      {
        if (i != 0)
          sNewValue.Append("|");
        sNewValue.Append(values[i]);
      }
      pProp->m_Value = sNewValue.GetData();
    }
  }
};

WReflectedPropertyDescriptorPatch_1_2 g_WReflectedPropertyDescriptorPatch_1_2;


WReflectedPropertyDescriptor::WReflectedPropertyDescriptor(WPropertyCategory::Enum category, WStringView sName, WStringView sType, WBitflags<WPropertyFlags> flags)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
}

WReflectedPropertyDescriptor::WReflectedPropertyDescriptor(WPropertyCategory::Enum category, WStringView sName, WStringView sType,
  WBitflags<WPropertyFlags> flags, WArrayPtr<const WPropertyAttribute* const> attributes)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

WReflectedPropertyDescriptor::WReflectedPropertyDescriptor(
  WStringView sName, const WVariant& constantValue, WArrayPtr<const WPropertyAttribute* const> attributes)
  : m_Category(WPropertyCategory::Constant)
  , m_sName(sName)
  , m_sType()
  , m_Flags(WPropertyFlags::StandardType | WPropertyFlags::ReadOnly)
  , m_ConstantValue(constantValue)
{
  m_ReferenceAttributes = attributes;
  const WRTTI* pType = WReflectionUtils::GetTypeFromVariant(constantValue);
  if (pType)
    m_sType = pType->GetTypeName();
}

WReflectedPropertyDescriptor::WReflectedPropertyDescriptor(const WReflectedPropertyDescriptor& rhs)
{
  operator=(rhs);
}

void WReflectedPropertyDescriptor::operator=(const WReflectedPropertyDescriptor& rhs)
{
  m_Category = rhs.m_Category;
  m_sName = rhs.m_sName;

  m_sType = rhs.m_sType;

  m_Flags = rhs.m_Flags;
  m_ConstantValue = rhs.m_ConstantValue;

  WAttributeHolder::operator=(rhs);
}

WReflectedPropertyDescriptor::~WReflectedPropertyDescriptor() = default;


////////////////////////////////////////////////////////////////////////
// WFunctionParameterDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WFunctionArgumentDescriptor, WNoBase, 1, WRTTIDefaultAllocator<WFunctionArgumentDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_sType),
    W_BITFLAGS_MEMBER_PROPERTY("Flags", WPropertyFlags, m_Flags),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WFunctionArgumentDescriptor::WFunctionArgumentDescriptor() = default;

WFunctionArgumentDescriptor::WFunctionArgumentDescriptor(WStringView sType, WBitflags<WPropertyFlags> flags)
  : m_sType(sType)
  , m_Flags(flags)
{
}


////////////////////////////////////////////////////////////////////////
// WReflectedFunctionDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WReflectedFunctionDescriptor, WAttributeHolder, 1, WRTTIDefaultAllocator<WReflectedFunctionDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_BITFLAGS_MEMBER_PROPERTY("Flags", WPropertyFlags, m_Flags),
    W_ENUM_MEMBER_PROPERTY("Type", WFunctionType, m_Type),
    W_MEMBER_PROPERTY("ReturnValue", m_ReturnValue),
    W_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WReflectedFunctionDescriptor::WReflectedFunctionDescriptor() = default;

WReflectedFunctionDescriptor::WReflectedFunctionDescriptor(WStringView sName, WBitflags<WPropertyFlags> flags, WEnum<WFunctionType> type, WArrayPtr<const WPropertyAttribute* const> attributes)
  : m_sName(sName)
  , m_Flags(flags)
  , m_Type(type)
{
  m_ReferenceAttributes = attributes;
}

WReflectedFunctionDescriptor::WReflectedFunctionDescriptor(const WReflectedFunctionDescriptor& rhs)
{
  operator=(rhs);
}

WReflectedFunctionDescriptor::~WReflectedFunctionDescriptor() = default;

void WReflectedFunctionDescriptor::operator=(const WReflectedFunctionDescriptor& rhs)
{
  m_sName = rhs.m_sName;
  m_Flags = rhs.m_Flags;
  m_Type = rhs.m_Type;
  m_ReturnValue = rhs.m_ReturnValue;
  m_Arguments = rhs.m_Arguments;
  WAttributeHolder::operator=(rhs);
}

////////////////////////////////////////////////////////////////////////
// WReflectedTypeDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WReflectedTypeDescriptor, WAttributeHolder, 1, WRTTIDefaultAllocator<WReflectedTypeDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("TypeName", m_sTypeName),
    W_MEMBER_PROPERTY("PluginName", m_sPluginName),
    W_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
    W_BITFLAGS_MEMBER_PROPERTY("Flags", WTypeFlags, m_Flags),
    W_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
    W_ARRAY_MEMBER_PROPERTY("Functions", m_Functions),
    W_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WReflectedTypeDescriptor::~WReflectedTypeDescriptor() = default;
