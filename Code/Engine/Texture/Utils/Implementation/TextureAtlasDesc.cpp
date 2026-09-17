#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

WResult WTextureAtlasCreationDesc::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(4);

  if (m_Layers.GetCount() > 255u)
    return W_FAILURE;

  const WUInt8 uiNumLayers = static_cast<WUInt8>(m_Layers.GetCount());
  inout_stream << uiNumLayers;

  for (WUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream << m_Layers[l].m_Usage;
    inout_stream << m_Layers[l].m_uiNumChannels;
  }

  inout_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    inout_stream << item.m_uiUniqueID;
    inout_stream << item.m_uiFlags;

    for (WUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream << item.m_sLayerInput[l];
    }

    inout_stream << item.m_sAlphaInput;

    inout_stream << item.m_uiNumVariationsX;
    inout_stream << item.m_uiNumVariationsY;
  }

  return W_SUCCESS;
}

WResult WTextureAtlasCreationDesc::Deserialize(WStreamReader& inout_stream)
{
  const WTypeVersion uiVersion = inout_stream.ReadVersion(4);

  WUInt8 uiNumLayers = 0;
  inout_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (WUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream >> m_Layers[l].m_Usage;
    inout_stream >> m_Layers[l].m_uiNumChannels;
  }

  WUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    inout_stream >> item.m_uiUniqueID;
    inout_stream >> item.m_uiFlags;

    for (WUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      inout_stream >> item.m_sAlphaInput;
    }

    if (uiVersion >= 4)
    {
      inout_stream >> item.m_uiNumVariationsX;
      inout_stream >> item.m_uiNumVariationsY;
    }
  }

  return W_SUCCESS;
}

WResult WTextureAtlasCreationDesc::Save(WStringView sFile) const
{
  WFileWriter file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  return Serialize(file);
}

WResult WTextureAtlasCreationDesc::Load(WStringView sFile)
{
  WFileReader file;
  W_SUCCEED_OR_RETURN(file.Open(sFile));

  return Deserialize(file);
}

void WTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

WResult WTextureAtlasRuntimeDesc::Serialize(WStreamWriter& inout_stream) const
{
  m_Items.Sort();

  inout_stream.WriteVersion(2);

  inout_stream << m_uiNumLayers;
  inout_stream << m_Items.GetCount();

  for (WUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    inout_stream << m_Items.GetKey(i);
    inout_stream << m_Items.GetValue(i).m_uiFlags;

    for (WUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      inout_stream << r.x;
      inout_stream << r.y;
      inout_stream << r.width;
      inout_stream << r.height;
    }

    inout_stream << m_Items.GetValue(i).m_uiNumVariationsX;
    inout_stream << m_Items.GetValue(i).m_uiNumVariationsY;
  }

  return W_SUCCESS;
}

WResult WTextureAtlasRuntimeDesc::Deserialize(WStreamReader& inout_stream)
{
  Clear();

  const WTypeVersion uiVersion = inout_stream.ReadVersion(2);

  inout_stream >> m_uiNumLayers;

  WUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (WUInt32 i = 0; i < uiNumItems; ++i)
  {
    WUInt32 key = 0;
    inout_stream >> key;

    auto& item = m_Items[key];
    inout_stream >> item.m_uiFlags;

    for (WUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      inout_stream >> r.x;
      inout_stream >> r.y;
      inout_stream >> r.width;
      inout_stream >> r.height;
    }

    if (uiVersion >= 2)
    {
      inout_stream >> item.m_uiNumVariationsX;
      inout_stream >> item.m_uiNumVariationsY;
    }
  }

  m_Items.Sort();
  return W_SUCCESS;
}
