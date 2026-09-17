#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// WAllocator::Stats

void operator<<(WStreamWriter& inout_stream, const WAllocator::Stats& rhs)
{
  inout_stream << rhs.m_uiNumAllocations;
  inout_stream << rhs.m_uiNumDeallocations;
  inout_stream << rhs.m_uiAllocationSize;
}

void operator>>(WStreamReader& inout_stream, WAllocator::Stats& rhs)
{
  inout_stream >> rhs.m_uiNumAllocations;
  inout_stream >> rhs.m_uiNumDeallocations;
  inout_stream >> rhs.m_uiAllocationSize;
}

// WTime

void operator<<(WStreamWriter& inout_stream, WTime value)
{
  inout_stream << value.GetSeconds();
}

void operator>>(WStreamReader& inout_stream, WTime& ref_value)
{
  double d = 0;
  inout_stream.ReadQWordValue(&d).IgnoreResult();

  ref_value = WTime::MakeFromSeconds(d);
}

// WUuid

void operator<<(WStreamWriter& inout_stream, const WUuid& value)
{
  inout_stream << value.m_uiHigh;
  inout_stream << value.m_uiLow;
}

void operator>>(WStreamReader& inout_stream, WUuid& ref_value)
{
  inout_stream >> ref_value.m_uiHigh;
  inout_stream >> ref_value.m_uiLow;
}

// WHashedString

void operator<<(WStreamWriter& inout_stream, const WHashedString& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
}

void operator>>(WStreamReader& inout_stream, WHashedString& ref_sValue)
{
  WStringBuilder sTemp;
  inout_stream >> sTemp;
  ref_sValue.Assign(sTemp);
}

// WTempHashedString

void operator<<(WStreamWriter& inout_stream, const WTempHashedString& sValue)
{
  inout_stream << (WUInt64)sValue.GetHash();
}

void operator>>(WStreamReader& inout_stream, WTempHashedString& ref_sValue)
{
  WUInt64 hash;
  inout_stream >> hash;
  ref_sValue = WTempHashedString(hash);
}

// WVariant

struct WriteValueFunc
{
  template <typename T>
  W_ALWAYS_INLINE void operator()()
  {
    (*m_pStream) << m_pValue->Get<T>();
  }

  WStreamWriter* m_pStream;
  const WVariant* m_pValue;
};

template <>
W_FORCE_INLINE void WriteValueFunc::operator()<WVariantArray>()
{
  const WVariantArray& values = m_pValue->Get<WVariantArray>();
  const WUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (WUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) << values[i];
  }
}

template <>
W_FORCE_INLINE void WriteValueFunc::operator()<WVariantDictionary>()
{
  const WVariantDictionary& values = m_pValue->Get<WVariantDictionary>();
  const WUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    (*m_pStream) << it.Key();
    (*m_pStream) << it.Value();
  }
}

template <>
inline void WriteValueFunc::operator()<WTypedPointer>()
{
  W_REPORT_FAILURE("Type 'WReflectedClass*' not supported in serialization.");
}

template <>
inline void WriteValueFunc::operator()<WTypedObject>()
{
  WTypedObject obj = m_pValue->Get<WTypedObject>();
  if (const WVariantTypeInfo* pTypeInfo = WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
  {
    (*m_pStream) << obj.m_pType->GetTypeName();
    pTypeInfo->Serialize(*m_pStream, obj.m_pObject);
  }
  else
  {
    W_REPORT_FAILURE("The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
  }
}

template <>
W_FORCE_INLINE void WriteValueFunc::operator()<WStringView>()
{
  WStringBuilder s = m_pValue->Get<WStringView>();
  (*m_pStream) << s;
}

template <>
W_FORCE_INLINE void WriteValueFunc::operator()<WDataBuffer>()
{
  const WDataBuffer& data = m_pValue->Get<WDataBuffer>();
  const WUInt32 iCount = data.GetCount();
  (*m_pStream) << iCount;
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).AssertSuccess();
}

struct ReadValueFunc
{
  template <typename T>
  W_FORCE_INLINE void operator()()
  {
    T value;
    (*m_pStream) >> value;
    *m_pValue = value;
  }

  WStreamReader* m_pStream;
  WVariant* m_pValue;
};

template <>
W_FORCE_INLINE void ReadValueFunc::operator()<WVariantArray>()
{
  WVariantArray values;
  WUInt32 iCount;
  (*m_pStream) >> iCount;
  values.SetCount(iCount);
  for (WUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) >> values[i];
  }
  *m_pValue = values;
}

template <>
W_FORCE_INLINE void ReadValueFunc::operator()<WVariantDictionary>()
{
  WVariantDictionary values;
  WUInt32 iCount;
  (*m_pStream) >> iCount;
  for (WUInt32 i = 0; i < iCount; i++)
  {
    WString key;
    WVariant value;
    (*m_pStream) >> key;
    (*m_pStream) >> value;
    values.Insert(key, value);
  }
  *m_pValue = values;
}

template <>
inline void ReadValueFunc::operator()<WTypedPointer>()
{
  W_REPORT_FAILURE("Type 'WTypedPointer' not supported in serialization.");
}

template <>
inline void ReadValueFunc::operator()<WTypedObject>()
{
  WStringBuilder sType;
  (*m_pStream) >> sType;
  const WRTTI* pType = WRTTI::FindTypeByName(sType);
  W_ASSERT_DEV(pType, "The type '{0}' could not be found.", sType);
  const WVariantTypeInfo* pTypeInfo = WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  W_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", sType);
  W_MSVC_ANALYSIS_ASSUME(pType != nullptr);
  W_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
  void* pObject = pType->GetAllocator()->Allocate<void>();
  pTypeInfo->Deserialize(*m_pStream, pObject);
  m_pValue->MoveTypedObject(pObject, pType);
}

template <>
inline void ReadValueFunc::operator()<WStringView>()
{
  W_REPORT_FAILURE("Type 'WStringView' not supported in serialization.");
}

template <>
W_FORCE_INLINE void ReadValueFunc::operator()<WDataBuffer>()
{
  WDataBuffer data;
  WUInt32 iCount;
  (*m_pStream) >> iCount;
  data.SetCountUninitialized(iCount);

  m_pStream->ReadBytes(data.GetData(), iCount);
  *m_pValue = data;
}

void operator<<(WStreamWriter& inout_stream, const WVariant& value)
{
  WUInt8 variantVersion = (WUInt8)WGetStaticRTTI<WVariant>()->GetTypeVersion();
  inout_stream << variantVersion;
  WVariant::Type::Enum type = value.GetType();
  WUInt8 typeStorage = type;
  if (typeStorage == WVariantType::StringView)
    typeStorage = WVariantType::String;
  inout_stream << typeStorage;

  if (type != WVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &value;

    WVariant::DispatchTo(func, type);
  }
}

void operator>>(WStreamReader& inout_stream, WVariant& ref_value)
{
  WUInt8 variantVersion;
  inout_stream >> variantVersion;
  W_ASSERT_DEBUG(WGetStaticRTTI<WVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  WUInt8 typeStorage;
  inout_stream >> typeStorage;
  WVariant::Type::Enum type = (WVariant::Type::Enum)typeStorage;

  if (type != WVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &ref_value;

    WVariant::DispatchTo(func, type);
  }
  else
  {
    ref_value = WVariant();
  }
}

// WTimestamp

void operator<<(WStreamWriter& inout_stream, WTimestamp value)
{
  inout_stream << value.GetInt64(WSIUnitOfTime::Microsecond);
}

void operator>>(WStreamReader& inout_stream, WTimestamp& ref_value)
{
  WInt64 value;
  inout_stream >> value;

  ref_value = WTimestamp::MakeFromInt(value, WSIUnitOfTime::Microsecond);
}

// WVarianceTypeFloat

void operator<<(WStreamWriter& inout_stream, const WVarianceTypeFloat& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(WStreamReader& inout_stream, WVarianceTypeFloat& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// WVarianceTypeTime

void operator<<(WStreamWriter& inout_stream, const WVarianceTypeTime& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(WStreamReader& inout_stream, WVarianceTypeTime& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// WVarianceTypeAngle

void operator<<(WStreamWriter& inout_stream, const WVarianceTypeAngle& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(WStreamReader& inout_stream, WVarianceTypeAngle& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}
