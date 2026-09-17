#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WPhantomConstantProperty : public WAbstractConstantProperty
{
public:
  WPhantomConstantProperty(const WReflectedPropertyDescriptor* pDesc);
  ~WPhantomConstantProperty();

  virtual const WRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer() const override;
  virtual WVariant GetConstant() const override { return m_Value; }

private:
  WVariant m_Value;
  WString m_sPropertyNameStorage;
  const WRTTI* m_pPropertyType;
};

class WPhantomMemberProperty : public WAbstractMemberProperty
{
public:
  WPhantomMemberProperty(const WReflectedPropertyDescriptor* pDesc);
  ~WPhantomMemberProperty();

  virtual const WRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer(const void* pInstance) const override { return nullptr; }
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override {}
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override {}

private:
  WString m_sPropertyNameStorage;
  const WRTTI* m_pPropertyType;
};

class WPhantomFunctionProperty : public WAbstractFunctionProperty
{
public:
  WPhantomFunctionProperty(WReflectedFunctionDescriptor* pDesc);
  ~WPhantomFunctionProperty();

  virtual WFunctionType::Enum GetFunctionType() const override;
  virtual const WRTTI* GetReturnType() const override;
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const override;
  virtual WUInt32 GetArgumentCount() const override;
  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const override;
  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const override;
  virtual void Execute(void* pInstance, WArrayPtr<WVariant> values, WVariant& ref_returnValue) const override;

private:
  WString m_sPropertyNameStorage;
  WEnum<WFunctionType> m_FunctionType;
  WFunctionArgumentDescriptor m_ReturnValue;
  WDynamicArray<WFunctionArgumentDescriptor> m_Arguments;
};


class WPhantomArrayProperty : public WAbstractArrayProperty
{
public:
  WPhantomArrayProperty(const WReflectedPropertyDescriptor* pDesc);
  ~WPhantomArrayProperty();

  virtual const WRTTI* GetSpecificType() const override;
  virtual WUInt32 GetCount(const void* pInstance) const override { return 0; }
  virtual void GetValue(const void* pInstance, WUInt32 uiIndex, void* pObject) const override {}
  virtual void SetValue(void* pInstance, WUInt32 uiIndex, const void* pObject) const override {}
  virtual void Insert(void* pInstance, WUInt32 uiIndex, const void* pObject) const override {}
  virtual void Remove(void* pInstance, WUInt32 uiIndex) const override {}
  virtual void Clear(void* pInstance) const override {}
  virtual void SetCount(void* pInstance, WUInt32 uiCount) const override {}


private:
  WString m_sPropertyNameStorage;
  const WRTTI* m_pPropertyType;
};


class WPhantomSetProperty : public WAbstractSetProperty
{
public:
  WPhantomSetProperty(const WReflectedPropertyDescriptor* pDesc);
  ~WPhantomSetProperty();

  virtual const WRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) const override {}
  virtual void Insert(void* pInstance, const void* pObject) const override {}
  virtual void Remove(void* pInstance, const void* pObject) const override {}
  virtual bool Contains(const void* pInstance, const void* pObject) const override { return false; }
  virtual void GetValues(const void* pInstance, WDynamicArray<WVariant>& out_keys) const override {}

private:
  WString m_sPropertyNameStorage;
  const WRTTI* m_pPropertyType;
};


class WPhantomMapProperty : public WAbstractMapProperty
{
public:
  WPhantomMapProperty(const WReflectedPropertyDescriptor* pDesc);
  ~WPhantomMapProperty();

  virtual const WRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) const override {}
  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) const override {}
  virtual void Remove(void* pInstance, const char* szKey) const override {}
  virtual bool Contains(const void* pInstance, const char* szKey) const override { return false; }
  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override { return false; }
  virtual void GetKeys(const void* pInstance, WHybridArray<WString, 16>& out_keys) const override {}

private:
  WString m_sPropertyNameStorage;
  const WRTTI* m_pPropertyType;
};
