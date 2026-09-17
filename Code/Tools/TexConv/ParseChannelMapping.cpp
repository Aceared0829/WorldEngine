#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

static WStringView ToString(WTexConvChannelValue::Enum e)
{
  switch (e)
  {
    case WTexConvChannelValue::Red:
      return "Red";
    case WTexConvChannelValue::Green:
      return "Green";
    case WTexConvChannelValue::Blue:
      return "Blue";
    case WTexConvChannelValue::Alpha:
      return "Alpha";
    case WTexConvChannelValue::Black:
      return "Black";
    case WTexConvChannelValue::White:
      return "White";

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  return "";
}

WResult WTexConv::ParseChannelMappings()
{
  if (m_Processor.m_Descriptor.m_OutputType == WTexConvOutputType::Atlas)
    return W_SUCCESS;

  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;

  W_SUCCEED_OR_RETURN(ParseChannelSliceMapping(-1));

  for (WUInt32 slice = 0; slice < 64; ++slice)
  {
    const WUInt32 uiPrevMappings = mappings.GetCount();

    W_SUCCEED_OR_RETURN(ParseChannelSliceMapping(slice));

    if (uiPrevMappings == mappings.GetCount())
    {
      // if no new mapping was found, don't try to find more
      break;
    }
  }

  if (!mappings.IsEmpty())
  {
    WLog::Info("Custom output channel mapping:");
    for (WUInt32 m = 0; m < mappings.GetCount(); ++m)
    {
      WLog::Info("Slice {}, R -> Input file {}, {}", m, mappings[m].m_Channel[0].m_iInputImageIndex, ToString(mappings[m].m_Channel[0].m_ChannelValue));
      WLog::Info("Slice {}, G -> Input file {}, {}", m, mappings[m].m_Channel[1].m_iInputImageIndex, ToString(mappings[m].m_Channel[1].m_ChannelValue));
      WLog::Info("Slice {}, B -> Input file {}, {}", m, mappings[m].m_Channel[2].m_iInputImageIndex, ToString(mappings[m].m_Channel[2].m_ChannelValue));
      WLog::Info("Slice {}, A -> Input file {}, {}", m, mappings[m].m_Channel[3].m_iInputImageIndex, ToString(mappings[m].m_Channel[3].m_ChannelValue));
    }
  }

  return W_SUCCESS;
}

WResult WTexConv::ParseChannelSliceMapping(WInt32 iSlice)
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();
  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;
  WStringBuilder tmp, param;

  const WUInt32 uiMappingIdx = iSlice < 0 ? 0 : iSlice;

  // input to output mappings
  {
    param = "-rgba";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, false));
    }

    param = "-rgb";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
    }

    param = "-rg";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
    }

    param = "-r";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, true));
    }

    param = "-g";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, true));
    }

    param = "-b";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, true));
    }

    param = "-a";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      W_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, true));
    }
  }
  return W_SUCCESS;
}

WResult WTexConv::ParseChannelMappingConfig(WTexConvChannelMapping& out_mapping, WStringView sCfg, WInt32 iChannelIndex, bool bSingleChannel)
{
  out_mapping.m_iInputImageIndex = -1;
  out_mapping.m_ChannelValue = WTexConvChannelValue::White;

  WStringBuilder tmp = sCfg;

  // '-r black' for setting it to zero
  if (tmp.IsEqual_NoCase("black"))
  {
    out_mapping.m_ChannelValue = WTexConvChannelValue::Black;
    return W_SUCCESS;
  }

  // '-r white' for setting it to 255
  if (tmp.IsEqual_NoCase("white"))
  {
    out_mapping.m_ChannelValue = WTexConvChannelValue::White;
    return W_SUCCESS;
  }

  // skip the 'in', if found
  // 'in' is optional, one can also write '-r 1.r' for '-r in1.r'
  if (tmp.StartsWith_NoCase("in"))
    tmp.Shrink(2, 0);

  if (tmp.StartsWith("."))
  {
    // no index given, e.g. '-r in.r'
    // in is equal to in0

    out_mapping.m_iInputImageIndex = 0;
  }
  else
  {
    WInt32 num = -1;
    const char* szLastPos = nullptr;
    if (WConversionUtils::StringToInt(tmp, num, &szLastPos).Failed())
    {
      WLog::Error("Could not parse channel mapping '{0}'", sCfg);
      return W_FAILURE;
    }

    // valid index after the 'in'
    if (num >= 0 && num < (WInt32)m_Processor.m_Descriptor.m_InputFiles.GetCount())
    {
      out_mapping.m_iInputImageIndex = (WInt8)num;
    }
    else
    {
      WLog::Error("Invalid channel mapping input file index '{0}'", num);
      return W_FAILURE;
    }

    WStringBuilder dummy = szLastPos;

    // continue after the index
    tmp = dummy;
  }

  // no additional info, e.g. '-g in2' is identical to '-g in2.g' (same channel)
  if (tmp.IsEmpty())
  {
    out_mapping.m_ChannelValue = (WTexConvChannelValue::Enum)((WInt32)WTexConvChannelValue::Red + iChannelIndex);
    return W_SUCCESS;
  }

  if (!tmp.StartsWith("."))
  {
    WLog::Error("Invalid channel mapping: Expected '.' after input file index in '{0}'", sCfg);
    return W_FAILURE;
  }

  tmp.Shrink(1, 0);

  if (!bSingleChannel)
  {
    // in case of '-rgb in1.bgr' map r to b, g to g, b to r, etc.
    // in case of '-rgb in1.r' map everything to the same input
    if (tmp.GetCharacterCount() > 1)
      tmp.Shrink(iChannelIndex, 0);
  }

  // no additional info, e.g. '-rgb in2.rg'
  if (tmp.IsEmpty())
  {
    WLog::Error("Invalid channel mapping: Too few channel identifiers '{0}'", sCfg);
    return W_FAILURE;
  }

  {
    const WUInt32 uiChar = tmp.GetIteratorFront().GetCharacter();

    if (uiChar == 'r')
    {
      out_mapping.m_ChannelValue = WTexConvChannelValue::Red;
    }
    else if (uiChar == 'g')
    {
      out_mapping.m_ChannelValue = WTexConvChannelValue::Green;
    }
    else if (uiChar == 'b')
    {
      out_mapping.m_ChannelValue = WTexConvChannelValue::Blue;
    }
    else if (uiChar == 'a')
    {
      out_mapping.m_ChannelValue = WTexConvChannelValue::Alpha;
    }
    else
    {
      WLog::Error("Invalid channel mapping: Unexpected channel identifier in '{}'", sCfg);
      return W_FAILURE;
    }

    tmp.Shrink(1, 0);
  }

  return W_SUCCESS;
}
