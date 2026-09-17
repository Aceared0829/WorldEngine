#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

/// A single stream in a stream group holding contiguous data of a given type.
class W_FOUNDATION_DLL WProcessingStream
{
public:
  /// The data types which can be stored in the stream.
  /// When adding new data types the GetDataTypeSize() of WProcessingStream needs to be updated.
  enum class DataType : WUInt8
  {
    Half,   // WFloat16
    Half2,  // 2x WFloat16
    Half3,  // 3x WFloat16
    Half4,  // 4x WFloat16

    Float,  // float
    Float2, // 2x float, e.g. WVec2
    Float3, // 3x float, e.g. WVec3
    Float4, // 4x float, e.g. WVec4

    Byte,
    Byte2,
    Byte3,
    Byte4,

    Short,
    Short2,
    Short3,
    Short4,

    Int,
    Int2,
    Int3,
    Int4,

    Count
  };

  WProcessingStream();
  WProcessingStream(const WHashedString& sName, const WProcessingStream& data);
  WProcessingStream(const WHashedString& sName, DataType type, WUInt16 uiStride, WUInt16 uiAlignment);
  WProcessingStream(const WHashedString& sName, WArrayPtr<WUInt8> data, DataType type, WUInt16 uiStride);
  WProcessingStream(const WHashedString& sName, WArrayPtr<WUInt8> data, DataType type);
  ~WProcessingStream();

  /// Returns a const pointer to the data cast to the type T, note that no type check is done!
  template <typename T>
  const T* GetData() const
  {
    return static_cast<const T*>(GetData());
  }

  /// Returns a const pointer to the start of the data block.
  const void* GetData() const { return m_pData; }

  /// Returns a non-const pointer to the data cast to the type T, note that no type check is done!
  template <typename T>
  T* GetWritableData() const
  {
    return static_cast<T*>(GetWritableData());
  }

  /// Returns a non-const pointer to the start of the data block.
  void* GetWritableData() const { return m_pData; }

  WUInt64 GetDataSize() const { return m_uiDataSize; }

  /// Returns the name of the stream
  const WHashedString& GetName() const { return m_sName; }

  /// Returns the alignment which was used to allocate the stream.
  WUInt16 GetAlignment() const { return m_uiAlignment; }

  /// Returns the data type of the stream.
  DataType GetDataType() const { return m_Type; }

  /// Returns the size of one stream element in bytes.
  WUInt16 GetElementSize() const { return m_uiTypeSize; }

  /// Returns the stride between two elements of the stream in bytes.
  WUInt16 GetElementStride() const { return m_uiStride; }

  static WUInt16 GetDataTypeSize(DataType type);
  static WStringView GetDataTypeName(DataType type);

protected:
  friend class WProcessingStreamGroup;

  void SetSize(WUInt64 uiNumElements);
  void FreeData();

  void* m_pData = nullptr;
  WUInt64 m_uiDataSize = 0; // in bytes

  WUInt16 m_uiAlignment = 0;
  WUInt16 m_uiTypeSize = 0;
  WUInt16 m_uiStride = 0;
  DataType m_Type;
  bool m_bExternalMemory = false;

  WHashedString m_sName;
};
