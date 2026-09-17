
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// The reflection data of a constant in a shader constant buffer.
/// \sa WShaderConstantBufferLayout
struct W_RENDERERFOUNDATION_DLL WShaderConstant
{
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  struct Type
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Default,
      Float1,
      Float2,
      Float3,
      Float4,
      Int1,
      Int2,
      Int3,
      Int4,
      UInt1,
      UInt2,
      UInt3,
      UInt4,
      Mat3x3,
      Mat4x4,
      Transform,
      Bool,
      Struct,
      ENUM_COUNT
    };
  };

  static WUInt32 s_TypeSize[Type::ENUM_COUNT];

  void CopyDataFromVariant(WUInt8* pDest, const WVariant* pValue) const;

  WHashedString m_sName;
  WEnum<Type> m_Type;
  WUInt8 m_uiArrayElements = 0;
  WUInt16 m_uiOffset = 0;
};

/// Reflection data of a shader constant buffer.
/// \sa WShaderResourceBinding
class W_RENDERERFOUNDATION_DLL WShaderConstantBufferLayout : public WRefCounted
{
public:
  WUInt32 m_uiTotalSize = 0;
  WHybridArray<WShaderConstant, 16> m_Constants;
  bool operator==(const WShaderConstantBufferLayout& rhs) const;
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WShaderConstantBufferLayout&);
};

/// Shader reflection of the vertex shader input.
/// This is needed to figure out how to map a WGALVertexDeclaration to a vertex shader stage.
/// \sa WGALShaderByteCode
struct W_RENDERERFOUNDATION_DLL WShaderVertexInputAttribute
{
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  WEnum<WGALVertexAttributeSemantic> m_eSemantic = WGALVertexAttributeSemantic::Position;
  WEnum<WGALResourceFormat> m_eFormat = WGALResourceFormat::XYZFloat;
  WUInt8 m_uiLocation = 0; // The bind slot of a vertex input
};

/// Shader reflection of a single shader resource (texture, constant buffer, etc.).
/// \sa WGALShaderByteCode
struct W_RENDERERFOUNDATION_DLL WShaderResourceBinding
{
  W_DECLARE_MEM_RELOCATABLE_TYPE();
  WEnum<WGALShaderResourceType> m_ResourceType;      //< The type of shader resource. Note, not all are supported by W right now.
  WEnum<WGALShaderTextureType> m_TextureType;        //< Only valid if m_ResourceType is Texture, TextureRW or TextureAndSampler.
  WBitflags<WGALShaderStageFlags> m_Stages;          //< The shader stages under which this resource is bound.
  WInt16 m_iBindGroup = -1;                           //< The bind group to which this resource belongs.
  WInt16 m_iSlot = -1;                                //< The slot under which the resource needs to be bound in the bind group.
  WUInt32 m_uiArraySize = 1;                          //< Number of array elements. Only 1 is currently supported. 0 if bindless.
  WHashedString m_sName;                              //< Name under which a resource must be bound to fulfill this resource binding.
  WSharedPtr<WShaderConstantBufferLayout> m_pLayout; //< Only valid if WGALShaderResourceType is ConstantBuffer, PushConstants, StructuredBuffer, StructuredBufferRW.

  static WResult CreateMergedShaderResourceBinding(const WArrayPtr<WArrayPtr<const WShaderResourceBinding>>& resourcesPerStage, WDynamicArray<WShaderResourceBinding>& out_bindings, bool bAllowMultipleBindingPerName);
};

/// This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class W_RENDERERFOUNDATION_DLL WGALShaderByteCode : public WRefCounted
{
public:
  WGALShaderByteCode();
  ~WGALShaderByteCode();

  inline const void* GetByteCode() const;
  inline WUInt32 GetSize() const;
  inline bool IsValid() const;

public:
  // Filled out by Shader Compiler platform implementation
  WDynamicArray<WUInt8> m_ByteCode;
  WHybridArray<WShaderResourceBinding, 8> m_ShaderResourceBindings;
  WHybridArray<WShaderVertexInputAttribute, 8> m_ShaderVertexInput;
  // Only set in the hull shader.
  WUInt8 m_uiTessellationPatchControlPoints = 0;

  // Filled out by compiler base library
  WEnum<WGALShaderStage> m_Stage = WGALShaderStage::ENUM_COUNT;
  bool m_bWasCompiledWithDebug = false;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
