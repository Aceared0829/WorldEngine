#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

void WToolsSerializationUtils::SerializeTypes(const WSet<const WRTTI*>& types, WAbstractObjectGraph& ref_typesGraph)
{
  WRttiConverterContext context;
  WRttiConverterWriter rttiConverter(&ref_typesGraph, &context, true, true);
  for (const WRTTI* pType : types)
  {
    WReflectedTypeDescriptor desc;
    if (pType->GetTypeFlags().IsSet(WTypeFlags::Phantom))
    {
      WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pType, desc);
    }
    else
    {
      WToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(pType, desc);
    }

    context.RegisterObject(WUuid::MakeStableUuidFromString(pType->GetTypeName()), WGetStaticRTTI<WReflectedTypeDescriptor>(), &desc);
    rttiConverter.AddObjectToGraph(WGetStaticRTTI<WReflectedTypeDescriptor>(), &desc);
  }
}

void WToolsSerializationUtils::CopyProperties(const WDocumentObject* pSource, const WDocumentObjectManager* pSourceManager, void* pTarget, const WRTTI* pTargetType, FilterFunction propertFilter)
{
  WAbstractObjectGraph graph;
  WDocumentObjectConverterWriter writer(&graph, pSourceManager, [](const WDocumentObject*, const WAbstractProperty* p)
    { return p->GetAttributeByType<WHiddenAttribute>() == nullptr; });
  WAbstractObjectNode* pAbstractObj = writer.AddObjectToGraph(pSource);

  WRttiConverterContext context;
  WRttiConverterReader reader(&graph, &context);

  reader.ApplyPropertiesToObject(pAbstractObj, pTargetType, pTarget);
}
