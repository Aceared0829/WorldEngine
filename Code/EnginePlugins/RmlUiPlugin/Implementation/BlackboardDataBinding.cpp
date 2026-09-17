#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>

namespace WRmlUiInternal
{
  Rml::DataVariableType GetVariableType(const WVariant& value)
  {
    if (value.IsA<WVariantArray>())
      return Rml::DataVariableType::Array;

    if (value.IsA<WVariantDictionary>())
      return Rml::DataVariableType::Struct;

    return Rml::DataVariableType::Scalar;
  }

  /// Shared implementation for looking up a child of an array or dictionary value.
  Rml::DataVariable GetChild(const WVariant& value, Rml::DataVariableType type, const Rml::DataAddressEntry& address, const VariantDefinitionSet& definitions)
  {
    if (type == Rml::DataVariableType::Array)
    {
      if (!value.IsA<WVariantArray>())
        return Rml::DataVariable();

      const WVariantArray& a = value.Get<WVariantArray>();

      const int index = address.index;
      const int count = static_cast<int>(a.GetCount());
      if (index < 0 || index >= count)
      {
        if (address.name == "size")
          return Rml::MakeLiteralIntVariable(count);

        WLog::Warning("Data array index out of bounds.");
        return Rml::DataVariable();
      }

      return definitions.GetDefinition(a[index]);
    }

    if (type == Rml::DataVariableType::Struct)
    {
      if (!value.IsA<WVariantDictionary>())
        return Rml::DataVariable();

      if (address.name.empty())
      {
        WLog::Warning("Expected a dictionary member name but none was given.");
        return Rml::DataVariable();
      }

      const WVariantDictionary& d = value.Get<WVariantDictionary>();

      const WStringView sName = WRmlUiConversionUtils::ToStringView(address.name);
      const WVariant* pElement = nullptr;
      if (!d.TryGetValue(sName, pElement))
      {
        WLog::Warning("Member '{}' not found in variant dictionary.", sName);
        return Rml::DataVariable();
      }

      return definitions.GetDefinition(*pElement);
    }

    WLog::Warning("Tried to get the child of a scalar type.");
    return Rml::DataVariable();
  }

  //////////////////////////////////////////////////////////////////

  VariantDefinitionSet::VariantDefinitionSet()
    : m_Scalar(Rml::DataVariableType::Scalar, *this)
    , m_Array(Rml::DataVariableType::Array, *this)
    , m_Struct(Rml::DataVariableType::Struct, *this)
  {
  }

  Rml::DataVariable VariantDefinitionSet::GetDefinition(const WVariant& value) const
  {
    const VariantVariableDefinition* pDefinition = &m_Scalar;

    switch (GetVariableType(value))
    {
      case Rml::DataVariableType::Array:
        pDefinition = &m_Array;
        break;
      case Rml::DataVariableType::Struct:
        pDefinition = &m_Struct;
        break;
      default:
        break;
    }

    // The RmlUi interface is non-const throughout, but neither the definitions nor the value are modified through it.
    return Rml::DataVariable(const_cast<VariantVariableDefinition*>(pDefinition), const_cast<WVariant*>(&value));
  }

  //////////////////////////////////////////////////////////////////

  VariantVariableDefinition::VariantVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions)
    : VariableDefinition(type)
    , m_Definitions(definitions)
  {
  }

  bool VariantVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    WVariant* pValue = static_cast<WVariant*>(pPtr);
    out_variant = WRmlUiConversionUtils::ToVariant(*pValue);

    return true;
  }

  bool VariantVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    WLog::Warning("Can't set the value of an element inside a variant array or dictionary. Nested values are read-only.");

    return false;
  }

  int VariantVariableDefinition::Size(void* pPtr)
  {
    WVariant* pValue = static_cast<WVariant*>(pPtr);

    if (pValue->IsA<WVariantArray>())
    {
      return static_cast<int>(pValue->Get<WVariantArray>().GetCount());
    }

    return 0;
  }

  Rml::DataVariable VariantVariableDefinition::Child(void* pPtr, const Rml::DataAddressEntry& address)
  {
    WVariant* pValue = static_cast<WVariant*>(pPtr);

    return GetChild(*pValue, Type(), address, m_Definitions);
  }

  Rml::StringList VariantVariableDefinition::ReflectMemberNames()
  {
    // RmlUi does not pass the instance pointer here, so the concrete dictionary and therefore its
    // keys are unknown at this point. Member access via 'obj.key' works regardless, only iterating
    // over the members of a nested dictionary with 'data-for' is unsupported.
    if (Type() == Rml::DataVariableType::Struct)
    {
      WLog::Warning("Iterating over the members of a variant dictionary is not supported. Access the members by name instead.");
      return Rml::StringList();
    }

    return VariableDefinition::ReflectMemberNames();
  }

  ////////////////////////////////////////////////////////////////

  BlackboardVariableDefinition::BlackboardVariableDefinition(Rml::DataVariableType type, const VariantDefinitionSet& definitions)
    : VariableDefinition(type)
    , m_Definitions(definitions)
  {
  }

  bool BlackboardVariableDefinition::Get(void* pPtr, Rml::Variant& out_variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    out_variant = WRmlUiConversionUtils::ToVariant(pInfo->m_CachedValue);

    return true;
  }

  bool BlackboardVariableDefinition::Set(void* pPtr, const Rml::Variant& variant)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    if (Type() != Rml::DataVariableType::Scalar)
    {
      WLog::Warning("Can't set the value of a variant array or dictionary. Only scalar entries are writable.");
      return false;
    }

    WVariant::Type::Enum targetType = WVariant::Type::Invalid;
    if (auto pEntry = pInfo->m_pBlackboard->GetEntry(pInfo->m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    pInfo->m_CachedValue = WRmlUiConversionUtils::ToVariant(variant, targetType);

    pInfo->m_pBlackboard->SetEntryValue(pInfo->m_sName, pInfo->m_CachedValue);

    return true;
  }

  int BlackboardVariableDefinition::Size(void* pPtr)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    if (pInfo->m_CachedValue.IsA<WVariantArray>())
    {
      return static_cast<int>(pInfo->m_CachedValue.Get<WVariantArray>().GetCount());
    }

    return 0;
  }

  Rml::DataVariable BlackboardVariableDefinition::Child(void* pPtr, const Rml::DataAddressEntry& address)
  {
    auto pInfo = static_cast<EntryInfo*>(pPtr);

    return GetChild(pInfo->m_CachedValue, Type(), address, m_Definitions);
  }

  Rml::StringList BlackboardVariableDefinition::ReflectMemberNames()
  {
    // See VariantVariableDefinition::ReflectMemberNames.
    if (Type() == Rml::DataVariableType::Struct)
    {
      WLog::Warning("Iterating over the members of a variant dictionary is not supported. Access the members by name instead.");
      return Rml::StringList();
    }

    return VariableDefinition::ReflectMemberNames();
  }

  //////////////////////////////////////////////////////////////////

  BlackboardDataBinding::BlackboardDataBinding(const WSharedPtr<WBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
    , m_ScalarDefinition(Rml::DataVariableType::Scalar, m_VariantDefinitions)
    , m_ArrayDefinition(Rml::DataVariableType::Array, m_VariantDefinitions)
    , m_StructDefinition(Rml::DataVariableType::Struct, m_VariantDefinitions)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  WResult BlackboardDataBinding::Initialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard == nullptr)
      return W_FAILURE;

    const char* szModelName = m_pBlackboard->GetName();
    if (WStringUtils::IsNullOrEmpty(szModelName))
    {
      WLog::Error("Can't bind a blackboard without a valid name");
      return W_FAILURE;
    }

    Rml::DataModelConstructor constructor = ref_context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return W_FAILURE;
    }

    for (auto it : m_pBlackboard->GetAllEntries())
    {
      auto type = it.Value().m_Value.GetType();
      if ((type >= WVariantType::Invalid && type <= WVariantType::Double) ||
          type == WVariantType::String || type == WVariantType::HashedString ||
          type == WVariantType::VariantArray || type == WVariantType::VariantDictionary)
      {
        auto& info = m_EntryInfos.ExpandAndGetRef();
        info.m_pBlackboard = m_pBlackboard;
        info.m_sName = it.Key();
        info.m_uiChangeCounter = it.Value().m_uiChangeCounter;
        info.m_CachedValue = it.Value().m_Value;
        info.m_Type = GetVariableType(info.m_CachedValue);
      }
    }

    for (auto& info : m_EntryInfos)
    {
      BlackboardVariableDefinition* pDefinition = &m_ScalarDefinition;
      if (info.m_Type == Rml::DataVariableType::Array)
      {
        pDefinition = &m_ArrayDefinition;
      }
      else if (info.m_Type == Rml::DataVariableType::Struct)
      {
        pDefinition = &m_StructDefinition;
      }

      constructor.BindCustomDataVariable(WRmlUiConversionUtils::ToString(info.m_sName), Rml::DataVariable(pDefinition, &info));
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();

    return W_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard != nullptr)
    {
      ref_context.RemoveDataModel(m_pBlackboard->GetName());
    }
  }

  bool BlackboardDataBinding::Update()
  {
    bool bUpdated = false;

    if (m_uiBlackboardChangeCounter != m_pBlackboard->GetBlackboardChangeCounter())
    {
      WLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_pBlackboard->GetBlackboardEntryChangeCounter())
    {
      for (auto& info : m_EntryInfos)
      {
        auto pEntry = m_pBlackboard->GetEntry(info.m_sName);

        if (pEntry != nullptr && info.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          // Refresh the cached value before marking the variable dirty, so that RmlUi reads the
          // new value and any pointers it takes into nested elements stay valid.
          info.m_CachedValue = pEntry->m_Value;

          const Rml::DataVariableType newType = GetVariableType(info.m_CachedValue);
          if (newType != info.m_Type)
          {
            WLog::Warning("Blackboard entry '{}' changed its data variable type after setup. This is not supported, the binding will keep using the original type.", info.m_sName);
          }

          m_hDataModel.DirtyVariable(WRmlUiConversionUtils::ToString(info.m_sName));
          info.m_uiChangeCounter = pEntry->m_uiChangeCounter;
          bUpdated = true;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }

    return bUpdated;
  }

} // namespace WRmlUiInternal
