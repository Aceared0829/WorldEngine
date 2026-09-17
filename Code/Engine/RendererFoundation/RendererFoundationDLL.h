#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define W_RENDERERFOUNDATION_DLL W_DECL_EXPORT
#  else
#    define W_RENDERERFOUNDATION_DLL W_DECL_IMPORT
#  endif
#else
#  define W_RENDERERFOUNDATION_DLL
#endif

// #TODO_SHADER obsolete, DX11 only
#define W_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define W_GAL_MAX_SAMPLER_COUNT 16

// Necessary array sizes
#define W_GAL_MAX_VERTEX_BUFFER_COUNT 8
#define W_GAL_MAX_VERTEX_ATTRIBUTE_COUNT 16
#define W_GAL_MAX_RENDERTARGET_COUNT 8
#define W_GAL_MAX_BIND_GROUPS 4

#define W_GAL_ALL_MIP_LEVELS 0xFFu
#define W_GAL_ALL_ARRAY_SLICES 0xFFFFu
#define W_GAL_WHOLE_SIZE 0xFFFFFFFFu

#define W_GAL_BIND_GROUP_FRAME 0
#define W_GAL_BIND_GROUP_RENDER_PASS 1
#define W_GAL_BIND_GROUP_MATERIAL 2
#define W_GAL_BIND_GROUP_DRAW_CALL 3

// Forward declarations

struct WGALDeviceCreationDescription;
struct WGALSwapChainCreationDescription;
struct WGALWindowSwapChainCreationDescription;
struct WGALShaderCreationDescription;
struct WGALTextureCreationDescription;
struct WGALBufferCreationDescription;
struct WGALDepthStencilStateCreationDescription;
struct WGALBlendStateCreationDescription;
struct WGALRasterizerStateCreationDescription;
struct WGALVertexDeclarationCreationDescription;
struct WGALSamplerStateCreationDescription;
struct WGALRenderTargetViewCreationDescription;
struct WGALBindGroupLayoutCreationDescription;
struct WGALBindGroupCreationDescription;
struct WGALPipelineLayoutCreationDescription;
struct WGALGraphicsPipelineCreationDescription;
struct WGALComputePipelineCreationDescription;


class WGALSwapChain;
class WGALShader;
class WGALResourceBase;
class WGALTexture;
class WGALSharedTexture;
class WGALBuffer;
class WGALDynamicBuffer;
class WGALReadbackBuffer;
class WGALReadbackTexture;
class WGALDepthStencilState;
class WGALBlendState;
class WGALRasterizerState;
class WGALVertexDeclaration;
class WGALSamplerState;
class WGALRenderTargetView;
class WGALDevice;
class WGALCommandEncoder;
class WGALBindGroup;
class WGALBindGroupLayout;
class WGALPipelineLayout;
class WGALGraphicsPipeline;
class WGALComputePipeline;

// Basic enums
struct WGALPrimitiveTopology
{
  using StorageType = WUInt8;
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in WMeshBufferResourceDescriptor::AllocateStreams
    Points,        // 1 index per primitive
    Lines,         // 2 indices per primitive
    Triangles,     // 3 indices per primitive
    TriangleStrip, // 3 indices per primitive, but the first two indices are shared with the previous primitive

    ENUM_COUNT,

    Default = Triangles
  };

  static WUInt32 GetIndexCount(Enum e, WUInt32 uiPrimitiveCount)
  {
    if (e <= Triangles)
      return uiPrimitiveCount * ((WUInt32)e + 1);

    // TriangleStrip
    return uiPrimitiveCount > 0 ? uiPrimitiveCount + 2 : 0;
  }
};

struct W_RENDERERFOUNDATION_DLL WGALIndexType
{
  using StorageType = WUInt8;

  enum Enum
  {
    None,   // indices are not used, vertices are just used in order to form primitives
    UShort, // 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices
    UInt,   // 32 bit indices are used to select which vertices shall form a primitive

    ENUM_COUNT,

    Default = None,
  };


  /// The size in bytes of a single element of the given index format.
  static WUInt8 GetSize(WGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const WUInt8 s_Size[WGALIndexType::ENUM_COUNT];
};

/// The stage of a shader. A complete shader can consist of multiple stages.
/// \sa WGALShaderStageFlags, WGALShaderCreationDescription
struct W_RENDERERFOUNDATION_DLL WGALShaderStage
{
  using StorageType = WUInt8;

  enum Enum : WUInt8
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,
    ComputeShader,
    /*
    // #TODO_SHADER: Future work:
    TaskShader,
    MeshShader,
    RayGenShader,
    RayAnyHitShader,
    RayClosestHitShader,
    RayMissShader,
    RayIntersectionShader,
    */
    ENUM_COUNT,
    Default = VertexShader
  };

  static const char* Names[ENUM_COUNT];
};

/// A set of shader stages.
/// \sa WGALShaderStage, WShaderResourceBinding
struct W_RENDERERFOUNDATION_DLL WGALShaderStageFlags
{
  using StorageType = WUInt16;

  enum Enum : WUInt16
  {
    VertexShader = W_BIT(0),
    HullShader = W_BIT(1),
    DomainShader = W_BIT(2),
    GeometryShader = W_BIT(3),
    PixelShader = W_BIT(4),
    ComputeShader = W_BIT(5),
    /*
    // #TODO_SHADER: Future work:
    TaskShader = W_BIT(6),
    MeshShader = W_BIT(7),
    RayGenShader = W_BIT(8),
    RayAnyHitShader = W_BIT(9),
    RayClosestHitShader = W_BIT(10),
    RayMissShader = W_BIT(11),
    RayIntersectionShader = W_BIT(12),
    */
    Auto = W_BIT(15), ///< Used by the render graph to infer the stage from the WGALResourceState
    Default = 0
  };

  struct Bits
  {
    StorageType VertexShader : 1;
    StorageType HullShader : 1;
    StorageType DomainShader : 1;
    StorageType GeometryShader : 1;
    StorageType PixelShader : 1;
    StorageType ComputeShader : 1;

    StorageType _padding : 9;
    StorageType Auto : 1;
  };

  inline static WGALShaderStageFlags::Enum MakeFromShaderStage(WGALShaderStage::Enum stage)
  {
    return static_cast<WGALShaderStageFlags::Enum>(W_BIT(stage));
  }
};
W_DECLARE_FLAGS_OPERATORS(WGALShaderStageFlags);


struct W_RENDERERFOUNDATION_DLL WGALMSAASampleCount
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 1,
    TwoSamples = 2,
    FourSamples = 4,
    EightSamples = 8,

    ENUM_COUNT = 4,

    Default = None
  };
};

struct WGALTextureType
{
  using StorageType = WUInt8;

  enum Enum
  {
    Invalid = 255,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,
    Texture2DShared,
    Texture2DArray,
    TextureCubeArray,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct WGALBlend
{
  using StorageType = WUInt8;

  enum Enum
  {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSaturated,
    BlendFactor,
    InvBlendFactor,

    ENUM_COUNT,

    Default = One
  };
};

struct WGALBlendOp
{
  using StorageType = WUInt8;

  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT,
    Default = Add
  };
};

struct WGALStencilOp
{
  using StorageType = WUInt8;

  enum Enum
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementSaturated,
    DecrementSaturated,
    Invert,
    Increment,
    Decrement,

    ENUM_COUNT,

    Default = Keep
  };
};

struct WGALCompareFunc
{
  using StorageType = WUInt8;

  enum Enum
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    ENUM_COUNT,

    Default = Never
  };
};

/// Defines which sides of a polygon gets culled by the graphics card
struct WGALCullMode
{
  using StorageType = WUInt8;

  /// Defines which sides of a polygon gets culled by the graphics card
  enum Enum
  {
    None = 0,  ///< Triangles do not get culled
    Front = 1, ///< When the 'front' of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< WGALRasterizerStateCreationDescription for details.
    Back = 2,  ///< When the 'back'  of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< WGALRasterizerStateCreationDescription for details.

    ENUM_COUNT,

    Default = Back
  };
};

struct WGALTextureFilterMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Point = 0,
    Linear,
    Anisotropic,

    Default = Linear
  };
};

/// Defines global texture filtering quality levels.
///
/// Used to scale all quality-adjustable samplers simultaneously. The actual quality applied to each sampler
/// depends on which quality mode slot it uses and what quality level is mapped to that slot.
/// \see WGALDevice::SetTextureQualityMode, WRenderContext::SetDefaultTextureQuality
struct WGALTextureQuality
{
  using StorageType = WUInt8;

  enum Enum
  {
    Nearest,
    Bilinear,
    Trilinear,
    Anisotropic2x,
    Anisotropic4x,
    Anisotropic8x,
    Anisotropic16x,

    Default = Nearest,
  };
};

/// Identifies one of the five abstract quality tiers used by the texture quality system.
///
/// Samplers that opt in via WGALSamplerStateCreationDescription::m_useTextureQualitySlot reference one of these slots.
/// Each slot is assigned an WGALTextureQuality value through WGALDevice::SetTextureQualityMode, and the device maps
/// the requested tier to a concrete filter based on the current global quality setting.
struct WGALTextureQualitySlot
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    LowestQuality,  ///< Lowest quality tier, typically used for distant or unimportant textures.
    LowQuality,     ///< Below-default quality tier.
    DefaultQuality, ///< Standard quality tier used for most textures.
    HighQuality,    ///< Above-default quality tier.
    HighestQuality, ///< Highest quality tier, typically reserved for hero assets or UI.

    None = 0xFF,    ///< Sentinel value: sampler uses its own fixed filter settings instead of a quality slot.

    Default = DefaultQuality
  };
};

struct WGALUpdateMode
{
  enum Enum
  {
    TransientConstantBuffer, ///< Can be executed at any time in a command encoder. Buffer must be completely overwritten. Data will not persist across frames. Only allowed on transient constant buffers.
    AheadOfTime,             ///< Can be executed at any time in a command encoder. Copy is ensured to happen before the next command in the command encoder. The same memory location can't be updated twice in one frame. Note that no GPU access must have happened to the modified memory range in the current command encoder before this call or undefined behavior will occur.
  };
};

/// Used by WGALVertexDeclarationCreationDescription -> WGALVertexBinding to define whether the data in a vertex buffer is indexed via vertex or instance index.
struct WGALVertexBindingRate
{
  using StorageType = WUInt8;
  enum Enum
  {
    Vertex,
    Instance,
    Default = Vertex,
  };
};

/// The initial state of a render target when starting to render to it.
struct WGALRenderTargetLoadOp
{
  using StorageType = WUInt8;
  enum Enum
  {
    Load,     ///< The previous contents of the render target are preserved when starting to render to it.
    Clear,    ///< The render target is cleared before rendering.
    DontCare, ///< The contents of the render target is undefined. Use if you intent to render to the entirety of the viewport.
    Default = Load
  };
};

/// The state of a render target after finishing to render to it.
struct WGALRenderTargetStoreOp
{
  using StorageType = WUInt8;
  enum Enum
  {
    Store,   ///< The render result is written back to the render target's memory.
    Discard, ///< The end result is not needed. Use for transient render targets.
    Default = Store
  };
};

/// The current state of an async operations in the renderer
struct WGALAsyncResult
{
  using StorageType = WUInt8;

  enum Enum
  {
    Ready,   ///< The async operation has finished and the result is ready.
    Pending, ///< The async operation is still running, retry later.
    Expired, ///< The async operation is too old and the result was thrown away. Pending results should be queried every frame until they are ready.
    Default = Expired
  };
};

/// Used to define a texture sub-resource, i.e. a single slice.
struct WGALTextureSubresource
{
  W_DECLARE_POD_TYPE();
  WUInt32 m_uiMipLevel = 0;
  WUInt32 m_uiArraySlice = 0;
};

/// Helper to map linear system memory to a 2D texture sub-resource.
struct WGALSystemMemoryDescription
{
  W_DECLARE_POD_TYPE();
  WConstByteBlobPtr m_pData;
  WUInt32 m_uiRowPitch = 0;
  WUInt32 m_uiSlicePitch = 0;
};

/// Defines the sub-resources a render target view is rendering to.
/// Used by the render graph to define a render target. Views can't be used as the render graph works on virtual handles that only later are converted to actual resources.
struct WGALRenderTargetRange
{
  W_DECLARE_POD_TYPE();
  static WGALRenderTargetRange MakeFromMipLevel(WUInt8 uiMipLevel = 0)
  {
    return {0, W_GAL_ALL_ARRAY_SLICES, uiMipLevel};
  }
  WUInt16 m_uiBaseArraySlice = 0;
  WUInt16 m_uiArraySlices = W_GAL_ALL_ARRAY_SLICES;
  WUInt8 m_uiBaseMipLevel = 0;
};

/// Defines a sub-set of a texture that can be bound in a shader. Default constructed means entire texture.
/// Mainly used in WBindGroupBuilder::BindTexture calls to map resources to shader bindings and other binding related methods.
struct WGALTextureRange
{
  W_DECLARE_POD_TYPE();
  /// Helper to just set mip levels without also having to set the array slice fields.
  static WGALTextureRange MakeFromMipRange(WUInt8 uiBaseMipLevel = 0, WUInt8 uiMipLevels = W_GAL_ALL_MIP_LEVELS)
  {
    return {0, 1, uiBaseMipLevel, uiMipLevels};
  }
  static WGALTextureRange MakeFromRenderTargetRange(const WGALRenderTargetRange& range)
  {
    return {range.m_uiBaseArraySlice, range.m_uiArraySlices, range.m_uiBaseMipLevel, 1};
  }
  WUInt16 m_uiBaseArraySlice = 0;                    ///< Index of the first array slice to be used.
  WUInt16 m_uiArraySlices = W_GAL_ALL_ARRAY_SLICES; ///< Number of array slices to be used. If set to W_GAL_ALL_ARRAY_SLICES, the maximum number of allowed slices is used dependent on texture size and binding contraints.
  WUInt8 m_uiBaseMipLevel = 0;                       ///< The first mip level to be used.
  WUInt8 m_uiMipLevels = W_GAL_ALL_MIP_LEVELS;      ///< Number of mip levels to be used. Ignored for UAVs. If set to W_GAL_ALL_MIP_LEVELS, the maximum number of allowed mip maps is used dependent on texture size.

  bool operator==(const WGALTextureRange& rhs) const
  {
    return m_uiBaseArraySlice == rhs.m_uiBaseArraySlice &&
           m_uiArraySlices == rhs.m_uiArraySlices &&
           m_uiBaseMipLevel == rhs.m_uiBaseMipLevel &&
           m_uiMipLevels == rhs.m_uiMipLevels;
  }

  bool operator!=(const WGALTextureRange& rhs) const { return !(*this == rhs); }

  /// Returns true if this range and the other range overlap in both the array-slice and mip-level dimensions.
  bool Overlaps(const WGALTextureRange& other) const
  {
    // Cast to WUInt32 to avoid overflow when base + count exceeds the WUInt16/WUInt8 range.
    const WUInt32 aSliceEnd = (WUInt32)m_uiBaseArraySlice + (WUInt32)m_uiArraySlices;
    const WUInt32 bSliceEnd = (WUInt32)other.m_uiBaseArraySlice + (WUInt32)other.m_uiArraySlices;
    if ((WUInt32)m_uiBaseArraySlice >= bSliceEnd || (WUInt32)other.m_uiBaseArraySlice >= aSliceEnd)
      return false;

    const WUInt32 aMipEnd = (WUInt32)m_uiBaseMipLevel + (WUInt32)m_uiMipLevels;
    const WUInt32 bMipEnd = (WUInt32)other.m_uiBaseMipLevel + (WUInt32)other.m_uiMipLevels;
    if ((WUInt32)m_uiBaseMipLevel >= bMipEnd || (WUInt32)other.m_uiBaseMipLevel >= aMipEnd)
      return false;

    return true;
  }

  /// Computes a flat sub-resource index for the given mip level and array layer within a texture whose full range is described by this instance.
  /// Index = uiMipLevel + uiLayer * m_uiMipLevels.
  W_ALWAYS_INLINE static WUInt32 ComputeSubResourceIndex(WUInt32 uiMipLevel, WUInt32 uiLayer, const WGALTextureRange& fullRange)
  {
    return uiMipLevel + uiLayer * fullRange.m_uiMipLevels;
  }
};

/// Defines a sub-set of a buffer that can be bound in a shader. Default constructed means entire buffer.
/// Mainly used in WBindGroupBuilder::BindBuffer calls to map resources to shader bindings and other binding related methods.
struct WGALBufferRange
{
  W_DECLARE_POD_TYPE();
  WUInt32 m_uiByteOffset = 0;                ///< Start of the view to the buffer. Must be multiple of the element size.
  WUInt32 m_uiByteCount = W_GAL_WHOLE_SIZE; ///< m_uiByteOffset + m_uiByteCount must be less than the size of the buffer, unless W_GAL_WHOLE_SIZE ist used, which maps to the rest of the buffer.
};

/// Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class WGALObject : public WRefCounted
{
public:
  WGALObject(const CreationDescription& description)
    : m_Description(description)
  {
  }

  W_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace WGAL
{
  using ez16_16Id = WGenericId<16, 16>;
  using ez18_14Id = WGenericId<18, 14>;
  using ez20_12Id = WGenericId<20, 12>;
  using ez20_44Id = WGenericId<20, 44>;
} // namespace WGAL

class WGALSwapChainHandle
{
  W_DECLARE_HANDLE_TYPE(WGALSwapChainHandle, WGAL::ez16_16Id);

  friend class WGALDevice;
};

class WGALShaderHandle
{
  W_DECLARE_HANDLE_TYPE(WGALShaderHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALTextureHandle
{
  W_DECLARE_HANDLE_TYPE(WGALTextureHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALReadbackTextureHandle
{
  W_DECLARE_HANDLE_TYPE(WGALReadbackTextureHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALBufferHandle
{
  W_DECLARE_HANDLE_TYPE(WGALBufferHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALDynamicBufferHandle
{
  W_DECLARE_HANDLE_TYPE(WGALDynamicBufferHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALReadbackBufferHandle
{
  W_DECLARE_HANDLE_TYPE(WGALReadbackBufferHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALRenderTargetViewHandle
{
  W_DECLARE_HANDLE_TYPE(WGALRenderTargetViewHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALDepthStencilStateHandle
{
  W_DECLARE_HANDLE_TYPE(WGALDepthStencilStateHandle, WGAL::ez16_16Id);

  friend class WGALDevice;
};

class WGALBlendStateHandle
{
  W_DECLARE_HANDLE_TYPE(WGALBlendStateHandle, WGAL::ez16_16Id);

  friend class WGALDevice;
};

class WGALRasterizerStateHandle
{
  W_DECLARE_HANDLE_TYPE(WGALRasterizerStateHandle, WGAL::ez16_16Id);

  friend class WGALDevice;
};

class WGALSamplerStateHandle
{
  W_DECLARE_HANDLE_TYPE(WGALSamplerStateHandle, WGAL::ez16_16Id);

  friend class WGALDevice;
};

class WGALVertexDeclarationHandle
{
  W_DECLARE_HANDLE_TYPE(WGALVertexDeclarationHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

/// Handle to WGALBindGroupLayout, created via WGALDevice::CreateBindGroupLayout
class WGALBindGroupLayoutHandle
{
  W_DECLARE_HANDLE_TYPE(WGALBindGroupLayoutHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

/// Handle to WGALBindGroup, created via WGALDevice::CreateBindGroup
class WGALBindGroupHandle
{
  W_DECLARE_HANDLE_TYPE(WGALBindGroupHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

/// Handle to WGALPipelineLayout, created via WGALDevice::CreatePipelineLayout
class WGALPipelineLayoutHandle
{
  W_DECLARE_HANDLE_TYPE(WGALPipelineLayoutHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALGraphicsPipelineHandle
{
  W_DECLARE_HANDLE_TYPE(WGALGraphicsPipelineHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

class WGALComputePipelineHandle
{
  W_DECLARE_HANDLE_TYPE(WGALComputePipelineHandle, WGAL::ez18_14Id);

  friend class WGALDevice;
};

using WGALPoolHandle = WGAL::ez20_44Id;
using WGALTimestampHandle = WGALPoolHandle;
using WGALOcclusionHandle = WGALPoolHandle;
using WGALFenceHandle = WUInt64;

namespace WGAL
{
  struct ModifiedRange
  {
    W_ALWAYS_INLINE void Reset()
    {
      m_uiMin = WInvalidIndex;
      m_uiMax = 0;
    }

    W_FORCE_INLINE void SetToIncludeValue(WUInt32 value)
    {
      m_uiMin = WMath::Min(m_uiMin, value);
      m_uiMax = WMath::Max(m_uiMax, value);
    }

    W_FORCE_INLINE void SetToIncludeRange(WUInt32 uiMin, WUInt32 uiMax)
    {
      m_uiMin = WMath::Min(m_uiMin, uiMin);
      m_uiMax = WMath::Max(m_uiMax, uiMax);
    }

    W_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    W_ALWAYS_INLINE WUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    WUInt32 m_uiMin = WInvalidIndex;
    WUInt32 m_uiMax = 0;
  };
} // namespace WGAL
