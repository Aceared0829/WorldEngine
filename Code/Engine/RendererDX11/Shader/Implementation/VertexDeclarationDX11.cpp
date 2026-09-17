#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererFoundation/Shader/Shader.h>

#include <d3d11.h>

WGALVertexDeclarationDX11::WGALVertexDeclarationDX11(const WGALVertexDeclarationCreationDescription& Description)
  : WGALVertexDeclaration(Description)

{
}

WGALVertexDeclarationDX11::~WGALVertexDeclarationDX11() = default;

static const char* GALSemanticToDX11[] = {
  "POSITION",
  "NORMAL",
  "TANGENT",
  "COLOR",
  "COLOR",
  "COLOR",
  "COLOR",
  "COLOR",
  "COLOR",
  "COLOR",
  "COLOR",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "TEXCOORD",
  "BITANGENT",
  "BONEINDICES",
  "BONEINDICES",
  "BONEWEIGHTS",
  "BONEWEIGHTS",
  "DATAOFFSETS",
};

static UINT GALSemanticToIndexDX11[] = {
  0,                            // Position
  0,                            // Normal
  0,                            // Tangent
  0, 1, 2, 3, 4, 5, 6, 7,       // Color
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, // TexCoord
  0,                            // BiTangent
  0, 1,                         // BoneIndices
  0, 1,                         // BoneWeights
  0,                            // DataOffsets
};

static D3D11_INPUT_CLASSIFICATION GalInputRateToDX11[] = {D3D11_INPUT_PER_VERTEX_DATA, D3D11_INPUT_PER_INSTANCE_DATA};

static_assert(W_ARRAY_SIZE(GALSemanticToDX11) == WGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToDX11 array size does not match vertex attribute semantic count");
static_assert(W_ARRAY_SIZE(GALSemanticToIndexDX11) == WGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToIndexDX11 array size does not match vertex attribute semantic count");

W_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

WResult WGALVertexDeclarationDX11::InitPlatform(WGALDevice* pDevice)
{
  WTempHybridArray<D3D11_INPUT_ELEMENT_DESC, 8> DXInputElementDescs;

  WGALDeviceDX11* pDXDevice = static_cast<WGALDeviceDX11*>(pDevice);

  const WGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(WGALShaderStage::VertexShader))
  {
    return W_FAILURE;
  }

  auto usedVertexAttributes = pShader->GetVertexInputAttributes();
  auto IsAttributeUsed = [&](WGALVertexAttributeSemantic::Enum semantic)
  {
    for (auto attrib : usedVertexAttributes)
    {
      if (attrib.m_eSemantic == semantic)
        return true;
    }
    return false;
  };

  // Copy attribute descriptions
  for (WUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const WGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];
    if (!IsAttributeUsed(Current.m_eSemantic))
      continue;

    D3D11_INPUT_ELEMENT_DESC DXDesc;
    DXDesc.AlignedByteOffset = Current.m_uiOffset;
    DXDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;

    if (DXDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
      WLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return W_FAILURE;
    }

    const WGALVertexBinding& binding = m_Description.m_VertexBindings[Current.m_uiVertexBufferSlot];

    DXDesc.InputSlot = Current.m_uiVertexBufferSlot;
    DXDesc.InputSlotClass = GalInputRateToDX11[binding.m_Rate.GetValue()];
    DXDesc.InstanceDataStepRate = binding.m_Rate == WGALVertexBindingRate::Vertex ? 0 : 1;
    DXDesc.SemanticIndex = GALSemanticToIndexDX11[Current.m_eSemantic];
    DXDesc.SemanticName = GALSemanticToDX11[Current.m_eSemantic];

    DXInputElementDescs.PushBack(DXDesc);
  }

  if (DXInputElementDescs.IsEmpty())
  {
    return W_FAILURE;
  }

  m_VertexBufferStrides.SetCount(m_Description.m_VertexBindings.GetCount());
  for (WUInt32 i = 0; i < m_Description.m_VertexBindings.GetCount(); i++)
  {
    m_VertexBufferStrides[i] = m_Description.m_VertexBindings[i].m_uiStride;
  }

  const WSharedPtr<const WGALShaderByteCode>& pByteCode = pShader->GetDescription().m_ByteCodes[WGALShaderStage::VertexShader];

  if (FAILED(pDXDevice->GetDXDevice()->CreateInputLayout(&DXInputElementDescs[0], DXInputElementDescs.GetCount(), pByteCode->GetByteCode(), pByteCode->GetSize(), &m_pDXInputLayout)))
  {
    return W_FAILURE;
  }
  else
  {
    return W_SUCCESS;
  }
}

WResult WGALVertexDeclarationDX11::DeInitPlatform(WGALDevice* pDevice)
{
  W_IGNORE_UNUSED(pDevice);

  W_GAL_DX11_RELEASE(m_pDXInputLayout);
  return W_SUCCESS;
}
