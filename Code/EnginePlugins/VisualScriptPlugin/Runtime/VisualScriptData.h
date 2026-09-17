#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Types/SharedPtr.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct W_VISUALSCRIPTPLUGIN_DLL WVisualScriptDataDescription : public WRefCounted
{
  struct DataOffset
  {
    W_DECLARE_POD_TYPE();

    struct W_VISUALSCRIPTPLUGIN_DLL Source
    {
      enum Enum
      {
        Local,
        Instance,
        Constant,

        Count
      };

      static const char* GetName(Enum source);
    };

    enum
    {
      BYTE_OFFSET_BITS = 24,
      TYPE_BITS = 6,
      SOURCE_BITS = 2,
      INVALID_BYTE_OFFSET = W_BIT(BYTE_OFFSET_BITS) - 1
    };

    W_ALWAYS_INLINE DataOffset()
    {
      m_uiByteOffset = INVALID_BYTE_OFFSET;
      m_uiType = WVisualScriptDataType::Invalid;
      m_uiSource = Source::Local;
    }

    W_ALWAYS_INLINE DataOffset(WUInt32 uiOffset, WVisualScriptDataType::Enum dataType, Source::Enum source)
    {
      m_uiByteOffset = uiOffset;
      m_uiType = dataType;
      m_uiSource = source;
    }

    W_ALWAYS_INLINE bool IsValid() const
    {
      return m_uiByteOffset != INVALID_BYTE_OFFSET &&
             m_uiType != WVisualScriptDataType::Invalid;
    }

    W_ALWAYS_INLINE WVisualScriptDataType::Enum GetType() const { return static_cast<WVisualScriptDataType::Enum>(m_uiType); }
    W_ALWAYS_INLINE Source::Enum GetSource() const { return static_cast<Source::Enum>(m_uiSource); }
    W_ALWAYS_INLINE bool IsLocal() const { return m_uiSource == Source::Local; }
    W_ALWAYS_INLINE bool IsInstance() const { return m_uiSource == Source::Instance; }
    W_ALWAYS_INLINE bool IsConstant() const { return m_uiSource == Source::Constant; }

    W_ALWAYS_INLINE WResult Serialize(WStreamWriter& inout_stream) const { return inout_stream.WriteDWordValue(this); }
    W_ALWAYS_INLINE WResult Deserialize(WStreamReader& inout_stream) { return inout_stream.ReadDWordValue(this); }

    WUInt32 m_uiByteOffset : BYTE_OFFSET_BITS;
    WUInt32 m_uiType : TYPE_BITS;
    WUInt32 m_uiSource : SOURCE_BITS;
  };

  struct OffsetAndCount
  {
    W_DECLARE_POD_TYPE();

    WUInt32 m_uiStartOffset = 0;
    WUInt32 m_uiCount = 0;
  };

  OffsetAndCount m_PerTypeInfo[WVisualScriptDataType::Count];
  WUInt32 m_uiStorageSizeNeeded = 0;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  void Clear();
  void CalculatePerTypeStartOffsets();
  void CheckOffset(DataOffset dataOffset, const WRTTI* pType) const;

  DataOffset GetOffset(WVisualScriptDataType::Enum dataType, WUInt32 uiIndex, DataOffset::Source::Enum source) const;
};

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptDataStorage : public WRefCounted
{
public:
  WVisualScriptDataStorage(const WSharedPtr<const WVisualScriptDataDescription>& pDesc);
  ~WVisualScriptDataStorage();

  const WVisualScriptDataDescription& GetDesc() const;

  bool IsAllocated() const;
  void AllocateStorage(WAllocator* pAllocator);
  void DeallocateStorage();

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream, WAllocator* pAllocator);

  using DataOffset = WVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  WTypedPointer GetPointerData(DataOffset dataOffset, WUInt32 uiExecutionCounter) const;

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const WRTTI* pType, WUInt32 uiExecutionCounter);

  WVariant GetDataAsVariant(DataOffset dataOffset, const WRTTI* pExpectedType, WUInt32 uiExecutionCounter) const;
  void SetDataFromVariant(DataOffset dataOffset, const WVariant& value, WUInt32 uiExecutionCounter);

private:
  WSharedPtr<const WVisualScriptDataDescription> m_pDesc;
  WByteArrayPtr m_Storage;
  WAllocator* m_pAllocator = nullptr;
};

struct WVisualScriptInstanceData
{
  WVisualScriptDataDescription::DataOffset m_DataOffset;
  WVariant m_DefaultValue;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

using WVisualScriptInstanceDataMapping = WRefCountedContainer<WHashTable<WHashedString, WVisualScriptInstanceData>>;

#include <VisualScriptPlugin/Runtime/VisualScriptData_inl.h>
