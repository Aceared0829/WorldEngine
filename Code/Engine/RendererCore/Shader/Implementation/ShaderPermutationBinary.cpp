#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct WShaderPermutationBinaryVersion
{
  enum Enum : WUInt32
  {
    Version1 = 1,
    Version2 = 2,
    Version3 = 3,
    Version4 = 4,
    Version5 = 5,
    Version6 = 6, // Fixed DX11 particles vanishing
    Version7 = 7,

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

WShaderPermutationBinary::WShaderPermutationBinary()
{
  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

WResult WShaderPermutationBinary::Write(WStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an WDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  W_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  const WUInt8 uiVersion = WShaderPermutationBinaryVersion::Current;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(WUInt8)).Failed())
    return W_FAILURE;

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return W_FAILURE;
  }

  m_StateDescriptor.Save(inout_stream);

  inout_stream << m_PermutationVars.GetCount();

  for (auto& var : m_PermutationVars)
  {
    inout_stream << var.m_sName.GetString();
    inout_stream << var.m_sValue.GetString();
  }

  return W_SUCCESS;
}

WResult WShaderPermutationBinary::Read(WStreamReader& inout_stream, bool& out_bOldVersion)
{
  W_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  WUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(WUInt8)) != sizeof(WUInt8))
    return W_FAILURE;

  W_ASSERT_DEV(uiVersion <= WShaderPermutationBinaryVersion::Current, "Wrong Version {0}", uiVersion);

  out_bOldVersion = uiVersion != WShaderPermutationBinaryVersion::Current;

  for (WUInt32 stage = 0; stage < WGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return W_FAILURE;
  }

  m_StateDescriptor.Load(inout_stream);

  if (uiVersion >= WShaderPermutationBinaryVersion::Version2)
  {
    WUInt32 uiPermutationCount;
    inout_stream >> uiPermutationCount;

    m_PermutationVars.SetCount(uiPermutationCount);

    WStringBuilder tmp;
    for (WUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVars[i];

      inout_stream >> tmp;
      var.m_sName.Assign(tmp.GetData());
      inout_stream >> tmp;
      var.m_sValue.Assign(tmp.GetData());
    }
  }

  return W_SUCCESS;
}
