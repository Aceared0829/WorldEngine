#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>

WOpenDdlReader::WOpenDdlReader()
{
  m_pCurrentChunk = nullptr;
  m_uiBytesInChunkLeft = 0;
}

WOpenDdlReader::~WOpenDdlReader()
{
  ClearDataChunks();
}

WResult WOpenDdlReader::ParseDocument(WStreamReader& inout_stream, WUInt32 uiFirstLineOffset, WLogInterface* pLog, WUInt32 uiCacheSizeInKB)
{
  W_ASSERT_DEBUG(m_ObjectStack.IsEmpty(), "A reader can only be used once.");

  SetLogInterface(pLog);
  SetCacheSize(uiCacheSizeInKB);
  SetInputStream(inout_stream, uiFirstLineOffset);

  m_TempCache.Reserve(s_uiChunkSize);

  WOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = WOpenDdlPrimitiveType::Custom;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_sCustomType = CopyString("root");
  pElement->m_sName = nullptr;
  pElement->m_uiNumChildElements = 0;

  m_ObjectStack.PushBack(pElement);

  return ParseAll();
}

const WOpenDdlReaderElement* WOpenDdlReader::GetRootElement() const
{
  W_ASSERT_DEBUG(!m_ObjectStack.IsEmpty(), "The reader has not parsed any document yet or an error occurred during parsing.");

  return m_ObjectStack[0];
}


const WOpenDdlReaderElement* WOpenDdlReader::FindElement(WStringView sGlobalName) const
{
  return m_GlobalNames.GetValueOrDefault(sGlobalName, nullptr);
}

WStringView WOpenDdlReader::CopyString(const WStringView& string)
{
  if (string.IsEmpty())
    return {};

  // no idea how to make this more efficient without running into lots of other problems
  m_Strings.PushBack(string);
  return m_Strings.PeekBack();
}

WOpenDdlReaderElement* WOpenDdlReader::CreateElement(WOpenDdlPrimitiveType type, WStringView sType, WStringView sName, bool bGlobalName)
{
  WOpenDdlReaderElement* pElement = &m_Elements.ExpandAndGetRef();
  pElement->m_pFirstChild = nullptr;
  pElement->m_pLastChild = nullptr;
  pElement->m_PrimitiveType = type;
  pElement->m_pSiblingElement = nullptr;
  pElement->m_sCustomType = sType;
  pElement->m_sName = CopyString(sName);
  pElement->m_uiNumChildElements = 0;

  if (bGlobalName)
  {
    pElement->m_uiNumChildElements = W_BIT(31);
  }

  if (bGlobalName && !sName.IsEmpty())
  {
    m_GlobalNames[sName] = pElement;
  }

  WOpenDdlReaderElement* pParent = m_ObjectStack.PeekBack();
  pParent->m_uiNumChildElements++;

  if (pParent->m_pFirstChild == nullptr)
  {
    pParent->m_pFirstChild = pElement;
    pParent->m_pLastChild = pElement;
  }
  else
  {
    ((WOpenDdlReaderElement*)pParent->m_pLastChild)->m_pSiblingElement = pElement;
    pParent->m_pLastChild = pElement;
  }

  m_ObjectStack.PushBack(pElement);

  return pElement;
}


void WOpenDdlReader::OnBeginObject(WStringView sType, WStringView sName, bool bGlobalName)
{
  CreateElement(WOpenDdlPrimitiveType::Custom, CopyString(sType), sName, bGlobalName);
}

void WOpenDdlReader::OnEndObject()
{
  m_ObjectStack.PopBack();
}

void WOpenDdlReader::OnBeginPrimitiveList(WOpenDdlPrimitiveType type, WStringView sName, bool bGlobalName)
{
  CreateElement(type, nullptr, sName, bGlobalName);

  m_TempCache.Clear();
}

void WOpenDdlReader::OnEndPrimitiveList()
{
  // if we had to temporarily store the primitive data, copy it into a new destination
  if (!m_TempCache.IsEmpty())
  {
    WUInt8* pTarget = AllocateBytes(m_TempCache.GetCount());
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;

    WMemoryUtils::Copy(pTarget, m_TempCache.GetData(), m_TempCache.GetCount());
  }

  m_ObjectStack.PopBack();
}

void WOpenDdlReader::StorePrimitiveData(bool bThisIsAll, WUInt32 bytecount, const WUInt8* pData)
{
  WUInt8* pTarget = nullptr;

  if (!bThisIsAll || !m_TempCache.IsEmpty())
  {
    // if this is not all, accumulate the data in a temp buffer
    WUInt32 offset = m_TempCache.GetCount();
    m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + bytecount);
    pTarget = &m_TempCache[offset]; // have to index m_TempCache after the resize, otherwise it could be empty and not like it
  }
  else
  {
    // otherwise, allocate the final storage immediately
    pTarget = AllocateBytes(bytecount);
    m_ObjectStack.PeekBack()->m_pFirstChild = pTarget;
  }

  WMemoryUtils::Copy(pTarget, pData, bytecount);
}


void WOpenDdlReader::OnPrimitiveBool(WUInt32 count, const bool* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(bool) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveInt8(WUInt32 count, const WInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WInt8) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveInt16(WUInt32 count, const WInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WInt16) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveInt32(WUInt32 count, const WInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WInt32) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveInt64(WUInt32 count, const WInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WInt64) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveUInt8(WUInt32 count, const WUInt8* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WUInt8) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveUInt16(WUInt32 count, const WUInt16* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WUInt16) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveUInt32(WUInt32 count, const WUInt32* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WUInt32) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveUInt64(WUInt32 count, const WUInt64* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(WUInt64) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveFloat(WUInt32 count, const float* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(float) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveDouble(WUInt32 count, const double* pData, bool bThisIsAll)
{
  StorePrimitiveData(bThisIsAll, sizeof(double) * count, (const WUInt8*)pData);
  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}

void WOpenDdlReader::OnPrimitiveString(WUInt32 count, const WStringView* pData, bool bThisIsAll)
{
  W_IGNORE_UNUSED(bThisIsAll);

  const WUInt32 uiDataSize = count * sizeof(WStringView);

  const WUInt32 offset = m_TempCache.GetCount();
  m_TempCache.SetCountUninitialized(m_TempCache.GetCount() + uiDataSize);
  WStringView* pTarget = (WStringView*)&m_TempCache[offset];

  for (WUInt32 i = 0; i < count; ++i)
  {
    pTarget[i] = CopyString(pData[i]);
  }

  m_ObjectStack.PeekBack()->m_uiNumChildElements += count;
}


void WOpenDdlReader::OnParsingError(WStringView sMessage, bool bFatal, WUInt32 uiLine, WUInt32 uiColumn)
{
  W_IGNORE_UNUSED(sMessage);
  W_IGNORE_UNUSED(uiLine);
  W_IGNORE_UNUSED(uiColumn);

  if (bFatal)
  {
    m_ObjectStack.Clear();
    m_GlobalNames.Clear();
    m_Elements.Clear();

    ClearDataChunks();
  }
}

//////////////////////////////////////////////////////////////////////////

void WOpenDdlReader::ClearDataChunks()
{
  for (WUInt32 i = 0; i < m_DataChunks.GetCount(); ++i)
  {
    W_DEFAULT_DELETE(m_DataChunks[i]);
  }

  m_DataChunks.Clear();
}

WUInt8* WOpenDdlReader::AllocateBytes(WUInt32 uiNumBytes)
{
  uiNumBytes = WMemoryUtils::AlignSize(uiNumBytes, static_cast<WUInt32>(W_ALIGNMENT_MINIMUM));

  // if the requested data is very large, just allocate it as an individual chunk
  if (uiNumBytes > s_uiChunkSize / 2)
  {
    WUInt8* pResult = W_DEFAULT_NEW_ARRAY(WUInt8, uiNumBytes).GetPtr();
    m_DataChunks.PushBack(pResult);
    return pResult;
  }

  // if our current chunk is too small, discard the remaining free bytes and just allocate a new chunk
  if (m_uiBytesInChunkLeft < uiNumBytes)
  {
    m_pCurrentChunk = W_DEFAULT_NEW_ARRAY(WUInt8, s_uiChunkSize).GetPtr();
    m_uiBytesInChunkLeft = s_uiChunkSize;
    m_DataChunks.PushBack(m_pCurrentChunk);
  }

  // no fulfill the request from the current chunk
  WUInt8* pResult = m_pCurrentChunk;
  m_pCurrentChunk += uiNumBytes;
  m_uiBytesInChunkLeft -= uiNumBytes;

  return pResult;
}

//////////////////////////////////////////////////////////////////////////

WUInt32 WOpenDdlReaderElement::GetNumChildObjects() const
{
  if (m_PrimitiveType != WOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~W_BIT(31)); // Bit 31 stores whether the name is global
}

WUInt32 WOpenDdlReaderElement::GetNumPrimitives() const
{
  if (m_PrimitiveType == WOpenDdlPrimitiveType::Custom)
    return 0;

  return m_uiNumChildElements & (~W_BIT(31)); // Bit 31 stores whether the name is global
}


bool WOpenDdlReaderElement::HasPrimitives(WOpenDdlPrimitiveType type, WUInt32 uiMinNumberOfPrimitives /*= 1*/) const
{
  /// \test This is new

  if (m_PrimitiveType != type)
    return false;

  return m_uiNumChildElements >= uiMinNumberOfPrimitives;
}

const WOpenDdlReaderElement* WOpenDdlReaderElement::FindChild(WStringView sName) const
{
  W_ASSERT_DEBUG(m_PrimitiveType == WOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const WOpenDdlReaderElement* pChild = static_cast<const WOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetName() == sName)
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const WOpenDdlReaderElement* WOpenDdlReaderElement::FindChildOfType(WOpenDdlPrimitiveType type, WStringView sName, WUInt32 uiMinNumberOfPrimitives /* = 1*/) const
{
  /// \test This is new

  W_ASSERT_DEBUG(m_PrimitiveType == WOpenDdlPrimitiveType::Custom, "Cannot search for a child object in a primitives list");

  const WOpenDdlReaderElement* pChild = static_cast<const WOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == type && pChild->GetName() == sName)
    {
      if (type == WOpenDdlPrimitiveType::Custom || pChild->GetNumPrimitives() >= uiMinNumberOfPrimitives)
        return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}

const WOpenDdlReaderElement* WOpenDdlReaderElement::FindChildOfType(WStringView sType, WStringView sName /*= {}*/) const
{
  const WOpenDdlReaderElement* pChild = static_cast<const WOpenDdlReaderElement*>(m_pFirstChild);

  while (pChild)
  {
    if (pChild->GetPrimitivesType() == WOpenDdlPrimitiveType::Custom && pChild->GetCustomType() == sType && (sName.IsEmpty() || pChild->GetName() == sName))
    {
      return pChild;
    }

    pChild = pChild->GetSibling();
  }

  return nullptr;
}
