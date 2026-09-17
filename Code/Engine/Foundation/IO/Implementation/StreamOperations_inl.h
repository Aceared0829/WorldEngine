#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

// Standard operators for overloads of common data types

/// bool versions

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, bool bValue)
{
  WUInt8 uiValue = bValue ? 1 : 0;
  inout_stream.WriteBytes(&uiValue, sizeof(WUInt8)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, bool& out_bValue)
{
  WUInt8 uiValue = 0;
  W_VERIFY(inout_stream.ReadBytes(&uiValue, sizeof(WUInt8)) == sizeof(WUInt8), "End of stream reached.");
  out_bValue = (uiValue != 0);
  return inout_stream;
}

/// unsigned int versions

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WUInt8 uiValue)
{
  inout_stream.WriteBytes(&uiValue, sizeof(WUInt8)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WUInt8& out_uiValue)
{
  W_VERIFY(inout_stream.ReadBytes(&out_uiValue, sizeof(WUInt8)) == sizeof(WUInt8), "End of stream reached.");
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WUInt8* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WUInt8) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WUInt8* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WUInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WUInt16 uiValue)
{
  inout_stream.WriteWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WUInt16& ref_uiValue)
{
  inout_stream.ReadWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WUInt16* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WUInt16) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WUInt16* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WUInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WUInt32 uiValue)
{
  inout_stream.WriteDWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WUInt32& ref_uiValue)
{
  inout_stream.ReadDWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WUInt32* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WUInt32) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WUInt32* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WUInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WUInt64 uiValue)
{
  inout_stream.WriteQWordValue(&uiValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WUInt64& ref_uiValue)
{
  inout_stream.ReadQWordValue(&ref_uiValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WUInt64* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WUInt64) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WUInt64* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WUInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}

/// signed int versions

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WInt8 iValue)
{
  inout_stream.WriteBytes(reinterpret_cast<const WUInt8*>(&iValue), sizeof(WInt8)).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WInt8& ref_iValue)
{
  W_VERIFY(inout_stream.ReadBytes(reinterpret_cast<WUInt8*>(&ref_iValue), sizeof(WInt8)) == sizeof(WInt8), "End of stream reached.");
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WInt8* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WInt8) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WInt8* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WInt8) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WInt16 iValue)
{
  inout_stream.WriteWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WInt16& ref_iValue)
{
  inout_stream.ReadWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WInt16* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WInt16) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WInt16* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WInt16) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WInt32 iValue)
{
  inout_stream.WriteDWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WInt32& ref_iValue)
{
  inout_stream.ReadDWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WInt32* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WInt32) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WInt32* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WInt32) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, WInt64 iValue)
{
  inout_stream.WriteQWordValue(&iValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WInt64& ref_iValue)
{
  inout_stream.ReadQWordValue(&ref_iValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const WInt64* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(WInt64) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, WInt64* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(WInt64) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


/// float and double versions

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, float fValue)
{
  inout_stream.WriteDWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, float& ref_fValue)
{
  inout_stream.ReadDWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const float* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(float) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, float* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(float) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


inline WStreamWriter& operator<<(WStreamWriter& inout_stream, double fValue)
{
  inout_stream.WriteQWordValue(&fValue).AssertSuccess();
  return inout_stream;
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, double& ref_fValue)
{
  inout_stream.ReadQWordValue(&ref_fValue).AssertSuccess();
  return inout_stream;
}

inline WResult SerializeArray(WStreamWriter& inout_stream, const double* pArray, WUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(double) * uiCount);
}

inline WResult DeserializeArray(WStreamReader& inout_stream, double* pArray, WUInt64 uiCount)
{
  const WUInt64 uiNumBytes = sizeof(double) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return W_SUCCESS;

  return W_FAILURE;
}


// C-style strings
// No read equivalent for C-style strings (but can be read as WString & WStringBuilder instances)

W_FOUNDATION_DLL WStreamWriter& operator<<(WStreamWriter& inout_stream, const char* szValue);
W_FOUNDATION_DLL WStreamWriter& operator<<(WStreamWriter& inout_stream, WStringView sValue);

// WHybridString

template <WUInt16 Size, typename AllocatorWrapper>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WHybridString<Size, AllocatorWrapper>& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

template <WUInt16 Size, typename AllocatorWrapper>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WHybridString<Size, AllocatorWrapper>& out_sValue)
{
  WStringBuilder builder;
  inout_stream.ReadString(builder).AssertSuccess();
  out_sValue = std::move(builder);

  return inout_stream;
}

// WStringBuilder

W_FOUNDATION_DLL WStreamWriter& operator<<(WStreamWriter& inout_stream, const WStringBuilder& sValue);
W_FOUNDATION_DLL WStreamReader& operator>>(WStreamReader& inout_stream, WStringBuilder& out_sValue);

// WEnum

template <typename T>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WEnum<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WEnum<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}

// WBitflags

template <typename T>
inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WBitflags<T>& value)
{
  inout_stream << value.GetValue();

  return inout_stream;
}

template <typename T>
inline WStreamReader& operator>>(WStreamReader& inout_stream, WBitflags<T>& value)
{
  typename T::StorageType storedValue = T::Default;
  inout_stream >> storedValue;
  value.SetValue(storedValue);

  return inout_stream;
}
