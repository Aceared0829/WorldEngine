#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

class WDocumentObject;
class WObjectAccessorBase;
struct WPropertyReference;
class WStringBuilder;
class WAbstractProperty;

struct W_EDITORFRAMEWORK_DLL WPropertyReference
{
  bool operator==(const WPropertyReference& rhs) const
  {
    return m_Object == rhs.m_Object && m_pProperty == rhs.m_pProperty && m_Index == rhs.m_Index;
  }
  WUuid m_Object;
  const WAbstractProperty* m_pProperty = nullptr;
  WVariant m_Index;
};

struct W_EDITORFRAMEWORK_DLL WObjectPropertyPathContext
{
  const WDocumentObject* m_pContextObject; ///< Paths start at this object.
  WObjectAccessorBase* m_pAccessor;        ///< Accessor used to traverse hierarchy and query properties.
  WString m_sRootProperty;                 ///< In case m_pContextObject points to the root object, this is the property to follow.
};

class W_EDITORFRAMEWORK_DLL WObjectPropertyPath
{
public:
  static WStatus CreatePath(const WObjectPropertyPathContext& context, const WPropertyReference& prop, WStringBuilder& out_sObjectSearchSequence,
    WStringBuilder& out_sComponentType, WStringBuilder& out_sPropertyPath);
  static WStatus CreatePropertyPath(const WObjectPropertyPathContext& context, const WPropertyReference& prop, WStringBuilder& out_sPropertyPath);
  static void AppendSubIndices(WStringBuilder& ref_sPropertyPath, WArrayPtr<WVariant> indices);
  static WStatus ResolvePath(const WObjectPropertyPathContext& context, WDynamicArray<WPropertyReference>& out_keys,
    const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath);
  static WStatus ResolvePropertyPath(const WObjectPropertyPathContext& context, const char* szPropertyPath, WPropertyReference& out_key);

  static const WDocumentObject* FindParentNodeComponent(const WDocumentObject* pObject);

private:
  static WStatus PrependProperty(
    const WDocumentObject* pObject, const WAbstractProperty* pProperty, WVariant index, WStringBuilder& out_sPropertyPath);
};
