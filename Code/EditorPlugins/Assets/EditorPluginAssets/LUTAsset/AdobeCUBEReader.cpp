
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <Foundation/CodeUtils/Tokenizer.h>

// This file implements a simple reader for the Adobe CUBE LUT file format.
// The specification can be found here (at the time of this writing):
// https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

namespace
{
  bool GetVec3FromLine(WArrayPtr<const WToken*> line, WUInt32 uiSkip, WVec3& ref_vOut)
  {
    if (line.GetCount() < (uiSkip + 3 + 2))
    {
      return false;
    }

    if ((line[uiSkip + 0]->m_iType != WTokenType::Float && line[uiSkip + 0]->m_iType != WTokenType::Integer) ||
        line[uiSkip + 1]->m_iType != WTokenType::Whitespace ||
        (line[uiSkip + 2]->m_iType != WTokenType::Float && line[uiSkip + 2]->m_iType != WTokenType::Integer) ||
        line[uiSkip + 3]->m_iType != WTokenType::Whitespace ||
        (line[uiSkip + 4]->m_iType != WTokenType::Float && line[uiSkip + 4]->m_iType != WTokenType::Integer))
    {
      return false;
    }

    double res = 0;
    WString sVal = line[uiSkip + 0]->m_DataView;

    if (WConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.x = static_cast<float>(res);

    sVal = line[uiSkip + 2]->m_DataView;
    if (WConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.y = static_cast<float>(res);


    sVal = line[uiSkip + 4]->m_DataView;
    if (WConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.z = static_cast<float>(res);

    return true;
  }
} // namespace

WAdobeCUBEReader::WAdobeCUBEReader() = default;
WAdobeCUBEReader::~WAdobeCUBEReader() = default;

WStatus WAdobeCUBEReader::ParseFile(WStreamReader& inout_stream, WLogInterface* pLog /*= nullptr*/)
{
  WString sContent;
  sContent.ReadAll(inout_stream);

  WTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);

  tokenizer.Tokenize(
    WArrayPtr<const WUInt8>((const WUInt8*)sContent.GetData(), sContent.GetElementCount()), pLog ? pLog : WLog::GetThreadLocalLogSystem());


  auto tokens = tokenizer.GetTokens();

  WTempHybridArray<const WToken*, 32> line;
  WUInt32 firstToken = 0;

  while (tokenizer.GetNextLine(firstToken, line).Succeeded())
  {
    if (line[0]->m_iType == WTokenType::LineComment || line[0]->m_iType == WTokenType::Newline)
      continue;

    if (line[0]->m_DataView == "TITLE")
    {
      if (line.GetCount() < 3)
      {
        return WStatus(WFmt("LUT file has invalid TITLE line."));
      }

      if (line[1]->m_iType != WTokenType::Whitespace && line[2]->m_iType != WTokenType::String1)
      {
        return WStatus(WFmt("LUT file has invalid TITLE line, expected TITLE<whitespace>\"<string>\"."));
      }

      m_sTitle = line[2]->m_DataView;

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MIN")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMin))
      {
        return WStatus(WFmt("LUT file has invalid DOMAIN_MIN line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MAX")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMax))
      {
        return WStatus(WFmt("LUT file has invalid DOMAIN_MAX line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "LUT_1D_SIZE")
    {
      return WStatus(WFmt("LUT file specifies a 1D LUT which is currently not implemented."));
    }
    else if (line[0]->m_DataView == "LUT_3D_SIZE")
    {
      if (m_uiLUTSize > 0)
      {
        return WStatus(WFmt("LUT file has more than one LUT_3D_SIZE entry. Aborting parse."));
      }

      if (line.GetCount() < 3)
      {
        return WStatus(WFmt("LUT file has invalid LUT_3D_SIZE line."));
      }

      if (line[1]->m_iType != WTokenType::Whitespace && line[2]->m_iType != WTokenType::Integer)
      {
        return WStatus(WFmt("LUT file has invalid LUT_3D_SIZE line, expected LUT_3D_SIZE<whitespace><N>."));
      }

      const WString sVal = line[2]->m_DataView;
      if (WConversionUtils::StringToUInt(sVal, m_uiLUTSize).Failed())
      {
        return WStatus(WFmt("LUT file has invalid LUT_3D_SIZE line, couldn't parse LUT size as WUInt32."));
      }

      if (m_uiLUTSize < 2 || m_uiLUTSize > 256)
      {
        return WStatus(WFmt("LUT file has invalid LUT_3D_SIZE size, got {0} - but must be in range 2, 256.", m_uiLUTSize));
      }

      m_LUTValues.Reserve(m_uiLUTSize * m_uiLUTSize * m_uiLUTSize);

      continue;
    }

    if (line[0]->m_iType == WTokenType::Float || line[0]->m_iType == WTokenType::Integer)
    {
      if (m_uiLUTSize == 0)
      {
        return WStatus(WFmt("LUT data before LUT size was specified."));
      }

      WVec3 lineValues;
      if (!::GetVec3FromLine(line, 0, lineValues))
      {
        return WStatus(WFmt("LUT data couldn't be read."));
      }

      m_LUTValues.PushBack(lineValues);
    }
  }

  if (m_vDomainMin.x > m_vDomainMax.x || m_vDomainMin.y > m_vDomainMax.y || m_vDomainMin.z > m_vDomainMax.z)
  {
    return WStatus("LUT file has invalid domain min/max values.");
  }

  if (m_LUTValues.GetCount() != (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize))
  {
    return WStatus(WFmt("LUT data incomplete, read {0} values but expected {1} values given a LUT size of {2}.", m_LUTValues.GetCount(),
      (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize), m_uiLUTSize));
  }

  return WStatus(W_SUCCESS);
}

WVec3 WAdobeCUBEReader::GetDomainMin() const
{
  return m_vDomainMin;
}

WVec3 WAdobeCUBEReader::GetDomainMax() const
{
  return m_vDomainMax;
}

WUInt32 WAdobeCUBEReader::GetLUTSize() const
{
  return m_uiLUTSize;
}

const WString& WAdobeCUBEReader::GetTitle() const
{
  return m_sTitle;
}

WVec3 WAdobeCUBEReader::GetLUTEntry(WUInt32 r, WUInt32 g, WUInt32 b) const
{
  return m_LUTValues[GetLUTIndex(r, g, b)];
}

WUInt32 WAdobeCUBEReader::GetLUTIndex(WUInt32 r, WUInt32 g, WUInt32 b) const
{
  return b * m_uiLUTSize * m_uiLUTSize + g * m_uiLUTSize + r;
}
