

#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>

#include <RendererCore/../../../Data/Plugins/RmlUiPlugin/Shaders/RmlUiConstants.h>

namespace WRmlUiInternal
{
  struct Vertex
  {
    W_DECLARE_POD_TYPE();

    WVec2 m_Position;
    WVec2 m_TexCoord;
    WColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    WUInt32 m_uiTriangleCount = 0;
    WGALBufferHandle m_hVertexBuffer;
    WGALBufferHandle m_hIndexBuffer;
  };

  struct CommandType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Invalid,
      RenderGeometry,
      RenderShader,
      SetScissorRegion,
      RenderToClipMask,

      Default = Invalid
    };
  };

  struct alignas(4) CommandHeader
  {
    WEnum<CommandType> m_Type;
    bool m_bNeedsPremultipliedAlpha = false;
    bool m_bUseStencilTest = false;
    WUInt8 m_uiSize = 0;
  };

  struct CommandRenderGeometry : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderGeometry;

    CompiledGeometry m_CompiledGeometry;
    WGALTextureHandle m_hTexture;
    WMat4 m_Transform = WMat4::MakeIdentity();
    WVec2 m_Translation = WVec2::MakeZero();
  };

  struct CommandRenderShader : CommandRenderGeometry
  {
    static constexpr CommandType::Enum Type = CommandType::RenderShader;

    WShaderResourceHandle m_hShader;
    WGALBufferHandle m_hAdditionalConstantBuffer;
    WEnum<ShaderType> m_ShaderType;
  };

  struct CommandSetScissorRegion : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::SetScissorRegion;

    WRectU32 m_ScissorRect = {};
  };

  struct CommandRenderToClipMask : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderToClipMask;

    Rml::ClipMaskOperation m_Operation;
    CompiledGeometry m_CompiledGeometry;
    WMat4 m_Transform = WMat4::MakeIdentity();
    WVec2 m_Translation = WVec2::MakeZero();
  };

  struct CommandBuffer
  {
    CommandBuffer() = default;
    ~CommandBuffer()
    {
      Clear();
    }

    WHashedString m_sName;
    WDynamicArray<WUInt8> m_Buffer;
    WGALTextureHandle m_hTargetTexture;
    WUInt32 m_uiTargetWidth = 0;
    WUInt32 m_uiTargetHeight = 0;

    template <typename T>
    T& AddCommand()
    {
      static_assert(sizeof(T) <= 255, "Command size must fit into a single byte");

      T& cmd = *reinterpret_cast<T*>(m_Buffer.ExpandBy(sizeof(T)));
      cmd.m_Type = T::Type;
      cmd.m_uiSize = sizeof(T);
      return cmd;
    }

    template <typename T>
    const T& ConsumeCommand(WUInt32& inout_uiOffset) const
    {
      const T& cmd = *reinterpret_cast<const T*>(m_Buffer.GetData() + inout_uiOffset);
      inout_uiOffset += sizeof(T);
      return cmd;
    }

    CommandType::Enum PeekCommandType(WUInt32 uiOffset) const
    {
      return static_cast<CommandType::Enum>(*(m_Buffer.GetData() + uiOffset));
    }

    const CommandHeader& PeekCommandHeader(WUInt32 uiOffset) const
    {
      return *reinterpret_cast<const CommandHeader*>(m_Buffer.GetData() + uiOffset);
    }

    void Clear()
    {
      WUInt32 uiOffset = 0;
      while (uiOffset < m_Buffer.GetCount())
      {
        const CommandHeader& header = PeekCommandHeader(uiOffset);
        if (header.m_Type == CommandType::RenderShader)
        {
          auto& cmd = *reinterpret_cast<CommandRenderShader*>(m_Buffer.GetData() + uiOffset);

          // Prevent resource leak by explicit invalidation since no destructors are called for command structs when the buffer is cleared below.
          cmd.m_hShader.Invalidate();
        }

        uiOffset += header.m_uiSize;
      }

      m_Buffer.Clear();
    }
  };

  //////////////////////////////////////////////////////////////////////////

  RenderInterface::RenderInterface()
  {
    WGALDevice::s_Events.AddEventHandler(WMakeDelegate(&RenderInterface::GALEventHandler, this));

    m_hNoiseTexture = WResourceManager::LoadResource<WTexture2DResource>("{ ac614d7c-2b31-4a7b-aa0c-c5d8200b7b89 }"); // BlueNoise
    m_hFallbackTexture = WResourceManager::LoadResource<WTexture2DResource>("White.color");
    m_hMainShader = WResourceManager::LoadResource<WShaderResource>("Shaders/RmlUi.WShader");
    m_hMainConstantBuffer = WRenderContext::CreateConstantBufferStorage<WRmlUiConstants>();

    // Setup the vertex declaration
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Position;
      va.m_eFormat = WGALResourceFormat::XYFloat;
      va.m_uiOffset = offsetof(WRmlUiInternal::Vertex, m_Position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = WGALResourceFormat::UVFloat;
      va.m_uiOffset = offsetof(WRmlUiInternal::Vertex, m_TexCoord);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = WGALVertexAttributeSemantic::Color0;
      va.m_eFormat = WGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(WRmlUiInternal::Vertex, m_Color);
    }

    m_pRenderGraph = WRenderGraphManager::CreateRenderGraph("RmlUi", WRenderGraphPhase::PreRender);
  }

  RenderInterface::~RenderInterface()
  {
    WGALDevice::s_Events.RemoveEventHandler(WMakeDelegate(&RenderInterface::GALEventHandler, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }
    m_CompiledGeometry.Clear();

    m_Textures.Clear();
    m_hFallbackTexture.Invalidate();

    m_Shaders.Clear();
    m_hMainShader.Invalidate();
  }

  Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
  {
    const WUInt32 uiNumVertices = static_cast<WUInt32>(vertices.size());
    const WUInt32 uiNumIndices = static_cast<WUInt32>(indices.size());

    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = uiNumIndices / 3;

    // vertices
    {
      WTempArray<Vertex> vertexStorage;
      vertexStorage.SetCountUninitialized(uiNumVertices);

      for (WUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = vertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = WRmlUiConversionUtils::ToVec2(srcVertex.position);
        destVertex.m_TexCoord = WRmlUiConversionUtils::ToVec2(srcVertex.tex_coord);
        destVertex.m_Color = WRmlUiConversionUtils::ToColor(srcVertex.colour);
      }

      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferFlags = WGALBufferUsageFlags::VertexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      geometry.m_hVertexBuffer = WGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      WGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(WUInt32);
      desc.m_uiTotalSize = uiNumIndices * desc.m_uiStructSize;
      desc.m_BufferFlags = WGALBufferUsageFlags::IndexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      geometry.m_hIndexBuffer = WGALDevice::GetDefaultDevice()->CreateBuffer(desc, WMakeArrayPtr(indices.data(), uiNumIndices).ToByteArray());
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void RenderInterface::RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderGeometry>();
    FillRenderCommand(cmd, hGeometry, translation, hTexture);
  }

  void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry)
  {
    W_LOCK(m_ReleasedCompiledGeometryMutex);

    m_ReleasedCompiledGeometry.PushBack({WRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry)});
  }

  Rml::TextureHandle RenderInterface::LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource)
  {
    WTexture2DResourceHandle hTexture = WResourceManager::LoadResource<WTexture2DResource>(WRmlUiConversionUtils::ToStringView(sSource));

    WResourceLock<WTexture2DResource> pTexture(hTexture, WResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == WResourceAcquireResult::Final)
    {
      out_textureSize = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      TextureInfo textureInfo;
      textureInfo.m_hTexture = hTexture;
      textureInfo.m_bHasPremultipliedAlpha = false;

      return m_Textures.Insert(textureInfo).ToRml();
    }

    return WRmlUiInternal::TextureId().ToRml();
  }

  Rml::TextureHandle RenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize)
  {
    WUInt32 uiWidth = sourceSize.x;
    WUInt32 uiHeight = sourceSize.y;
    WUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;
    W_ASSERT_DEV(uiSizeInBytes == source.size(), "Invalid source size");

    WUInt64 uiHash = WHashingUtils::xxHash64(source.data(), uiSizeInBytes);

    WStringBuilder sTextureName;
    sTextureName.SetFormat("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    WTexture2DResourceHandle hTexture = WResourceManager::GetExistingResource<WTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      WGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = WMakeByteBlobPtr(source.data(), uiSizeInBytes);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      WTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = WGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent = WMakeArrayPtr(&memoryDesc, 1);

      hTexture = WResourceManager::GetOrCreateResource<WTexture2DResource>(sTextureName, std::move(desc));
    }

    TextureInfo textureInfo;
    textureInfo.m_hTexture = hTexture;
    textureInfo.m_bHasPremultipliedAlpha = true;

    return m_Textures.Insert(textureInfo).ToRml();
  }

  void RenderInterface::ReleaseTexture(Rml::TextureHandle hTexture)
  {
    TextureId textureId = TextureId::FromRml(hTexture);
    if (textureId.IsInvalidated() == false)
    {
      W_VERIFY(m_Textures.Remove(textureId), "Invalid texture handle");
    }
  }

  void RenderInterface::EnableScissorRegion(bool bEnable)
  {
    // Scissor is always enabled in our shaders, set the full viewport if disabled
    if (bEnable == false)
    {
      SetScissorRegion(Rml::Rectanglei::FromSize({(int)m_pCurrentCommandBuffer->m_uiTargetWidth, (int)m_pCurrentCommandBuffer->m_uiTargetHeight}));
    }
  }

  void RenderInterface::SetScissorRegion(Rml::Rectanglei region)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandSetScissorRegion>();
    cmd.m_ScissorRect = WRectU32(region.Left(), region.Top(), region.Width(), region.Height());
  }

  void RenderInterface::EnableClipMask(bool bEnable)
  {
    m_bUseStencilTest = bEnable;
  }

  void RenderInterface::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderToClipMask>();
    cmd.m_Operation = operation;
    cmd.m_CompiledGeometry = m_CompiledGeometry[GeometryId::FromRml(hGeometry)];
    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = WRmlUiConversionUtils::ToVec2(translation);
  }

  void RenderInterface::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr bool bColumnMajor = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value;

      if (bColumnMajor)
        m_mTransform = m_mProjection * WMat4::MakeFromColumnMajorArray(pTransform->data());
      else
        m_mTransform = m_mProjection * WMat4::MakeFromRowMajorArray(pTransform->data());
    }
    else
    {
      m_mTransform = m_mProjection;
    }
  }

  Rml::LayerHandle RenderInterface::PushLayer()
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  void RenderInterface::CompositeLayers(Rml::LayerHandle hSource, Rml::LayerHandle hDestination, Rml::BlendMode blendMode, Rml::Span<const Rml::CompiledFilterHandle> filters)
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  void RenderInterface::PopLayer()
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::TextureHandle RenderInterface::SaveLayerAsTexture()
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  Rml::CompiledFilterHandle RenderInterface::SaveLayerAsMaskImage()
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  Rml::CompiledFilterHandle RenderInterface::CompileFilter(const Rml::String& sName, const Rml::Dictionary& parameters)
  {
    W_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  void RenderInterface::ReleaseFilter(Rml::CompiledFilterHandle hFilter)
  {
    W_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::CompiledShaderHandle RenderInterface::CompileShader(const Rml::String& sName, const Rml::Dictionary& parameters)
  {
    auto ApplyColorStopList = [](WRmlUiAdditionalConstants& out_data, const Rml::Dictionary& shader_parameters)
    {
      auto it = shader_parameters.find("color_stop_list");
      W_ASSERT_DEV(it != shader_parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST, "Color stop list not found or invalid type");
      const Rml::ColorStopList& color_stop_list = it->second.GetReference<Rml::ColorStopList>();
      const WUInt32 uiNumStops = WMath::Min(static_cast<WUInt32>(color_stop_list.size()), GRADIENT_MAX_NUM_STOPS);

      out_data.GradientNumStops = uiNumStops;
      float* pStopPositions = &out_data.GradientStopPositions[0].x;
      for (WUInt32 i = 0; i < uiNumStops; i++)
      {
        const Rml::ColorStop& stop = color_stop_list[i];
        W_ASSERT_DEV(stop.position.unit == Rml::Unit::NUMBER, "Invalid color stop position unit");
        pStopPositions[i] = stop.position.number;
        out_data.GradientStopColors[i] = WRmlUiConversionUtils::ToColor(stop.color);
      }
    };

    auto CreateGradientConstantBuffer = [](const WRmlUiAdditionalConstants& data) -> WGALBufferHandle
    {
      WGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = 0;
      bufferDesc.m_BufferFlags = WGALBufferUsageFlags::ConstantBuffer;
      bufferDesc.m_ResourceAccess.m_bImmutable = true;
      bufferDesc.m_uiTotalSize = sizeof(WRmlUiAdditionalConstants);

      return WGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc, WMakeByteArrayPtr(&data, 1));
    };

    ShaderInfo shaderInfo;
    shaderInfo.m_hShader = m_hMainShader;

    const bool repeating = Rml::Get(parameters, "repeating", false);

    WRmlUiAdditionalConstants data;
    WMemoryUtils::ZeroFill(&data);

    if (sName == "linear-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_LINEAR : GRADIENT_LINEAR;
      data.GradientParams0 = WRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "p0", Rml::Vector2f(0.f)));
      data.GradientParams1 = WRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "p1", Rml::Vector2f(0.f))) - data.GradientParams0;
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }
    else if (sName == "radial-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_RADIAL : GRADIENT_RADIAL;
      data.GradientParams0 = WRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "center", Rml::Vector2f(0.f)));
      data.GradientParams1 = WRmlUiConversionUtils::ToVec2(Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f)));
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }
    else if (sName == "conic-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_CONIC : GRADIENT_CONIC;
      data.GradientParams0 = WRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "center", Rml::Vector2f(0.f)));

      const WAngle angle = WAngle::MakeFromRadian(Rml::Get(parameters, "angle", 0.f));
      data.GradientParams1 = WVec2(WMath::Cos(angle), WMath::Sin(angle));
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }


    if (shaderInfo.m_Type != ShaderType::Invalid)
    {
      return m_Shaders.Insert(shaderInfo).ToRml();
    }

    WLog::Warning("Unsupported shader type '{}'.", sName.c_str());
    return {};
  }

  void RenderInterface::RenderShader(Rml::CompiledShaderHandle hShader, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderShader>();
    FillRenderCommand(cmd, hGeometry, translation, hTexture);

    ShaderInfo shaderInfo;
    W_VERIFY(m_Shaders.TryGetValue(ShaderId::FromRml(hShader), shaderInfo), "Invalid shader handle");

    cmd.m_hShader = shaderInfo.m_hShader;
    cmd.m_hAdditionalConstantBuffer = shaderInfo.m_hAdditionalConstantBuffer;
    cmd.m_ShaderType = shaderInfo.m_Type;
  }

  void RenderInterface::ReleaseShader(Rml::CompiledShaderHandle hShader)
  {
    ShaderId shaderId = ShaderId::FromRml(hShader);
    if (shaderId.IsInvalidated() == false)
    {
      ShaderInfo shaderInfo;
      W_VERIFY(m_Shaders.Remove(shaderId, &shaderInfo), "Invalid shader handle");

      WGALDevice::GetDefaultDevice()->DestroyBuffer(shaderInfo.m_hAdditionalConstantBuffer);
    }
  }

  void RenderInterface::BeginExtraction(const WHashedString& sName, WGALTextureHandle hTargetTexture)
  {
    m_pCurrentCommandBuffer = AllocateCommandBuffer();
    m_pCurrentCommandBuffer->m_sName = sName;
    m_pCurrentCommandBuffer->m_hTargetTexture = hTargetTexture;

    const WGALTexture* pTargetTexture = WGALDevice::GetDefaultDevice()->GetTexture(hTargetTexture);
    const WUInt32 uiTargetWidth = pTargetTexture->GetDescription().m_uiWidth;
    const WUInt32 uiTargetHeight = pTargetTexture->GetDescription().m_uiHeight;

    m_pCurrentCommandBuffer->m_uiTargetWidth = uiTargetWidth;
    m_pCurrentCommandBuffer->m_uiTargetHeight = uiTargetHeight;

    m_mProjection = WGraphicsUtils::CreateOrthographicProjectionMatrix(0.0f, (float)uiTargetWidth, (float)uiTargetHeight, 0.0f, -1.0f, 1.0f);
    SetTransform(nullptr);
    m_bUseStencilTest = false;
  }

  void RenderInterface::EndExtraction()
  {
    SubmitCommandBuffer(std::move(m_pCurrentCommandBuffer));
  }

  void RenderInterface::GALEventHandler(const WGALDeviceEvent& e)
  {
    if (e.m_Type == WGALDeviceEvent::AfterBeginFrame)
    {
      BeginFrame();
    }
    else if (e.m_Type == WGALDeviceEvent::AfterEndFrame)
    {
      EndFrame();
    }
  }

  void RenderInterface::BeginFrame()
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[WRenderWorld::GetDataIndexForRendering()];
    if (submittedCommandBuffers.IsEmpty())
      return;

    WGALDevice* pDevice = WGALDevice::GetDefaultDevice();

    const WGALResourceFormat::Enum tempTargetFormat = WGALResourceFormat::RGBAUByteNormalized;
    const WGALResourceFormat::Enum tempStencilFormat = WGALResourceFormat::D24S8;
    const WGALMSAASampleCount::Enum msaaSampleCount = WGALMSAASampleCount::FourSamples;

    m_pRenderGraph->Reset();

    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      const WGALTexture* pTargetTexture = pDevice->GetTexture(pCommandBuffer->m_hTargetTexture);
      if (pTargetTexture == nullptr)
      {
        FreeCommandBuffer(std::move(pCommandBuffer));
        continue;
      }

      auto& textureDesc = pTargetTexture->GetDescription();

      WRenderGraphTextureHandle hTarget = m_pRenderGraph->ImportTexture(pCommandBuffer->m_hTargetTexture);
      WGALTextureHandle hTargetTexture = pCommandBuffer->m_hTargetTexture;

      WGALTextureCreationDescription tempColorDesc;
      tempColorDesc.SetAsRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempTargetFormat, msaaSampleCount);
      WRenderGraphTextureHandle hTempColor = m_pRenderGraph->CreateTexture(tempColorDesc);

      WGALTextureCreationDescription tempStencilDesc;
      tempStencilDesc.SetAsRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempStencilFormat, msaaSampleCount);
      WRenderGraphTextureHandle hTempStencil = m_pRenderGraph->CreateTexture(tempStencilDesc);

      {
        auto pass = m_pRenderGraph->AddGraphicsPass(pCommandBuffer->m_sName);
        pass.AddColorTarget(hTempColor, {}, WGALRenderTargetLoadOp::Clear);
        pass.SetClearColor(0);
        pass.AddDepthStencilTarget(hTempStencil, {},
          WGALRenderTargetLoadOp::Clear, WGALRenderTargetStoreOp::Discard,
          WGALRenderTargetLoadOp::Clear, WGALRenderTargetStoreOp::Discard);
        pass.SetClearDepth();
        pass.SetClearStencil();

        pass.SetExecuteCallback([this, pCommandBuffer = pCommandBuffer.Borrow()](const WRenderGraphContext& ctx)
          {
            auto* pCommandEncoder = ctx.GetCommandEncoder();
            auto* pRenderContext = ctx.GetRenderContext();

            bool bAllowAsyncShaderLoading = pRenderContext->GetAllowAsyncShaderLoading();
            pRenderContext->SetAllowAsyncShaderLoading(false);
            W_SCOPE_EXIT(pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

            WRectFloat viewport(static_cast<float>(pCommandBuffer->m_uiTargetWidth), static_cast<float>(pCommandBuffer->m_uiTargetHeight));
            pCommandEncoder->SetViewport(viewport);

            WRectU32 scissorRect(0, 0, pCommandBuffer->m_uiTargetWidth, pCommandBuffer->m_uiTargetHeight);
            pCommandEncoder->SetScissorRect(scissorRect);

            pRenderContext->BindShader(m_hMainShader);
            WBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
            bindGroup.BindBuffer("WRmlUiConstants", m_hMainConstantBuffer);
            bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture);

            auto Draw = [&](const CommandRenderGeometry& cmd, bool bGradient)
            {
              pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", cmd.m_bUseStencilTest ? WTempHashedString("RMLUI_MODE_STENCIL_TEST") : WTempHashedString("RMLUI_MODE_NORMAL"));
              pRenderContext->SetShaderPermutationVariable("RMLUI_GRADIENT", bGradient ? WTempHashedString("TRUE") : WTempHashedString("FALSE"));

              WRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<WRmlUiConstants>(m_hMainConstantBuffer);
              pConstants->UiTransform = cmd.m_Transform;
              pConstants->UiTranslation = cmd.m_Translation;
              pConstants->TextureNeedsAlphaMultiplication = cmd.m_bNeedsPremultipliedAlpha;

              pRenderContext->BindMeshBuffer(WMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, WGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

              bindGroup.BindTexture("BaseTexture", cmd.m_hTexture);

              pRenderContext->DrawMeshBuffer().IgnoreResult();
            };

            WUInt32 uiCommandOffset = 0;
            while (uiCommandOffset < pCommandBuffer->m_Buffer.GetCount())
            {
              CommandType::Enum cmdType = pCommandBuffer->PeekCommandType(uiCommandOffset);
              switch (cmdType)
              {
                case CommandType::RenderGeometry:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderGeometry>(uiCommandOffset);

                  pRenderContext->BindShader(m_hMainShader);
                  Draw(cmd, false);
                }
                break;

                case CommandType::RenderShader:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderShader>(uiCommandOffset);

                  pRenderContext->BindShader(cmd.m_hShader);
                  bindGroup.BindBuffer("WRmlUiAdditionalConstants", cmd.m_hAdditionalConstantBuffer);
                  Draw(cmd, cmd.m_ShaderType == ShaderType::Gradient);
                }
                break;

                case CommandType::SetScissorRegion:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandSetScissorRegion>(uiCommandOffset);

                  pCommandEncoder->SetScissorRect(cmd.m_ScissorRect);
                }
                break;

                case CommandType::RenderToClipMask:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderToClipMask>(uiCommandOffset);

                  W_ASSERT_DEV(cmd.m_Operation == Rml::ClipMaskOperation::Set, "Only 'Set' clip mask operation is implemented.");
                  pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

                  WRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<WRmlUiConstants>(m_hMainConstantBuffer);
                  pConstants->UiTransform = cmd.m_Transform;
                  pConstants->UiTranslation = cmd.m_Translation;

                  pRenderContext->BindMeshBuffer(WMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, WGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

                  pRenderContext->DrawMeshBuffer().IgnoreResult();
                }
                break;

                default:
                {
                  W_ASSERT_ALWAYS(false, "RmlUI: Command Type '{}' is not implemented.", cmdType);
                  break;
                }
              }
            } //
          });
      }

      // Transfer pass: resolve MSAA to target texture
      {
        auto resolvePass = m_pRenderGraph->AddTransferPass("RmlUi Resolve");
        resolvePass.ReadTexture(hTempColor, {}, WGALResourceState::ResolveSource);
        resolvePass.WriteTexture(hTarget, {}, WGALResourceState::ResolveDestination);
        resolvePass.SetExecuteCallback(
          [hTempColor, hTarget](const WRenderGraphContext& ctx)
          {
            ctx.GetCommandEncoder()->ResolveTexture(
              ctx.ResolveTexture(hTarget), WGALTextureSubresource(),
              ctx.ResolveTexture(hTempColor), WGALTextureSubresource());
          });
      }

      // Optional: generate mipmaps for the target
      if (textureDesc.m_uiMipLevelCount > 1 && textureDesc.m_TextureFlags.IsSet(WGALTextureUsageFlags::RenderTarget))
      {
        WRenderGraphUtils::GenerateMipMaps(hTargetTexture, {}, *m_pRenderGraph);
      }
    }

    WRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  }

  void RenderInterface::EndFrame()
  {
    WUInt64 uiFrameCounter = WRenderWorld::GetFrameCounter();

    W_LOCK(m_ReleasedCompiledGeometryMutex);

    while (!m_ReleasedCompiledGeometry.IsEmpty())
    {
      auto& releasedGeometry = m_ReleasedCompiledGeometry.PeekFront();

      if (releasedGeometry.m_uiFrame >= uiFrameCounter)
        break;

      FreeReleasedGeometry(releasedGeometry.m_Id);

      m_CompiledGeometry.Remove(releasedGeometry.m_Id);
      m_ReleasedCompiledGeometry.PopFront();
    }

    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[WRenderWorld::GetDataIndexForRendering()];
    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      if (pCommandBuffer != nullptr)
      {
        FreeCommandBuffer(std::move(pCommandBuffer));
      }
    }
    submittedCommandBuffers.Clear();
  }

  void RenderInterface::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    WGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    WGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();
  }

  void RenderInterface::FillRenderCommand(CommandRenderGeometry& out_cmd, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    W_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry), out_cmd.m_CompiledGeometry), "Invalid compiled geometry");

    TextureInfo textureInfo;
    if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), textureInfo) == false)
    {
      textureInfo.m_hTexture = m_hFallbackTexture;
    }

    WResourceLock<WTexture2DResource> pTexture(textureInfo.m_hTexture, WResourceAcquireMode::BlockTillLoaded);
    out_cmd.m_hTexture = pTexture->GetGALTexture();

    out_cmd.m_Transform = m_mTransform;
    out_cmd.m_Translation = WRmlUiConversionUtils::ToVec2(translation);
    out_cmd.m_bNeedsPremultipliedAlpha = textureInfo.m_bHasPremultipliedAlpha == false;
    out_cmd.m_bUseStencilTest = m_bUseStencilTest;
  }

  WUniquePtr<CommandBuffer> RenderInterface::AllocateCommandBuffer()
  {
    if (m_FreeCommandBuffers.IsEmpty() == false)
    {
      WUniquePtr<CommandBuffer> cmdBuffer = std::move(m_FreeCommandBuffers.PeekBack());
      m_FreeCommandBuffers.PopBack();
      return cmdBuffer;
    }

    return W_DEFAULT_NEW(CommandBuffer);
  }

  void RenderInterface::FreeCommandBuffer(WUniquePtr<CommandBuffer>&& pBuffer)
  {
    pBuffer->Clear();

    m_FreeCommandBuffers.PushBack(std::move(pBuffer));
  }

  void RenderInterface::SubmitCommandBuffer(WUniquePtr<CommandBuffer>&& pBuffer)
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[WRenderWorld::GetDataIndexForExtraction()];
    submittedCommandBuffers.PushBack(std::move(pBuffer));
  }

} // namespace WRmlUiInternal
