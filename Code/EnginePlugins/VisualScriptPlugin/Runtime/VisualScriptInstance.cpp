#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

WVisualScriptInstance::WVisualScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld, const WSharedPtr<WVisualScriptDataStorage>& pConstantDataStorage, const WSharedPtr<const WVisualScriptDataDescription>& pInstanceDataDesc, const WSharedPtr<WVisualScriptInstanceDataMapping>& pInstanceDataMapping)
  : WScriptInstance(inout_owner, pWorld)
  , m_pConstantDataStorage(pConstantDataStorage)
  , m_pInstanceDataMapping(pInstanceDataMapping)
  , m_InstanceDataStorage(pInstanceDataDesc)
{
  if (pInstanceDataDesc != nullptr)
  {
    m_InstanceDataStorage.AllocateStorage(WScriptAllocator::GetAllocator());

    for (auto& it : m_pInstanceDataMapping->m_Content)
    {
      auto& instanceData = it.Value();
      m_InstanceDataStorage.SetDataFromVariant(instanceData.m_DataOffset, instanceData.m_DefaultValue, 0);
    }
  }
}

void WVisualScriptInstance::SetInstanceVariable(const WHashedString& sName, const WVariant& value)
{
  if (m_pInstanceDataMapping == nullptr)
    return;

  WVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return;

  WResult conversionStatus = W_FAILURE;
  WVariantType::Enum targetType = WVisualScriptDataType::GetVariantType(pInstanceData->m_DataOffset.GetType());

  WVariant convertedValue = value.ConvertTo(targetType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    WLog::Error("Can't apply instance variable '{}' because the given value of type '{}' can't be converted the expected target type '{}'", sName, value.GetType(), targetType);
    return;
  }

  m_InstanceDataStorage.SetDataFromVariant(pInstanceData->m_DataOffset, convertedValue, 0);
}

WVariant WVisualScriptInstance::GetInstanceVariable(const WHashedString& sName)
{
  if (m_pInstanceDataMapping == nullptr)
    return WVariant();

  WVisualScriptInstanceData* pInstanceData = nullptr;
  if (m_pInstanceDataMapping->m_Content.TryGetValue(sName, pInstanceData) == false)
    return WVariant();

  return m_InstanceDataStorage.GetDataAsVariant(pInstanceData->m_DataOffset, nullptr, 0);
}
