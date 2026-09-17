#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

// Ensure that we can retrieve the base data type with this simple bit operation
static_assert(((int)WProcessingStream::DataType::Half3 & ~3) == (int)WProcessingStream::DataType::Half);
static_assert(((int)WProcessingStream::DataType::Float4 & ~3) == (int)WProcessingStream::DataType::Float);
static_assert(((int)WProcessingStream::DataType::Byte2 & ~3) == (int)WProcessingStream::DataType::Byte);
static_assert(((int)WProcessingStream::DataType::Short3 & ~3) == (int)WProcessingStream::DataType::Short);
static_assert(((int)WProcessingStream::DataType::Int4 & ~3) == (int)WProcessingStream::DataType::Int);

#if W_ENABLED(W_PLATFORM_64BIT)
static_assert(sizeof(WProcessingStream) == 32);
#endif

WProcessingStream::WProcessingStream() = default;

WProcessingStream::WProcessingStream(const WHashedString& sName, DataType type, WUInt16 uiStride, WUInt16 uiAlignment)
  : m_uiAlignment(uiAlignment)
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(uiStride)
  , m_Type(type)
  , m_sName(sName)
{
}

WProcessingStream::WProcessingStream(const WHashedString& sName, WArrayPtr<WUInt8> data, DataType type, WUInt16 uiStride)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(uiStride)
  , m_Type(type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

WProcessingStream::WProcessingStream(const WHashedString& sName, WArrayPtr<WUInt8> data, DataType type)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(type))
  , m_uiStride(m_uiTypeSize)
  , m_Type(type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

WProcessingStream::WProcessingStream(const WHashedString& sName, const WProcessingStream& data)
  : m_pData(data.m_pData)
  , m_uiDataSize(data.m_uiDataSize)
  , m_uiAlignment(data.m_uiAlignment)
  , m_uiTypeSize(data.m_uiTypeSize)
  , m_uiStride(data.m_uiStride)
  , m_Type(data.m_Type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

WProcessingStream::~WProcessingStream()
{
  FreeData();
}

void WProcessingStream::SetSize(WUInt64 uiNumElements)
{
  WUInt64 uiNewDataSize = uiNumElements * m_uiTypeSize;
  if (m_uiDataSize == uiNewDataSize)
    return;

  FreeData();

  if (uiNewDataSize == 0)
  {
    return;
  }

  /// \todo Allow to reuse memory from a pool ?
  if (m_uiAlignment > 0)
  {
    m_pData = WFoundation::GetAlignedAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), static_cast<size_t>(m_uiAlignment));
  }
  else
  {
    m_pData = WFoundation::GetDefaultAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), 0);
  }

  W_ASSERT_DEV(m_pData != nullptr, "Allocating {0} elements of {1} bytes each, with {2} bytes alignment, failed", uiNumElements, ((WUInt32)GetDataTypeSize(m_Type)), m_uiAlignment);
  m_uiDataSize = uiNewDataSize;
}

void WProcessingStream::FreeData()
{
  if (m_pData != nullptr && m_bExternalMemory == false)
  {
    if (m_uiAlignment > 0)
    {
      WFoundation::GetAlignedAllocator()->Deallocate(m_pData);
    }
    else
    {
      WFoundation::GetDefaultAllocator()->Deallocate(m_pData);
    }
  }

  m_pData = nullptr;
  m_uiDataSize = 0;
}

static WUInt16 s_TypeSize[] = {
  2,  // Half,
  4,  // Half2,
  6,  // Half3,
  8,  // Half4,

  4,  // Float,
  8,  // Float2,
  12, // Float3,
  16, // Float4,

  1,  // Byte,
  2,  // Byte2,
  3,  // Byte3,
  4,  // Byte4,

  2,  // Short,
  4,  // Short2,
  6,  // Short3,
  8,  // Short4,

  4,  // Int,
  8,  // Int2,
  12, // Int3,
  16, // Int4,
};
static_assert(W_ARRAY_SIZE(s_TypeSize) == (size_t)WProcessingStream::DataType::Count);

// static
WUInt16 WProcessingStream::GetDataTypeSize(DataType type)
{
  return s_TypeSize[(WUInt32)type];
}

static WStringView s_TypeName[] = {
  "Half"_wsv,   // Half,
  "Half2"_wsv,  // Half2,
  "Half3"_wsv,  // Half3,
  "Half4"_wsv,  // Half4,

  "Float"_wsv,  // Float,
  "Float2"_wsv, // Float2,
  "Float3"_wsv, // Float3,
  "Float4"_wsv, // Float4,

  "Byte"_wsv,   // Byte,
  "Byte2"_wsv,  // Byte2,
  "Byte3"_wsv,  // Byte3,
  "Byte4"_wsv,  // Byte4,

  "Short"_wsv,  // Short,
  "Short2"_wsv, // Short2,
  "Short3"_wsv, // Short3,
  "Short4"_wsv, // Short4,

  "Int"_wsv,    // Int,
  "Int2"_wsv,   // Int2,
  "Int3"_wsv,   // Int3,
  "Int4"_wsv,   // Int4,
};
static_assert(W_ARRAY_SIZE(s_TypeName) == (size_t)WProcessingStream::DataType::Count);

// static
WStringView WProcessingStream::GetDataTypeName(DataType type)
{
  return s_TypeName[(WUInt32)type];
}
