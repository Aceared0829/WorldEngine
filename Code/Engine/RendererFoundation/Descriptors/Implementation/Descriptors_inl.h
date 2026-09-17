

inline WGALShaderCreationDescription::WGALShaderCreationDescription()
  : WHashableStruct()
{
}

inline WGALShaderCreationDescription::WGALShaderCreationDescription(const WGALShaderCreationDescription& other)
{
  for (WUInt32 i = 0; i < WGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = other.m_ByteCodes[i];
  }
}

inline WGALShaderCreationDescription::~WGALShaderCreationDescription()
{
  for (WUInt32 i = 0; i < WGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = nullptr;
  }
}

inline void WGALShaderCreationDescription::operator=(const WGALShaderCreationDescription& other)
{
  for (WUInt32 i = 0; i < WGALShaderStage::ENUM_COUNT; ++i)
  {
    m_ByteCodes[i] = other.m_ByteCodes[i];
  }
}

inline bool WGALShaderCreationDescription::HasByteCodeForStage(WGALShaderStage::Enum stage) const
{
  return m_ByteCodes[stage] != nullptr && m_ByteCodes[stage]->IsValid();
}

inline void WGALTextureCreationDescription::SetAsRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WGALResourceFormat::Enum format, WGALMSAASampleCount::Enum sampleCount)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_uiDepth = 1;
  m_uiMipLevelCount = 1;
  m_uiArraySize = 1;
  m_SampleCount = sampleCount;
  m_Format = format;
  m_Type = WGALTextureType::Texture2D;
  m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::RenderTarget;
  m_ResourceAccess.m_bImmutable = false;
  m_pExisitingNativeObject = nullptr;
}

inline void WGALTextureCreationDescription::SetAsRenderTarget(WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiArraySize, WGALResourceFormat::Enum format, WGALMSAASampleCount::Enum sampleCount)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_uiDepth = 1;
  m_uiMipLevelCount = 1;
  m_uiArraySize = uiArraySize;
  m_SampleCount = sampleCount;
  m_Format = format;
  m_Type = uiArraySize == 1 ? WGALTextureType::Texture2D : WGALTextureType::Texture2DArray;
  m_TextureFlags = WGALTextureUsageFlags::ShaderResource | WGALTextureUsageFlags::RenderTarget;
  m_ResourceAccess.m_bImmutable = false;
  m_pExisitingNativeObject = nullptr;
}

W_ALWAYS_INLINE constexpr WGALVertexAttribute::WGALVertexAttribute(WGALVertexAttributeSemantic::Enum semantic, WGALResourceFormat::Enum format, WUInt8 uiOffset, WUInt8 uiVertexBufferSlot)
  : m_eSemantic(semantic)
  , m_eFormat(format)
  , m_uiOffset(uiOffset)
  , m_uiVertexBufferSlot(uiVertexBufferSlot)
{
}
