#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>

class WAsWriteStream : public asIBinaryStream
{
public:
  int Write(const void* pPtr, asUINT size)
  {
    m_pBuffer->PushBackRange(WConstByteArrayPtr((const WUInt8*)pPtr, size));
    return size;
  }

  int Read(void* pPtr, asUINT size)
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }

  WDynamicArray<WUInt8>* m_pBuffer = nullptr;
};

class WAsReadStream : public asIBinaryStream
{
public:
  int Write(const void* pPtr, asUINT size)
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return 0;
  }

  int Read(void* pPtr, asUINT size)
  {
    const WUInt32 uiReadSize = WMath::Min(size, m_Buffer.GetCount() - m_uiReadPos);

    WMemoryUtils::RawByteCopy(pPtr, m_Buffer.GetPtr() + m_uiReadPos, uiReadSize);

    m_uiReadPos += uiReadSize;
    return uiReadSize;
  }

  WUInt32 m_uiReadPos = 0;
  WArrayPtr<WUInt8> m_Buffer;
};

void WAngelScriptUtils::SaveByteCode(asIScriptModule* pModule, WDynamicArray<WUInt8>& out_byteCode)
{
  WAsWriteStream stream;
  stream.m_pBuffer = &out_byteCode;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  pModule->SaveByteCode(&stream, false); // TODO AS: strip debug info ? (flag?)
#else
  pModule->SaveByteCode(&stream, true);
#endif
}

asIScriptModule* WAngelScriptUtils::LoadFromByteCode(asIScriptEngine* pEngine, WStringView sModuleName, WArrayPtr<WUInt8> byteCode)
{
  WAsReadStream stream;
  stream.m_Buffer = byteCode;

  WStringBuilder tmp;
  asIScriptModule* pModule = pEngine->GetModule(sModuleName.GetData(tmp), asGM_ALWAYS_CREATE);

  if (pModule->LoadByteCode(&stream) < 0)
  {
    return nullptr;
  }

  return pModule;
}
