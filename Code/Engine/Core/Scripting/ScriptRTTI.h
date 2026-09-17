#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class WWorld;

/// Runtime type information for script classes, extending WRTTI with script-specific functionality.
///
/// Manages type metadata for script classes including function properties and message handlers.
/// Supports reference counting and provides efficient storage for small numbers of functions
/// and message handlers through inplace storage optimization.
class W_CORE_DLL WScriptRTTI : public WRTTI, public WRefCountingImpl
{
  W_DISALLOW_COPY_AND_ASSIGN(WScriptRTTI);

public:
  enum
  {
    NumInplaceFunctions = 7
  };

  using FunctionList = WSmallArray<WUniquePtr<WAbstractFunctionProperty>, NumInplaceFunctions>;
  using MessageHandlerList = WSmallArray<WUniquePtr<WAbstractMessageHandler>, NumInplaceFunctions>;

  WScriptRTTI(WStringView sName, const WRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~WScriptRTTI();

  const WAbstractFunctionProperty* GetFunctionByIndex(WUInt32 uiIndex) const;

private:
  WString m_sTypeNameStorage;
  FunctionList m_FunctionStorage;
  MessageHandlerList m_MessageHandlerStorage;
  WSmallArray<const WAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  WSmallArray<WAbstractMessageHandler*, NumInplaceFunctions> m_MessageHandlerRawPtrs;
};

class W_CORE_DLL WScriptFunctionProperty : public WAbstractFunctionProperty
{
public:
  WScriptFunctionProperty(WStringView sName);
  ~WScriptFunctionProperty();

private:
  WHashedString m_sPropertyNameStorage;
};

struct WScriptMessageDesc
{
  const WRTTI* m_pType = nullptr;
  WArrayPtr<const WAbstractProperty* const> m_Properties;
};

class W_CORE_DLL WScriptMessageHandler : public WAbstractMessageHandler
{
public:
  WScriptMessageHandler(const WScriptMessageDesc& desc);
  ~WScriptMessageHandler();

  void FillMessagePropertyValues(const WMessage& msg, WDynamicArray<WVariant>& out_propertyValues);

private:
  WArrayPtr<const WAbstractProperty* const> m_Properties;
};

class W_CORE_DLL WScriptInstance
{
public:
  WScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld);
  virtual ~WScriptInstance() = default;

  WReflectedClass& GetOwner() { return m_Owner; }
  WWorld* GetWorld() { return m_pWorld; }

  virtual void SetInstanceVariables(const WArrayMap<WHashedString, WVariant>& parameters);
  virtual void SetInstanceVariable(const WHashedString& sName, const WVariant& value) = 0;
  virtual WVariant GetInstanceVariable(const WHashedString& sName) = 0;

private:
  WReflectedClass& m_Owner;
  WWorld* m_pWorld = nullptr;
};

struct W_CORE_DLL WScriptAllocator
{
  static WAllocator* GetAllocator();
};

/// creates a new instance of type using the script allocator
#define W_SCRIPT_NEW(type, ...) W_NEW(WScriptAllocator::GetAllocator(), type, __VA_ARGS__)
