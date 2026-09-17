#pragma once

#include <VisualScriptPlugin/Runtime/VisualScript.h>

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptFunctionProperty : public WScriptFunctionProperty
{
public:
  WVisualScriptFunctionProperty(WStringView sName, const WSharedPtr<const WVisualScriptGraphDescription>& pDesc);
  ~WVisualScriptFunctionProperty();

  virtual WFunctionType::Enum GetFunctionType() const override { return WFunctionType::Member; }
  virtual const WRTTI* GetReturnType() const override { return nullptr; }
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const override { return WPropertyFlags::Void; }
  virtual WUInt32 GetArgumentCount() const override { return 0; }
  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const override { return nullptr; }
  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const override { return WPropertyFlags::Void; }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override;

private:
  WSharedPtr<const WVisualScriptGraphDescription> m_pDesc;
};

class W_VISUALSCRIPTPLUGIN_DLL WVisualScriptMessageHandler : public WScriptMessageHandler
{
public:
  WVisualScriptMessageHandler(const WScriptMessageDesc& desc, const WSharedPtr<const WVisualScriptGraphDescription>& pDesc);
  ~WVisualScriptMessageHandler();

  static void Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg);

private:
  WSharedPtr<const WVisualScriptGraphDescription> m_pDesc;
};
