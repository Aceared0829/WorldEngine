#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderHelper.h>

namespace WShaderHelper
{
  void WTextSectionizer::Clear()
  {
    m_Sections.Clear();
    m_sText.Clear();
  }

  void WTextSectionizer::AddSection(const char* szName)
  {
    m_Sections.PushBack(WTextSection(szName));
  }

  void WTextSectionizer::Process(const char* szText)
  {
    for (WUInt32 i = 0; i < m_Sections.GetCount(); ++i)
      m_Sections[i].Reset();

    m_sText = szText;


    for (WUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName.GetData());
      while ((m_Sections[s].m_szSectionStart != nullptr) && (m_Sections[s].m_szSectionStart != m_sText.GetData()) && (*(m_Sections[s].m_szSectionStart - 1) != '\n'))
      {
        m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName.GetData(), m_Sections[s].m_szSectionStart + 1);
      }

      if (m_Sections[s].m_szSectionStart != nullptr)
        m_Sections[s].m_Content = WStringView(m_Sections[s].m_szSectionStart + m_Sections[s].m_sName.GetElementCount());
    }

    for (WUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      if (m_Sections[s].m_szSectionStart == nullptr)
        continue;

      WUInt32 uiLine = 1;

      const char* sz = m_sText.GetData();
      while (sz < m_Sections[s].m_szSectionStart)
      {
        if (*sz == '\n')
          ++uiLine;

        ++sz;
      }

      m_Sections[s].m_uiFirstLine = uiLine;

      for (WUInt32 s2 = 0; s2 < m_Sections.GetCount(); ++s2)
      {
        if (s == s2)
          continue;

        if (m_Sections[s2].m_szSectionStart > m_Sections[s].m_szSectionStart)
        {
          const char* szContentStart = m_Sections[s].m_Content.GetStartPointer();
          const char* szSectionEnd = WMath::Min(m_Sections[s].m_Content.GetEndPointer(), m_Sections[s2].m_szSectionStart);

          m_Sections[s].m_Content = WStringView(szContentStart, szSectionEnd);
          m_Sections[s].m_Content.Trim(" \t\r\n");
        }
      }
    }
  }

  WStringView WTextSectionizer::GetSectionContent(WUInt32 uiSection, WUInt32& out_uiFirstLine) const
  {
    out_uiFirstLine = m_Sections[uiSection].m_uiFirstLine;
    return m_Sections[uiSection].m_Content;
  }

  void GetShaderSections(const char* szContent, WTextSectionizer& out_sections)
  {
    out_sections.Clear();

    out_sections.AddSection("[PLATFORMS]");
    out_sections.AddSection("[PERMUTATIONS]");
    out_sections.AddSection("[MATERIALPARAMETER]");
    out_sections.AddSection("[MATERIALCONFIG]");
    out_sections.AddSection("[MATERIALCONSTANTS]");
    out_sections.AddSection("[RENDERSTATE]");
    out_sections.AddSection("[SHADER]");
    out_sections.AddSection("[VERTEXSHADER]");
    out_sections.AddSection("[HULLSHADER]");
    out_sections.AddSection("[DOMAINSHADER]");
    out_sections.AddSection("[GEOMETRYSHADER]");
    out_sections.AddSection("[PIXELSHADER]");
    out_sections.AddSection("[COMPUTESHADER]");
    out_sections.AddSection("[TEMPLATE_VARS]");

    out_sections.Process(szContent);
  }

  WUInt32 CalculateHash(const WArrayPtr<WPermutationVar>& vars)
  {
    WTempHybridArray<WUInt64, 128> buffer;
    buffer.SetCountUninitialized(vars.GetCount() * 2);

    for (WUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& var = vars[i];
      buffer[i * 2 + 0] = var.m_sName.GetHash();
      buffer[i * 2 + 1] = var.m_sValue.GetHash();
    }

    auto bytes = buffer.GetByteArrayPtr();
    return WHashingUtils::xxHash32(bytes.GetPtr(), bytes.GetCount());
  }
} // namespace WShaderHelper
