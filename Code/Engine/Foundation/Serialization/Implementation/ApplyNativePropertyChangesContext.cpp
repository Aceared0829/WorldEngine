#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>


WApplyNativePropertyChangesContext::WApplyNativePropertyChangesContext(WRttiConverterContext& ref_source, const WAbstractObjectGraph& originalGraph)
  : m_NativeContext(ref_source)
  , m_OriginalGraph(originalGraph)
{
}

WUuid WApplyNativePropertyChangesContext::GenerateObjectGuid(const WUuid& parentGuid, const WAbstractProperty* pProp, WVariant index, void* pObject) const
{
  if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
  {
    // If the object is already known by the native context (a pointer that existed before the native changes)
    // we can just return it. Any other pointer will get a new guid assigned.
    WUuid guid = m_NativeContext.GetObjectGUID(pProp->GetSpecificType(), pObject);
    if (guid.IsValid())
      return guid;
  }
  else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
  {
    // In case of by-value classes we lookup the guid in the object manager graph by using
    // the index as the identify of the object. If the index is not valid (e.g. the array was expanded by native changes)
    // a new guid is assigned.
    if (const WAbstractObjectNode* originalNode = m_OriginalGraph.GetNode(parentGuid))
    {
      if (const WAbstractObjectNode::Property* originalProp = originalNode->FindProperty(pProp->GetPropertyName()))
      {
        switch (pProp->GetCategory())
        {
          case WPropertyCategory::Member:
          {
            if (originalProp->m_Value.IsA<WUuid>() && originalProp->m_Value.Get<WUuid>().IsValid())
              return originalProp->m_Value.Get<WUuid>();
          }
          break;
          case WPropertyCategory::Array:
          {
            WUInt32 uiIndex = index.Get<WUInt32>();
            if (originalProp->m_Value.IsA<WVariantArray>())
            {
              const WVariantArray& values = originalProp->m_Value.Get<WVariantArray>();
              if (uiIndex < values.GetCount())
              {
                const auto& originalElemValue = values[uiIndex];
                if (originalElemValue.IsA<WUuid>() && originalElemValue.Get<WUuid>().IsValid())
                  return originalElemValue.Get<WUuid>();
              }
            }
          }
          break;
          case WPropertyCategory::Map:
          {
            const WString& sIndex = index.Get<WString>();
            if (originalProp->m_Value.IsA<WVariantDictionary>())
            {
              const WVariantDictionary& values = originalProp->m_Value.Get<WVariantDictionary>();
              if (values.Contains(sIndex))
              {
                const auto& originalElemValue = *values.GetValue(sIndex);
                if (originalElemValue.IsA<WUuid>() && originalElemValue.Get<WUuid>().IsValid())
                  return originalElemValue.Get<WUuid>();
              }
            }
          }
          break;

          default:
            break;
        }
      }
    }
  }

  return WUuid::MakeUuid();
}
