#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/EndianHelper.h>
#include <Foundation/Memory/MemoryUtils.h>

void WEndianHelper::SwitchStruct(void* pDataPointer, const char* szFormat)
{
  W_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  W_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");

  WUInt8* pWorkPointer = static_cast<WUInt8*>(pDataPointer);
  char cCurrentElement = *szFormat;

  while (cCurrentElement != '\0')
  {
    switch (cCurrentElement)
    {
      case 'c':
      case 'b':
        pWorkPointer++;
        break;

      case 's':
      case 'w':
      {
        WUInt16* pWordElement = reinterpret_cast<WUInt16*>(pWorkPointer);
        *pWordElement = Switch(*pWordElement);
        pWorkPointer += sizeof(WUInt16);
      }
      break;

      case 'd':
      {
        WUInt32* pDWordElement = reinterpret_cast<WUInt32*>(pWorkPointer);
        *pDWordElement = Switch(*pDWordElement);
        pWorkPointer += sizeof(WUInt32);
      }
      break;

      case 'q':
      {
        WUInt64* pQWordElement = reinterpret_cast<WUInt64*>(pWorkPointer);
        *pQWordElement = Switch(*pQWordElement);
        pWorkPointer += sizeof(WUInt64);
      }
      break;
    }

    szFormat++;
    cCurrentElement = *szFormat;
  }
}

void WEndianHelper::SwitchStructs(void* pDataPointer, const char* szFormat, WUInt32 uiStride, WUInt32 uiCount)
{
  W_ASSERT_DEBUG(pDataPointer != nullptr, "Data necessary!");
  W_ASSERT_DEBUG((szFormat != nullptr) && (szFormat[0] != '\0'), "Struct format description necessary!");
  W_ASSERT_DEBUG(uiStride > 0, "Struct size necessary!");

  for (WUInt32 i = 0; i < uiCount; i++)
  {
    SwitchStruct(pDataPointer, szFormat);
    pDataPointer = WMemoryUtils::AddByteOffset(pDataPointer, uiStride);
  }
}
