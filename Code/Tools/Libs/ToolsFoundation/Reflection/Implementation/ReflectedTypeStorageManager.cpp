#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

WMap<const WRTTI*, WReflectedTypeStorageManager::ReflectedTypeStorageMapping*> WReflectedTypeStorageManager::s_ReflectedTypeToStorageMapping;

// clang-format off
// 
W_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeStorageManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WReflectedTypeStorageManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WReflectedTypeStorageManager::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WReflectedTypeStorageManager::ReflectedTypeStorageMapping public functions
////////////////////////////////////////////////////////////////////////

void WReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const WRTTI* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function.
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = WVariant::Type::Invalid;
  }

  WSet<const WDocumentObject*> requiresPatchingEmbeddedClass;
  AddPropertiesRecursive(pType, requiresPatchingEmbeddedClass);

  for (const WDocumentObject* pObject : requiresPatchingEmbeddedClass)
  {
    pObject->GetDocumentObjectManager()->PatchEmbeddedClassObjects(pObject);
  }
}

void WReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(
  const WRTTI* pType, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  // Parse parent class
  const WRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    AddPropertiesRecursive(pParent, ref_requiresPatchingEmbeddedClass);

  // Parse properties
  const WUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (WUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const WAbstractProperty* pProperty = pType->GetProperties()[i];

    WString path = pProperty->GetPropertyName();

    StorageInfo* storageInfo = nullptr;
    if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
    {
      // Value already present, update type and instances
      storageInfo->m_Type = WToolsReflectionUtils::GetStorageType(pProperty);
      storageInfo->m_DefaultValue = WToolsReflectionUtils::GetStorageDefault(pProperty);
      UpdateInstances(storageInfo->m_uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
    else
    {
      const WUInt16 uiIndex = (WUInt16)m_PathToStorageInfoTable.GetCount();

      // Add value, new entries are appended
      m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, WToolsReflectionUtils::GetStorageType(pProperty), WToolsReflectionUtils::GetStorageDefault(pProperty)));
      AddPropertyToInstances(uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
  }
}

void WReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(
  WUInt32 uiIndex, const WAbstractProperty* pProperty, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    WDynamicArray<WVariant>& data = it.Key()->m_Data;
    W_ASSERT_DEV(uiIndex < data.GetCount(), "WReflectedTypeStorageAccessor found with fewer properties that is should have!");
    WVariant& value = data[uiIndex];

    const auto SpecVarType = WToolsReflectionUtils::GetStorageType(pProperty);

    switch (pProperty->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        if (pProperty->GetFlags().IsSet(WPropertyFlags::Class) && !pProperty->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            if (!value.Get<WUuid>().IsValid())
            {
              ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
            }
          }
          else
          {
            value = WToolsReflectionUtils::GetStorageDefault(pProperty);
            ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
          }
          continue;
        }
        else
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            // The types are equal so nothing needs to be done. The current value will stay valid.
            // This should be the most common case.
            continue;
          }
          else
          {
            // The type is new or has changed but we have a valid value stored. Assume that the type of a property was changed
            // and try to convert the value.
            if (value.CanConvertTo(SpecVarType))
            {
              value = value.ConvertTo(SpecVarType);
            }
            else
            {
              value = WToolsReflectionUtils::GetStorageDefault(pProperty);
            }
            continue;
          }
        }
      }
      break;
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      {
        if (value.GetType() != WVariantType::VariantArray)
        {
          value = WVariantArray();
          continue;
        }
        WVariantArray values = value.Get<WVariantArray>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for WPropertyCategory::Member, but for each element instead.
        for (WUInt32 i = 0; i < values.GetCount(); i++)
        {
          WVariant& var = values[i];
          if (var.GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            WResult res(W_FAILURE);
            var = var.ConvertTo(SpecVarType, &res);
            if (res == W_FAILURE)
            {
              var = WReflectionUtils::GetDefaultValue(pProperty, i);
            }
          }
        }
        value = values;
      }
      break;
      case WPropertyCategory::Map:
      {
        if (value.GetType() != WVariantType::VariantDictionary)
        {
          value = WVariantDictionary();
          continue;
        }
        WVariantDictionary values = value.Get<WVariantDictionary>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for WPropertyCategory::Member, but for each element instead.
        for (auto it2 = values.GetIterator(); it2.IsValid(); ++it2)
        {
          if (it2.Value().GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            WResult res(W_FAILURE);
            it2.Value() = it2.Value().ConvertTo(SpecVarType, &res);
            if (res == W_FAILURE)
            {
              it2.Value() = WReflectionUtils::GetDefaultValue(pProperty, it2.Key());
            }
          }
        }
        value = values;
      }
      break;
      default:
        break;
    }
  }
}

void WReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(
  WUInt32 uiIndex, const WAbstractProperty* pProperty, WSet<const WDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  if (pProperty->GetCategory() != WPropertyCategory::Member)
    return;

  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    WDynamicArray<WVariant>& data = it.Key()->m_Data;
    W_ASSERT_DEV(data.GetCount() == uiIndex, "WReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(WToolsReflectionUtils::GetStorageDefault(pProperty));
    if (pProperty->GetFlags().IsSet(WPropertyFlags::Class) && !pProperty->GetFlags().IsSet(WPropertyFlags::Pointer))
    {
      ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
    }
  }
}

////////////////////////////////////////////////////////////////////////
// WReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void WReflectedTypeStorageManager::Startup()
{
  WPlugin::Events().AddEventHandler(WReflectedTypeStorageManager::PluginEventHandler);
  WPhantomRttiManager::s_Events.AddEventHandler(TypeEventHandler);
}

void WReflectedTypeStorageManager::Shutdown()
{
  WPhantomRttiManager::s_Events.RemoveEventHandler(TypeEventHandler);
  WPlugin::Events().RemoveEventHandler(WReflectedTypeStorageManager::PluginEventHandler);

  for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    ReflectedTypeStorageMapping* pMapping = it.Value();

    for (auto inst : pMapping->m_Instances)
    {
      WLog::Error("Type '{0}' survived shutdown!", inst->GetType()->GetTypeName());
    }

    W_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
    W_DEFAULT_DELETE(pMapping);
  }
  s_ReflectedTypeToStorageMapping.Clear();
}

const WReflectedTypeStorageManager::ReflectedTypeStorageMapping* WReflectedTypeStorageManager::AddStorageAccessor(
  WReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void WReflectedTypeStorageManager::RemoveStorageAccessor(WReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Remove(pInstance);
}

WReflectedTypeStorageManager::ReflectedTypeStorageMapping* WReflectedTypeStorageManager::GetTypeStorageMapping(const WRTTI* pType)
{
  W_ASSERT_DEV(pType != nullptr, "Nullptr is not a valid type!");
  auto it = s_ReflectedTypeToStorageMapping.Find(pType);
  if (it.IsValid())
    return it.Value();

  ReflectedTypeStorageMapping* pMapping = W_DEFAULT_NEW(ReflectedTypeStorageMapping);
  pMapping->AddProperties(pType);
  s_ReflectedTypeToStorageMapping[pType] = pMapping;
  return pMapping;
}

void WReflectedTypeStorageManager::TypeEventHandler(const WPhantomRttiManagerEvent& e)
{
  switch (e.m_Type)
  {
    case WPhantomRttiManagerEvent::Type::TypeAdded:
    {
      const WRTTI* pType = e.m_pChangedType;
      W_ASSERT_DEV(pType != nullptr, "A type was added but it has an invalid handle!");

      W_ASSERT_DEV(!s_ReflectedTypeToStorageMapping.Find(e.m_pChangedType).IsValid(), "The type '{0}' was added twice!", pType->GetTypeName());
      GetTypeStorageMapping(e.m_pChangedType);
    }
    break;
    case WPhantomRttiManagerEvent::Type::TypeChanged:
    {
      const WRTTI* pNewType = e.m_pChangedType;
      W_ASSERT_DEV(pNewType != nullptr, "A type was updated but its handle is invalid!");

      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      W_ASSERT_DEV(pMapping != nullptr, "A type was updated but no mapping exists for it!");

      if (pNewType->GetParentType() != nullptr && pNewType->GetParentType()->GetTypeName() == "WEnumBase")
      {
        // W_ASSERT_DEV(false, "Updating enums not implemented yet!");
        break;
      }
      else if (pNewType->GetParentType() != nullptr && pNewType->GetParentType()->GetTypeName() == "WBitflagsBase")
      {
        W_ASSERT_DEV(false, "Updating bitflags not implemented yet!");
      }

      pMapping->AddProperties(pNewType);

      WSet<WRTTI*> dependencies;
      // Update all types that either derive from the changed type or have the type as a member.
      for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key() == e.m_pChangedType)
          continue;

        const WRTTI* pType = it.Key();
        if (pType->IsDerivedFrom(e.m_pChangedType))
        {
          it.Value()->AddProperties(pType);
        }
      }
    }
    break;
    case WPhantomRttiManagerEvent::Type::TypeRemoved:
    {
      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      W_ASSERT_DEV(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
      W_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
      s_ReflectedTypeToStorageMapping.Remove(e.m_pChangedType);
      W_DEFAULT_DELETE(pMapping);
    }
    break;
  }
}

void WReflectedTypeStorageManager::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::BeforeUnloading:
    {
      for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid();)
      {
        if (it.Key()->GetPluginName() == EventData.m_sPluginBinary)
        {
          ReflectedTypeStorageMapping* pMapping = it.Value();
          W_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
          it = s_ReflectedTypeToStorageMapping.Remove(it);
          W_DEFAULT_DELETE(pMapping);
        }
        else
        {
          ++it;
        }
      }
    }
    break;

    default:
      break;
  }
}
