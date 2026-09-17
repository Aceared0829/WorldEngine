#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>
#include <Texture/WTexFormat/WTexFormat.h>

/// Resource loader for texture resources.
///
/// Loads textures from .WTex files which contain compressed texture data and metadata.
class W_RENDERERCORE_DLL WTextureResourceLoader : public WResourceTypeLoader
{
public:
  /// Data structure for loaded texture information.
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    WContiguousMemoryStreamStorage m_Storage;
    WMemoryStreamReader m_Reader;
    WImage m_Image;

    bool m_bIsFallback = false;
    WTexFormat m_TexFormat; ///< Texture format information from the .WTex file.
  };

  virtual WResourceLoadData OpenDataStream(const WResource* pResource) override;
  virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const WResource* pResource) const override;

  /// Loads texture data from a .WTex file stream.
  static WResult LoadTexFile(WStreamReader& inout_stream, LoadedData& ref_data);

  /// Writes texture data to a stream in the loader's internal format.
  static void WriteTextureLoadStream(WStreamWriter& inout_stream, const LoadedData& data);
};
