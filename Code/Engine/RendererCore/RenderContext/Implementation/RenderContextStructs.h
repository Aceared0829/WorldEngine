#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Declarations.h>

//////////////////////////////////////////////////////////////////////////
// WShaderBindFlags
//////////////////////////////////////////////////////////////////////////

struct W_RENDERERCORE_DLL WShaderBindFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,                ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind = W_BIT(0), ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was
                             ///< previously used with custom bound states
    NoRasterizerState =
      W_BIT(1),             ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer
    NoDepthStencilState = W_BIT(
      2),                    ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil
    NoBlendState =
      W_BIT(3),             ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend
    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WShaderBindFlags);

//////////////////////////////////////////////////////////////////////////
// WRenderContextFlags
//////////////////////////////////////////////////////////////////////////

struct W_RENDERERCORE_DLL WRenderContextFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = W_BIT(0),
    BindGroupChanged = W_BIT(1),
    BindGroupLayoutChanged = W_BIT(2),
    MeshBufferBindingChanged = W_BIT(3),
    MaterialBindingChanged = W_BIT(4),
    PipelineChanged = W_BIT(5),
    NonPipelineStateChanged = W_BIT(6),

    AllStatesInvalid = ShaderStateChanged | BindGroupChanged | BindGroupLayoutChanged | MeshBufferBindingChanged | PipelineChanged | NonPipelineStateChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType BindGroupChanged : 1;
    StorageType BindGroupLayoutChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType MaterialBindingChanged : 1;
    StorageType PipelineChanged : 1;
    StorageType NonPipelineStateChanged : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WRenderContextFlags);

//////////////////////////////////////////////////////////////////////////
// WDefaultSamplerFlags
//////////////////////////////////////////////////////////////////////////

struct W_RENDERERCORE_DLL WDefaultSamplerFlags
{
  using StorageType = WUInt32;

  enum Enum
  {
    PointFiltering = 0,
    LinearFiltering = W_BIT(0),

    Wrap = 0,
    Clamp = W_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WDefaultSamplerFlags);
