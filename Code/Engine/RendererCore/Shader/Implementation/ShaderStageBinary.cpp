#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>



//////////////////////////////////////////////////////////////////////////

WMap<WUInt32, WShaderStageBinary> WShaderStageBinary::s_ShaderStageBinaries[WGALShaderStage::ENUM_COUNT];
WMutex WShaderStageBinary::s_ShaderStageBinariesLock;

WShaderStageBinary::WShaderStageBinary() = default;

WShaderStageBinary::~WShaderStageBinary()
{
  m_pGALByteCode = nullptr;
}

WResult WShaderStageBinary::Write(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = WShaderStageBinary::VersionCurrent;

  // WShaderStageBinary
  inout_stream << uiVersion;
  inout_stream << m_uiSourceHash;

  // WGALShaderByteCode
  inout_stream << m_pGALByteCode->m_uiTessellationPatchControlPoints;
  inout_stream << m_pGALByteCode->m_Stage;
  inout_stream << m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  const WUInt32 uiByteCodeSize = m_pGALByteCode->m_ByteCode.GetCount();
  inout_stream << uiByteCodeSize;
  if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize).Failed())
    return W_FAILURE;

  // m_ShaderResourceBindings
  const WUInt16 uiResources = static_cast<WUInt16>(m_pGALByteCode->m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;
  for (const auto& r : m_pGALByteCode->m_ShaderResourceBindings)
  {
    inout_stream << r.m_ResourceType;
    inout_stream << r.m_TextureType;
    inout_stream << r.m_Stages;
    inout_stream << r.m_iBindGroup;
    inout_stream << r.m_iSlot;
    inout_stream << r.m_uiArraySize;
    inout_stream << r.m_sName.GetData();
    const bool bHasLayout = r.m_pLayout != nullptr;
    inout_stream << bHasLayout;
    if (bHasLayout)
    {
      W_SUCCEED_OR_RETURN(Write(inout_stream, *r.m_pLayout));
    }
  }

  // m_ShaderVertexInput
  const WUInt16 uiVertexInputs = static_cast<WUInt16>(m_pGALByteCode->m_ShaderVertexInput.GetCount());
  inout_stream << uiVertexInputs;
  for (const auto& v : m_pGALByteCode->m_ShaderVertexInput)
  {
    inout_stream << v.m_eSemantic;
    inout_stream << v.m_eFormat;
    inout_stream << v.m_uiLocation;
  }

  return W_SUCCESS;
}


WResult WShaderStageBinary::Write(WStreamWriter& inout_stream, const WShaderConstantBufferLayout& layout) const
{
  inout_stream << layout.m_uiTotalSize;

  WUInt16 uiConstants = static_cast<WUInt16>(layout.m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : layout.m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return W_SUCCESS;
}

WResult WShaderStageBinary::Read(WStreamReader& inout_stream)
{
  W_ASSERT_DEBUG(m_pGALByteCode == nullptr, "");
  m_pGALByteCode = W_DEFAULT_NEW(WGALShaderByteCode);

  WUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(WUInt8)) != sizeof(WUInt8))
    return W_FAILURE;

  if (uiVersion < WShaderStageBinary::Version::Version6)
  {
    WLog::Error("Old shader binaries are not supported anymore and need to be recompiled, please delete shader cache.");
    return W_FAILURE;
  }

  W_ASSERT_DEV(uiVersion <= WShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  inout_stream >> m_uiSourceHash;

  // WGALShaderByteCode
  if (uiVersion >= WShaderStageBinary::Version::Version7)
  {
    inout_stream >> m_pGALByteCode->m_uiTessellationPatchControlPoints;
  }
  inout_stream >> m_pGALByteCode->m_Stage;
  inout_stream >> m_pGALByteCode->m_bWasCompiledWithDebug;

  // m_ByteCode
  {
    WUInt32 uiByteCodeSize = 0;
    inout_stream >> uiByteCodeSize;
    m_pGALByteCode->m_ByteCode.SetCountUninitialized(uiByteCodeSize);
    if (!m_pGALByteCode->m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_pGALByteCode->m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
      return W_FAILURE;
  }

  // m_ShaderResourceBindings
  {
    WUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_pGALByteCode->m_ShaderResourceBindings.SetCount(uiResources);

    WString sTemp;

    for (auto& r : m_pGALByteCode->m_ShaderResourceBindings)
    {
      inout_stream >> r.m_ResourceType;
      inout_stream >> r.m_TextureType;
      inout_stream >> r.m_Stages;
      inout_stream >> r.m_iBindGroup;
      inout_stream >> r.m_iSlot;
      inout_stream >> r.m_uiArraySize;
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());

      bool bHasLayout = false;
      inout_stream >> bHasLayout;

      if (bHasLayout)
      {
        r.m_pLayout = W_DEFAULT_NEW(WShaderConstantBufferLayout);
        W_SUCCEED_OR_RETURN(Read(inout_stream, *r.m_pLayout));
      }
    }
  }

  // m_ShaderVertexInput
  {
    WUInt16 uiVertexInputs = 0;
    inout_stream >> uiVertexInputs;
    m_pGALByteCode->m_ShaderVertexInput.SetCount(uiVertexInputs);

    for (auto& v : m_pGALByteCode->m_ShaderVertexInput)
    {
      inout_stream >> v.m_eSemantic;
      inout_stream >> v.m_eFormat;
      inout_stream >> v.m_uiLocation;
    }
  }

  return W_SUCCESS;
}



WResult WShaderStageBinary::Read(WStreamReader& inout_stream, WShaderConstantBufferLayout& out_layout)
{
  inout_stream >> out_layout.m_uiTotalSize;

  WUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  out_layout.m_Constants.SetCount(uiConstants);

  for (auto& constant : out_layout.m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return W_SUCCESS;
}

WSharedPtr<const WGALShaderByteCode> WShaderStageBinary::GetByteCode() const
{
  return m_pGALByteCode;
}

WResult WShaderStageBinary::WriteStageBinary(WLogInterface* pLog, WStringView sPlatform) const
{
  WStringBuilder sShaderStageFile = WShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(sPlatform);
  sShaderStageFile.AppendFormat("/{0}_{1}.WShaderStage", WGALShaderStage::Names[m_pGALByteCode->m_Stage], WArgU(m_uiSourceHash, 8, true, 16, true));

  WFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile).Failed())
  {
    WLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return W_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    WLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return W_FAILURE;
  }

  return W_SUCCESS;
}

// static
WShaderStageBinary* WShaderStageBinary::LoadStageBinary(WGALShaderStage::Enum Stage, WUInt32 uiHash, WStringView sPlatform)
{
  W_LOCK(s_ShaderStageBinariesLock);
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    WStringBuilder sShaderStageFile = WShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(sPlatform);
    sShaderStageFile.AppendFormat("/{0}_{1}.WShaderStage", WGALShaderStage::Names[Stage], WArgU(uiHash, 8, true, 16, true));

    WFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      WLog::Debug("Could not open shader stage file '{0}' for reading", sShaderStageFile);
      return nullptr;
    }

    WShaderStageBinary shaderStageBinary;
    if (shaderStageBinary.Read(StageFileIn).Failed())
    {
      WLog::Error("Could not read shader stage file '{0}'", sShaderStageFile);
      return nullptr;
    }

    itStage = WShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, shaderStageBinary);
  }

  WShaderStageBinary* pShaderStageBinary = &itStage.Value();
  return pShaderStageBinary;
}

// static
void WShaderStageBinary::OnEngineShutdown()
{
  W_LOCK(s_ShaderStageBinariesLock);
  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}
