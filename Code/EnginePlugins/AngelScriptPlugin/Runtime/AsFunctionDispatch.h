#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Core/Scripting/ScriptRTTI.h>
#include <Foundation/Strings/String.h>

//////////////////////////////////////////////////////////////////////////

class asIScriptFunction;

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptFunctionProperty : public WScriptFunctionProperty
{
public:
  WAngelScriptFunctionProperty(WStringView sName, asIScriptFunction* pFunction);
  ~WAngelScriptFunctionProperty();

  virtual WFunctionType::Enum GetFunctionType() const override { return WFunctionType::Member; }
  virtual const WRTTI* GetReturnType() const override { return nullptr; }
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const override { return WPropertyFlags::Void; }
  virtual WUInt32 GetArgumentCount() const override { return 0; }
  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const override { return nullptr; }
  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const override { return WPropertyFlags::Void; }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override;

private:
  asIScriptFunction* m_pAsFunction = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptMessageHandler : public WScriptMessageHandler
{
public:
  WAngelScriptMessageHandler(const WScriptMessageDesc& desc, asIScriptFunction* pFunction);
  ~WAngelScriptMessageHandler();

  static void Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg);

private:
  asIScriptFunction* m_pAsFunction = nullptr;
};

//////////////////////////////////////////////////////////////////////////

struct WMsgDeliverAngelScriptMsg : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgDeliverAngelScriptMsg, WMessage);

  ~WMsgDeliverAngelScriptMsg();

  WMsgDeliverAngelScriptMsg(const WMsgDeliverAngelScriptMsg& rhs);
  WMsgDeliverAngelScriptMsg(WMsgDeliverAngelScriptMsg&& rhs);
  void operator=(const WMsgDeliverAngelScriptMsg& rhs);
  void operator=(WMsgDeliverAngelScriptMsg&& rhs);

  bool m_bRelease = false;
  void* m_pAsMsg = nullptr;
};

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptCustomAsMessageHandler : public WScriptMessageHandler
{
public:
  WAngelScriptCustomAsMessageHandler(const WScriptMessageDesc& desc);
  ~WAngelScriptCustomAsMessageHandler();

  void AddReceiver(asIScriptFunction* pFunction, const char* szArgType);

  static void Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg);

private:
  struct Receiver
  {
    WHashedString m_sArgType;
    asIScriptFunction* m_pAsFunction = nullptr;
  };

  WHybridArray<Receiver, 2> m_Receivers;
};
