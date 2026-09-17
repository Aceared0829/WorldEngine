#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// The type of a shader resource (WShaderResourceBinding).
/// Shader resources need to be bound to a shader for it to function. This includes samplers, constant buffers, textures etc. which are all handled by W via GAL resource types / views. However, vertex buffers, index buffers and vertex layouts are not considered shader resources and are handled separately.
/// \sa WGALShaderTextureType, WShaderResourceBinding
struct WGALShaderResourceType
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Unknown = 0,
    /// Texture sampler (WGALSamplerStateHandle). HLSL: SamplerState, SamplerComparisonState
    Sampler = 1,

    /// Read-only struct (WGALBufferHandle). HLSL: cbuffer, ConstantBuffer
    ConstantBuffer = 2,
    // Read-only struct. Set directly via WGALCommandEncoder::SetPushConstants. HLSL: Use macro BEGIN_PUSH_CONSTANTS, END_PUSH_CONSTANTS, GET_PUSH_CONSTANT
    PushConstants = 3,

    /// \name Shader Resource Views (SRVs).
    ///@{

    /// Read-only texture view. When set, WGALShaderTextureType is also set. HLSL: Texture*
    Texture = 4,
    /// Read-only texture view with attached sampler. When set, WGALShaderTextureType is also set. HLSL: Name sampler the same as texture with _AutoSampler appended.
    TextureAndSampler = 5,
    /// Read-only texel buffer. It's like a 1D texture. HLSL: Buffer
    TexelBuffer = 6,
    /// Read-only array of structs. HLSL: StructuredBuffer<T>
    StructuredBuffer = 7,
    /// Read-only array of bytes. HLSL: ByteAddressBuffer
    ByteAddressBuffer = 11,

    ///@}
    /// \name Unordered Access Views (UAVs).
    ///@{

    /// Read-write texture view. When set, WGALShaderTextureType is also set. HLSL: RWTexture*
    TextureRW = 8,
    /// Read-write texel buffer. It's like a 1D texture. HLSL: RWBuffer
    TexelBufferRW = 9,
    /// Read-write array of structs. HLSL: RWStructuredBuffer<T>, AppendStructuredBuffer, ConsumeStructuredBuffer
    StructuredBufferRW = 10,
    /// Read-write array of bytes. HLSL: RWByteAddressBuffer
    ByteAddressBufferRW = 12,

    ///@}

    // #TODO_SHADER: Future work:
    // Not supported: W does not support AppendStructuredBuffer, ConsumeStructuredBuffer yet so while the shader can be compiled, nothing can be bound to these resources. On Vulkan, will probably need yet another type to distinguish the data from the count resource (uav_counter_binding).
    // Not supported: tbuffer, TextureBuffer, these map to CBV on DX11 and to eStorageBuffer on Vulkan, requiring to use a constantBufferView or a UAV. Thus, it bleeds platform implementation details.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, frame-buffer local read-only image view. Required for render passes on mobile.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,Vulkan 1.3 addition, surpasses push-constants but not widely supported yet. May be able to abstract this via PushConstants and custom shader compiler / GAL implementations.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, Vulkan extension for raytracing.
    COUNT = ByteAddressBufferRW + 1,
    Default = Unknown
  };
};

/// General category of the shader resource (WShaderResourceBinding).
/// Note that these are flags because some resources can be multiple resource types, e.g. WGALShaderResourceType::TextureAndSampler.
struct WGALShaderResourceCategory
{
  using StorageType = WUInt8;
  static constexpr int ENUM_COUNT = 4;
  enum Enum : WUInt8
  {
    Sampler = W_BIT(0),        //< Sampler (WGALSamplerStateHandle).
    ConstantBuffer = W_BIT(1), //< Constant Buffer (WGALBufferHandle)
    TextureSRV = W_BIT(2),     //< Shader Resource Views
    BufferSRV = W_BIT(3),      //< Shader Resource Views
    TextureUAV = W_BIT(4),     //< Unordered Access Views
    BufferUAV = W_BIT(5),      //< Unordered Access Views
    Default = 0
  };

  struct Bits
  {
    StorageType Sampler : 1;
    StorageType ConstantBuffer : 1;
    StorageType TextureSRV : 1;
    StorageType BufferSRV : 1;
    StorageType TextureUAV : 1;
    StorageType BufferUAV : 1;
  };

  static WBitflags<WGALShaderResourceCategory> MakeFromShaderDescriptorType(WGALShaderResourceType::Enum type);
};

W_DECLARE_FLAGS_OPERATORS(WGALShaderResourceCategory);

/// The texture type of the shader resource (WShaderResourceBinding).
struct WGALShaderTextureType
{
  using StorageType = WUInt8;
  enum Enum : WUInt8
  {
    Unknown = 0,
    Texture1D = 1,
    Texture1DArray = 2,
    Texture2D = 3,
    Texture2DArray = 4,
    Texture2DMS = 5,
    Texture2DMSArray = 6,
    Texture3D = 7,
    TextureCube = 8,
    TextureCubeArray = 9,

    Default = Unknown
  };

  static bool IsArray(WGALShaderTextureType::Enum format);
  static bool IsMSAA(WGALShaderTextureType::Enum format);
  static WGALTextureType::Enum GetTextureType(WGALShaderTextureType::Enum format);
};

/// Defines a swap chain's present mode.
/// \sa WGALWindowSwapChainCreationDescription
struct WGALPresentMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Immediate,
    VSync,
    ENUM_COUNT,
    Default = VSync
  };
};

/// Defines the usage semantic of a vertex attribute.
/// \sa WGALVertexAttribute
struct WGALVertexAttributeSemantic
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Position,
    Normal,
    Tangent,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent, // Not commonly used

    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    DataOffsets,

    ENUM_COUNT,
    Default = Position
  };
};

/// Defines for what purpose a texture can be used for.
/// \sa WGALTextureCreationDescription
struct WGALTextureUsageFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    ShaderResource = W_BIT(0),  ///< Can be used for WGALShaderResourceType in the SRV section.
    UnorderedAccess = W_BIT(1), ///< Can be used for WGALShaderResourceType in the UAV section.
    RenderTarget = W_BIT(2),    ///< Can be used as a render target or depth-stencil target.
    Presentable = W_BIT(4),     ///< Can be presented by a swapchain

    Default = ShaderResource
  };

  struct Bits
  {
    StorageType ShaderResource : 1;
    StorageType UnorderedAccess : 1;
    StorageType RenderTarget : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WGALTextureUsageFlags);

/// Defines for what purpose a buffer can be used for.
/// \sa WGALBufferCreationDescription
struct WGALBufferUsageFlags
{
  using StorageType = WUInt16;

  enum Enum
  {
    VertexBuffer = W_BIT(0),      ///< Can be used as a vertex buffer.
    IndexBuffer = W_BIT(1),       ///< Can be used as an index buffer.
    ConstantBuffer = W_BIT(2),    ///< Can be used as a constant buffer. Can't be combined with any of the other *Buffer flags.
    TexelBuffer = W_BIT(3),       ///< Can be used as a texel buffer.
    StructuredBuffer = W_BIT(4),  ///< WGALShaderResourceType::StructuredBuffer
    ByteAddressBuffer = W_BIT(5), ///< WGALShaderResourceType::ByteAddressBuffer (RAW)

    ShaderResource = W_BIT(6),    ///< Can be used for WGALShaderResourceType in the SRV section.
    UnorderedAccess = W_BIT(7),   ///< Can be used for WGALShaderResourceType in the UAV section.
    DrawIndirect = W_BIT(8),      ///< Can be used in an indirect draw call.
    Transient = W_BIT(9),         ///< Does not persist across frames. If ConstantBuffer is also set, it can be updated multiple times per frame in the middle of any operation as new memory is created on every update call.

    Default = 0
  };

  struct Bits
  {
    StorageType VertexBuffer : 1;
    StorageType IndexBuffer : 1;
    StorageType ConstantBuffer : 1;
    StorageType TexelBuffer : 1;
    StorageType StructuredBuffer : 1;
    StorageType ByteAddressBuffer : 1;
    StorageType ShaderResource : 1;
    StorageType UnorderedAccess : 1;
    StorageType DrawIndirect : 1;
    StorageType Transient : 1;
  };
};
W_DECLARE_FLAGS_OPERATORS(WGALBufferUsageFlags);

/// Type of GPU->CPU query.
struct WGALQueryType
{
  using StorageType = WUInt8;

  enum Enum
  {
    /// Number of samples that passed the depth and stencil test between begin and end (on a context).
    NumSamplesPassed,
    /// Boolean version of NumSamplesPassed. Any number bigger than 0 equals true.
    AnySamplesPassed,

    Default = NumSamplesPassed
  };
};

/// Type of the shared texture (INTERNAL)
struct WGALSharedTextureType
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    None,     ///< Not shared
    Exported, ///< Allocation owned by this process
    Imported, ///< Allocation owned by a different process
    Default = None
  };
};

/// Defines on what type of queue a render pass is being executed.
/// Note that this does not mean a dedicated queue for this kind of task is used, it merely limits what kind of commands a pass is allowed to execute.
struct WGALQueueType
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    Graphics,
    Compute,
    Transfer,
    Default = Graphics
  };
};

/// Describes the usage state of a GPU resource for barrier and layout transition purposes.
///
/// Read-only states can be combined via bitwise OR when a resource is used for multiple read purposes simultaneously (buffers ONLY, e.g., sampled in a shader while also bound as a vertex buffer).
/// Write states are exclusive and must not be combined with other states.
///
/// Not every state is valid for every resource type. Using an invalid state (e.g., RenderTarget on a buffer) is a usage error.
struct WGALResourceState
{
  using StorageType = WUInt32;

  enum Enum : WUInt32
  {
    Unknown = 0,

    // Read-only states (combinable for buffers)
    ShaderResource = W_BIT(0),   ///< Texture/buffer read by a shader stage
    ConstantBuffer = W_BIT(1),   ///< Bound as a constant/uniform buffer
    VertexBuffer = W_BIT(2),     ///< Bound as a vertex buffer
    IndexBuffer = W_BIT(3),      ///< Bound as an index buffer
    DrawIndirect = W_BIT(4),     ///< Indirect draw/dispatch argument buffer
    DepthStencilRead = W_BIT(5), ///< Depth/stencil attachment without writes
    CopySource = W_BIT(6),       ///< Source of a copy or blit operation
    ResolveSource = W_BIT(7),    ///< Source of an MSAA resolve

    // Write states (exclusive)
    UnorderedAccess = W_BIT(16),    ///< UAV/storage write
    RenderTarget = W_BIT(17),       ///< Color render target attachment
    DepthStencilWrite = W_BIT(18),  ///< Depth/stencil attachment with writes (implies read)
    CopyDestination = W_BIT(19),    ///< Destination of a copy or blit operation
    ResolveDestination = W_BIT(20), ///< Destination of an MSAA resolve

    // Barrier hints
    Discard = W_BIT(28), ///< Textures write only: The previous data / layout can be discarded

    // Special - handled internally
    Present = W_BIT(29),  ///< Swapchain presentation
    CpuRead = W_BIT(30),  ///< GPU->CPU readback (e.g., into a staging buffer)
    CpuWrite = W_BIT(31), ///< CPU->GPU upload (e.g., from a staging buffer)

    // Masks
    AllTextureStates = ShaderResource | DepthStencilRead | CopySource | ResolveSource | UnorderedAccess | RenderTarget | DepthStencilWrite | CopyDestination | ResolveDestination | Discard,

    AllBufferStates = ShaderResource | ConstantBuffer | VertexBuffer | IndexBuffer | DrawIndirect | CopySource | UnorderedAccess | CopyDestination,

    AllReadStates = ShaderResource | ConstantBuffer | VertexBuffer | IndexBuffer | DrawIndirect | DepthStencilRead | CopySource | ResolveSource,

    AllWriteStates = UnorderedAccess | RenderTarget | DepthStencilWrite | CopyDestination | ResolveDestination | Discard,

    Default = Unknown
  };

  struct Bits
  {
    // Read states (bits 0-7)
    StorageType ShaderResource : 1;   // 0
    StorageType ConstantBuffer : 1;   // 1
    StorageType VertexBuffer : 1;     // 2
    StorageType IndexBuffer : 1;      // 3
    StorageType IndirectArgument : 1; // 4
    StorageType DepthStencilRead : 1; // 5
    StorageType CopySource : 1;       // 6
    StorageType ResolveSource : 1;    // 7

    StorageType _Padding0 : 8;        // 8-15

    // Write states (bits 16-20)
    StorageType UnorderedAccess : 1;    // 16
    StorageType RenderTarget : 1;       // 17
    StorageType DepthStencilWrite : 1;  // 18
    StorageType CopyDestination : 1;    // 19
    StorageType ResolveDestination : 1; // 20

    StorageType _Padding1 : 7;          // 21-27

    StorageType Discard : 1;            // 28

    // Special (bits 29-31)
    StorageType Present : 1;  // 29
    StorageType CpuRead : 1;  // 30
    StorageType CpuWrite : 1; // 31
  };
};
W_DECLARE_FLAGS_OPERATORS(WGALResourceState);

/// Describes a texture barrier for a layout/state transition.
struct WGALTextureBarrier
{
  W_DECLARE_POD_TYPE();
  WGALTextureHandle m_hTexture;
  WBitflags<WGALResourceState> m_StateBefore;
  WBitflags<WGALResourceState> m_StateAfter;
  WBitflags<WGALShaderStageFlags> m_StagesBefore = WGALShaderStageFlags::Auto;
  WBitflags<WGALShaderStageFlags> m_StagesAfter = WGALShaderStageFlags::Auto;
  WGALTextureSubresource m_Subresource = {};
  bool m_bAllSubresources = true; ///< If true, the barrier applies to all subresources and m_Subresource is ignored.
  bool m_bDiscard = false;        ///< Discard previous texture layout
};

/// Describes a buffer barrier for a state transition.
struct WGALBufferBarrier
{
  W_DECLARE_POD_TYPE();
  WGALBufferHandle m_hBuffer;
  WBitflags<WGALResourceState> m_StateBefore;
  WBitflags<WGALResourceState> m_StateAfter;
  WBitflags<WGALShaderStageFlags> m_StagesBefore = WGALShaderStageFlags::Auto;
  WBitflags<WGALShaderStageFlags> m_StagesAfter = WGALShaderStageFlags::Auto;
};

#include <RendererFoundation/Descriptors/Implementation/Enumerations_inl.h>
