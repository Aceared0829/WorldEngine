#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

class WRenderGraph;

using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;
using WShaderResourceHandle = WTypedResourceHandle<class WShaderResource>;

namespace WRmlUiInternal
{
  struct GeometryId : public WGenericId<24, 8>
  {
    using WGenericId::WGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle hGeometry) { return GeometryId(static_cast<WUInt32>(hGeometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public WGenericId<24, 8>
  {
    using WGenericId::WGenericId;

    static TextureId FromRml(Rml::TextureHandle hTexture) { return TextureId(static_cast<WUInt32>(hTexture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  struct ShaderId : public WGenericId<24, 8>
  {
    using WGenericId::WGenericId;

    static ShaderId FromRml(Rml::CompiledShaderHandle hShader) { return ShaderId(static_cast<WUInt32>(hShader)); }

    Rml::CompiledShaderHandle ToRml() const { return m_Data; }
  };

  struct ShaderType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Invalid,
      Gradient,
      Custom,

      Default = Invalid
    };
  };

  //////////////////////////////////////////////////////////////////////////

  struct CompiledGeometry;
  struct CommandBuffer;
  struct CommandRenderGeometry;

  class RenderInterface final : public Rml::RenderInterface
  {
  public:
    RenderInterface();
    virtual ~RenderInterface();

    // Interface implementation
    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    virtual void RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry) override;

    virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource) override;
    virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize) override;
    virtual void ReleaseTexture(Rml::TextureHandle hTexture) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(Rml::Rectanglei region) override;

    virtual void EnableClipMask(bool bEnable) override;
    virtual void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation) override;

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    virtual Rml::LayerHandle PushLayer() override;
    virtual void CompositeLayers(Rml::LayerHandle hSource, Rml::LayerHandle hDestination, Rml::BlendMode blendMode, Rml::Span<const Rml::CompiledFilterHandle> filters) override;
    virtual void PopLayer() override;

    virtual Rml::TextureHandle SaveLayerAsTexture() override;
    virtual Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

    virtual Rml::CompiledFilterHandle CompileFilter(const Rml::String& sName, const Rml::Dictionary& parameters) override;
    virtual void ReleaseFilter(Rml::CompiledFilterHandle hFilter) override;

    virtual Rml::CompiledShaderHandle CompileShader(const Rml::String& sName, const Rml::Dictionary& parameters) override;
    virtual void RenderShader(Rml::CompiledShaderHandle hShader, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseShader(Rml::CompiledShaderHandle hShader) override;

    // W specific functions
    void BeginExtraction(const WHashedString& sName, WGALTextureHandle hTargetTexture);
    void EndExtraction();

  private:
    void GALEventHandler(const WGALDeviceEvent& e);
    void BeginFrame();
    void EndFrame();
    void FreeReleasedGeometry(GeometryId id);

    void FillRenderCommand(CommandRenderGeometry& out_cmd, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture);
    WUniquePtr<CommandBuffer> AllocateCommandBuffer();
    void FreeCommandBuffer(WUniquePtr<CommandBuffer>&& pBuffer);
    void SubmitCommandBuffer(WUniquePtr<CommandBuffer>&& pBuffer);

    WIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      WUInt64 m_uiFrame;
      GeometryId m_Id;
    };

    WMutex m_ReleasedCompiledGeometryMutex;
    WDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    struct TextureInfo
    {
      WTexture2DResourceHandle m_hTexture;
      bool m_bHasPremultipliedAlpha = false;
    };

    WIdTable<TextureId, TextureInfo> m_Textures;
    WTexture2DResourceHandle m_hNoiseTexture;
    WTexture2DResourceHandle m_hFallbackTexture;

    struct ShaderInfo
    {
      WShaderResourceHandle m_hShader;
      WGALBufferHandle m_hAdditionalConstantBuffer;
      WEnum<ShaderType> m_Type;
    };

    WIdTable<ShaderId, ShaderInfo> m_Shaders;

    WMat4 m_mProjection = WMat4::MakeIdentity();
    WMat4 m_mTransform = WMat4::MakeIdentity();
    bool m_bUseStencilTest = false;

    WDynamicArray<WUniquePtr<CommandBuffer>> m_FreeCommandBuffers;
    WDynamicArray<WUniquePtr<CommandBuffer>> m_SubmittedCommandBuffers[2];

    WUniquePtr<CommandBuffer> m_pCurrentCommandBuffer;

    WShaderResourceHandle m_hMainShader;
    WConstantBufferStorageHandle m_hMainConstantBuffer;
    WSmallArray<WGALVertexAttribute, 3> m_VertexAttributes;

    WSharedPtr<WRenderGraph> m_pRenderGraph;
  };
} // namespace WRmlUiInternal
