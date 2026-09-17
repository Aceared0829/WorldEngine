#include <Foundation/FoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WGraphPatch);

WGraphPatch::WGraphPatch(const char* szType, WUInt32 uiTypeVersion, PatchType type)
  : m_szType(szType)
  , m_uiTypeVersion(uiTypeVersion)
  , m_PatchType(type)
{
}

const char* WGraphPatch::GetType() const
{
  return m_szType;
}

WUInt32 WGraphPatch::GetTypeVersion() const
{
  return m_uiTypeVersion;
}


WGraphPatch::PatchType WGraphPatch::GetPatchType() const
{
  return m_PatchType;
}
