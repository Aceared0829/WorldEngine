#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

namespace
{
  static const char* s_DataOffsetSourceNames[] = {
    "Local",
    "Instance",
    "Constant",
  };
  static_assert(W_ARRAY_SIZE(s_DataOffsetSourceNames) == (size_t)WVisualScriptDataDescription::DataOffset::Source::Count);
} // namespace

// Check that DataOffset fits in one uint32 and also check that we have enough bits for dataType and source.
static_assert(sizeof(WVisualScriptDataDescription::DataOffset) == sizeof(WUInt32));
static_assert(WVisualScriptDataType::Count <= W_BIT(WVisualScriptDataDescription::DataOffset::TYPE_BITS));
static_assert(WVisualScriptDataDescription::DataOffset::Source::Count <= W_BIT(WVisualScriptDataDescription::DataOffset::SOURCE_BITS));

// static
const char* WVisualScriptDataDescription::DataOffset::Source::GetName(Enum source)
{
  W_ASSERT_DEBUG(source >= 0 && static_cast<WUInt32>(source) < W_ARRAY_SIZE(s_DataOffsetSourceNames), "Out of bounds access");
  return s_DataOffsetSourceNames[source];
}

//////////////////////////////////////////////////////////////////////////

static const WTypeVersion s_uiVisualScriptDataDescriptionVersion = 2;

WResult WVisualScriptDataDescription::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiCount;
  }

  return W_SUCCESS;
}

WResult WVisualScriptDataDescription::Deserialize(WStreamReader& inout_stream)
{
  WTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);
  if (uiVersion < 2)
  {
    WLog::Error("Invalid visual script data desc version. Expected >= 2 but got {}. Visual Script needs re-export", uiVersion);
    return W_FAILURE;
  }

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiCount;
  }

  CalculatePerTypeStartOffsets();

  return W_SUCCESS;
}

void WVisualScriptDataDescription::Clear()
{
  WMemoryUtils::ZeroFillArray(m_PerTypeInfo);
  m_uiStorageSizeNeeded = 0;
}

void WVisualScriptDataDescription::CalculatePerTypeStartOffsets()
{
  WUInt32 uiOffset = 0;
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(m_PerTypeInfo); ++i)
  {
    auto dataType = static_cast<WVisualScriptDataType::Enum>(i);
    auto& typeInfo = m_PerTypeInfo[i];

    if (typeInfo.m_uiCount > 0)
    {
      uiOffset = WMemoryUtils::AlignSize(uiOffset, WVisualScriptDataType::GetStorageAlignment(dataType));
      typeInfo.m_uiStartOffset = uiOffset;

      uiOffset += WVisualScriptDataType::GetStorageSize(dataType) * typeInfo.m_uiCount;
    }
  }

  m_uiStorageSizeNeeded = uiOffset;
}

//////////////////////////////////////////////////////////////////////////

WVisualScriptDataStorage::WVisualScriptDataStorage(const WSharedPtr<const WVisualScriptDataDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

WVisualScriptDataStorage::~WVisualScriptDataStorage()
{
  DeallocateStorage();
}

void WVisualScriptDataStorage::AllocateStorage(WAllocator* pAllocator)
{
  W_ASSERT_DEV(IsAllocated() == false, "Storage already allocated");

  m_Storage = W_NEW_ARRAY(pAllocator, WUInt8, m_pDesc->m_uiStorageSizeNeeded);
  WMemoryUtils::ZeroFill(m_Storage.GetPtr(), m_Storage.GetCount());
  m_pAllocator = pAllocator;

  auto pData = m_Storage.GetPtr();

  for (WUInt32 scriptDataType = 0; scriptDataType < WVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == WVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<WString*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    if (scriptDataType == WVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<WHashedString*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<WVariant*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Construct<SkipTrivialTypes>(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<WVariantArray*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Construct<SkipTrivialTypes>(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<WVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Construct<SkipTrivialTypes>(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void WVisualScriptDataStorage::DeallocateStorage()
{
  if (IsAllocated() == false)
    return;

  auto pData = m_Storage.GetPtr();

  for (WUInt32 scriptDataType = 0; scriptDataType < WVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == WVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<WString*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<WHashedString*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<WVariant*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Destruct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<WVariantArray*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Destruct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == WVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<WVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      WMemoryUtils::Destruct(pVariantMaps, typeInfo.m_uiCount);
    }
  }

  W_DELETE_ARRAY(m_pAllocator, m_Storage);
  m_pAllocator = nullptr;
}

WResult WVisualScriptDataStorage::Serialize(WStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetPtr();

  for (WUInt32 scriptDataType = 0; scriptDataType < WVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == WVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<const WString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<const WHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<const WVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream << *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<const WVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        W_SUCCEED_OR_RETURN(inout_stream.WriteArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<const WVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        W_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::GameObject ||
             scriptDataType == WVisualScriptDataType::Component ||
             scriptDataType == WVisualScriptDataType::TypedPointer ||
             scriptDataType == WVisualScriptDataType::Coroutine)
    {
      WLog::Error("Cannot serialize visual script data type '{}'", WVisualScriptDataType::GetName(static_cast<WVisualScriptDataType::Enum>(scriptDataType)));
      return W_FAILURE;
    }
    else
    {
      const WUInt32 uiBytesToWrite = typeInfo.m_uiCount * WVisualScriptDataType::GetStorageSize(static_cast<WVisualScriptDataType::Enum>(scriptDataType));
      W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return W_SUCCESS;
}

WResult WVisualScriptDataStorage::Deserialize(WStreamReader& inout_stream, WAllocator* pAllocator)
{
  if (IsAllocated() == false)
  {
    AllocateStorage(pAllocator);
  }

  auto pData = m_Storage.GetPtr();

  for (WUInt32 scriptDataType = 0; scriptDataType < WVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == WVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<WString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<WHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<WVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream >> *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<WVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        W_SUCCEED_OR_RETURN(inout_stream.ReadArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == WVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<WVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        W_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const WUInt32 uiBytesToRead = typeInfo.m_uiCount * WVisualScriptDataType::GetStorageSize(static_cast<WVisualScriptDataType::Enum>(scriptDataType));
      inout_stream.ReadBytes(pData + typeInfo.m_uiStartOffset, uiBytesToRead);
    }
  }

  return W_SUCCESS;
}

WTypedPointer WVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, WUInt32 uiExecutionCounter) const
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiType == WVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const WVisualScriptGameObjectHandle*>(pData);
    return WTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), WGetStaticRTTI<WGameObject>());
  }
  else if (dataOffset.m_uiType == WVisualScriptDataType::Component)
  {
    auto& componentHandle = *reinterpret_cast<const WVisualScriptComponentHandle*>(pData);
    WComponent* pComponent = componentHandle.GetPtr(uiExecutionCounter);
    return WTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiType == WVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const WTypedPointer*>(pData);
  }

  WTypedPointer t;
  t.m_pObject = const_cast<WUInt8*>(pData);
  t.m_pType = WVisualScriptDataType::GetRtti(static_cast<WVisualScriptDataType::Enum>(dataOffset.m_uiType));
  return t;
}

WVariant WVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, const WRTTI* pExpectedType, WUInt32 uiExecutionCounter) const
{
  auto scriptDataType = dataOffset.GetType();

  // pExpectedType == nullptr means that the caller expects an WVariant so we decide solely based on the scriptDataType.
  if (pExpectedType == nullptr || pExpectedType == WGetStaticRTTI<WVariant>())
  {
    pExpectedType = WVisualScriptDataType::GetRtti(scriptDataType);
  }

  switch (scriptDataType)
  {
    case WVisualScriptDataType::Invalid:
      return WVariant();

    case WVisualScriptDataType::Bool:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<bool>(), "");
      return GetData<bool>(dataOffset);

    case WVisualScriptDataType::Byte:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WUInt8>(), "");
      return GetData<WUInt8>(dataOffset);

    case WVisualScriptDataType::Int:
      if (pExpectedType == WGetStaticRTTI<WInt32>())
      {
        return GetData<WInt32>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WInt16>())
      {
        return static_cast<WInt16>(GetData<WInt32>(dataOffset));
      }
      else if (pExpectedType == WGetStaticRTTI<WUInt16>())
      {
        return static_cast<WUInt16>(GetData<WInt32>(dataOffset));
      }
      else if (pExpectedType == WGetStaticRTTI<WUInt32>())
      {
        return static_cast<WUInt32>(GetData<WInt32>(dataOffset));
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::Int64:
      W_ASSERT_DEBUG(pExpectedType->GetTypeFlags().IsSet(WTypeFlags::IsEnum) || pExpectedType->GetTypeFlags().IsSet(WTypeFlags::Bitflags) || pExpectedType == WGetStaticRTTI<WInt64>(), "");
      return GetData<WInt64>(dataOffset);

    case WVisualScriptDataType::Float:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<float>(), "");
      return GetData<float>(dataOffset);

    case WVisualScriptDataType::Double:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<double>(), "");
      return GetData<double>(dataOffset);

    case WVisualScriptDataType::Color:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WColor>(), "");
      return GetData<WColor>(dataOffset);

    case WVisualScriptDataType::Vector2:
      if (pExpectedType == WGetStaticRTTI<WVec2>())
      {
        return GetData<WVec2>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WVec2I32>())
      {
        auto& v = GetData<WVec2>(dataOffset);
        return WVec2I32(int(v.x), int(v.y));
      }
      else if (pExpectedType == WGetStaticRTTI<WVec2U32>())
      {
        auto& v = GetData<WVec2>(dataOffset);
        return WVec2U32(WUInt32(v.x), WUInt32(v.y));
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::Vector3:
      if (pExpectedType == WGetStaticRTTI<WVec3>())
      {
        return GetData<WVec3>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WVec3I32>())
      {
        auto& v = GetData<WVec3>(dataOffset);
        return WVec3I32(int(v.x), int(v.y), int(v.z));
      }
      else if (pExpectedType == WGetStaticRTTI<WVec3U32>())
      {
        auto& v = GetData<WVec3>(dataOffset);
        return WVec3U32(WUInt32(v.x), WUInt32(v.y), WUInt32(v.z));
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::Vector4:
      if (pExpectedType == WGetStaticRTTI<WVec4>())
      {
        return GetData<WVec4>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WVec4I32>())
      {
        auto& v = GetData<WVec4>(dataOffset);
        return WVec4I32(int(v.x), int(v.y), int(v.z), int(v.w));
      }
      else if (pExpectedType == WGetStaticRTTI<WVec4U32>())
      {
        auto& v = GetData<WVec4>(dataOffset);
        return WVec4U32(WUInt32(v.x), WUInt32(v.y), WUInt32(v.z), WUInt32(v.w));
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::Quaternion:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WQuat>(), "");
      return GetData<WQuat>(dataOffset);

    case WVisualScriptDataType::Transform:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WTransform>(), "");
      return GetData<WTransform>(dataOffset);

    case WVisualScriptDataType::Time:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WTime>(), "");
      return GetData<WTime>(dataOffset);

    case WVisualScriptDataType::Angle:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WAngle>(), "");
      return GetData<WAngle>(dataOffset);

    case WVisualScriptDataType::String:
      if (pExpectedType == WGetStaticRTTI<WString>() || pExpectedType == WGetStaticRTTI<const char*>())
      {
        return GetData<WString>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WStringView>())
      {
        return WVariant(GetData<WString>(dataOffset).GetView(), false);
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::HashedString:
      if (pExpectedType == WGetStaticRTTI<WHashedString>())
      {
        return GetData<WHashedString>(dataOffset);
      }
      else if (pExpectedType == WGetStaticRTTI<WTempHashedString>())
      {
        return WTempHashedString(GetData<WHashedString>(dataOffset));
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::GameObject:
      if (pExpectedType == WGetStaticRTTI<WGameObject>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == WGetStaticRTTI<WGameObjectHandle>())
      {
        return GetData<WGameObjectHandle>(dataOffset);
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::Component:
      if (pExpectedType->IsDerivedFrom<WComponent>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == WGetStaticRTTI<WComponentHandle>())
      {
        return GetData<WComponentHandle>(dataOffset);
      }
      W_ASSERT_NOT_IMPLEMENTED;
      break;

    case WVisualScriptDataType::TypedPointer:
      return GetPointerData(dataOffset, uiExecutionCounter);

    case WVisualScriptDataType::Variant:
      return GetData<WVariant>(dataOffset);

    case WVisualScriptDataType::Array:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WVariantArray>(), "");
      return GetData<WVariantArray>(dataOffset);

    case WVisualScriptDataType::Map:
      W_ASSERT_DEBUG(pExpectedType == WGetStaticRTTI<WVariantDictionary>(), "");
      return GetData<WVariantDictionary>(dataOffset);

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return WVariant();
}

void WVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const WVariant& value, WUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = dataOffset.GetType();
  switch (scriptDataType)
  {
    case WVisualScriptDataType::Bool:
      SetData(dataOffset, value.Get<bool>());
      break;
    case WVisualScriptDataType::Byte:
      if (value.IsA<WInt8>())
      {
        SetData(dataOffset, WUInt8(value.Get<WInt8>()));
      }
      else
      {
        SetData(dataOffset, value.Get<WUInt8>());
      }
      break;
    case WVisualScriptDataType::Int:
      if (value.IsA<WInt16>())
      {
        SetData(dataOffset, WInt32(value.Get<WInt16>()));
      }
      else if (value.IsA<WUInt16>())
      {
        SetData(dataOffset, WInt32(value.Get<WUInt16>()));
      }
      else if (value.IsA<WInt32>())
      {
        SetData(dataOffset, value.Get<WInt32>());
      }
      else
      {
        SetData(dataOffset, WInt32(value.Get<WUInt32>()));
      }
      break;
    case WVisualScriptDataType::Int64:
      if (value.IsA<WInt64>())
      {
        SetData(dataOffset, value.Get<WInt64>());
      }
      else
      {
        SetData(dataOffset, WInt64(value.Get<WUInt64>()));
      }
      break;
    case WVisualScriptDataType::Float:
      SetData(dataOffset, value.Get<float>());
      break;
    case WVisualScriptDataType::Double:
      SetData(dataOffset, value.Get<double>());
      break;
    case WVisualScriptDataType::Color:
      SetData(dataOffset, value.Get<WColor>());
      break;
    case WVisualScriptDataType::Vector2:
      if (value.IsA<WVec2I32>())
      {
        auto& v = value.Get<WVec2I32>();
        SetData(dataOffset, WVec2(float(v.x), float(v.y)));
      }
      else if (value.IsA<WVec2U32>())
      {
        auto& v = value.Get<WVec2U32>();
        SetData(dataOffset, WVec2(float(v.x), float(v.y)));
      }
      else
      {
        SetData(dataOffset, value.Get<WVec2>());
      }
      break;
    case WVisualScriptDataType::Vector3:
      if (value.IsA<WVec3I32>())
      {
        auto& v = value.Get<WVec3I32>();
        SetData(dataOffset, WVec3(float(v.x), float(v.y), float(v.z)));
      }
      else if (value.IsA<WVec3U32>())
      {
        auto& v = value.Get<WVec3U32>();
        SetData(dataOffset, WVec3(float(v.x), float(v.y), float(v.z)));
      }
      else
      {
        SetData(dataOffset, value.Get<WVec3>());
      }
      break;
    case WVisualScriptDataType::Vector4:
      if (value.IsA<WVec4I32>())
      {
        auto& v = value.Get<WVec4I32>();
        SetData(dataOffset, WVec4(float(v.x), float(v.y), float(v.z), float(v.w)));
      }
      else if (value.IsA<WVec4U32>())
      {
        auto& v = value.Get<WVec4U32>();
        SetData(dataOffset, WVec4(float(v.x), float(v.y), float(v.z), float(v.w)));
      }
      else
      {
        SetData(dataOffset, value.Get<WVec4>());
      }
      break;
    case WVisualScriptDataType::Quaternion:
      SetData(dataOffset, value.Get<WQuat>());
      break;
    case WVisualScriptDataType::Transform:
      SetData(dataOffset, value.Get<WTransform>());
      break;
    case WVisualScriptDataType::Time:
      SetData(dataOffset, value.Get<WTime>());
      break;
    case WVisualScriptDataType::Angle:
      SetData(dataOffset, value.Get<WAngle>());
      break;
    case WVisualScriptDataType::String:
      if (value.IsA<WStringView>())
      {
        SetData(dataOffset, WString(value.Get<WStringView>()));
      }
      else
      {
        SetData(dataOffset, value.Get<WString>());
      }
      break;
    case WVisualScriptDataType::HashedString:
      if (value.IsA<WTempHashedString>())
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        SetData(dataOffset, value.Get<WHashedString>());
      }
      break;
    case WVisualScriptDataType::GameObject:
      if (value.IsA<WGameObjectHandle>())
      {
        SetData(dataOffset, value.Get<WGameObjectHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<WGameObject*>(), WGetStaticRTTI<WGameObject>(), uiExecutionCounter);
      }
      break;
    case WVisualScriptDataType::Component:
      if (value.IsA<WComponentHandle>())
      {
        SetData(dataOffset, value.Get<WComponentHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<WComponent*>(), WGetStaticRTTI<WComponent>(), uiExecutionCounter);
      }
      break;
    case WVisualScriptDataType::TypedPointer:
    {
      WTypedPointer typedPtr = value.Get<WTypedPointer>();
      SetPointerData(dataOffset, typedPtr.m_pObject, typedPtr.m_pType, uiExecutionCounter);
    }
    break;
    case WVisualScriptDataType::Variant:
      SetData(dataOffset, value);
      break;
    case WVisualScriptDataType::Array:
      SetData(dataOffset, value.Get<WVariantArray>());
      break;
    case WVisualScriptDataType::Map:
      SetData(dataOffset, value.Get<WVariantDictionary>());
      break;
    case WVisualScriptDataType::Coroutine:
      SetData(dataOffset, value.Get<WScriptCoroutineHandle>());
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
