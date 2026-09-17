#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace WShaderHelper
{
  /// Parses shader source code into named sections.
  ///
  /// Shader files are divided into sections like [PLATFORMS], [PERMUTATIONS], [VERTEXSHADER], etc.
  /// This class extracts these sections for further processing during shader compilation.
  class W_RENDERERCORE_DLL WTextSectionizer
  {
  public:
    void Clear();

    /// Registers a section name to look for during parsing.
    void AddSection(const char* szName);

    /// Parses the text and extracts all registered sections.
    void Process(const char* szText);

    /// Returns the content of a specific section and its starting line number.
    WStringView GetSectionContent(WUInt32 uiSection, WUInt32& out_uiFirstLine) const;

  private:
    struct WTextSection
    {
      WTextSection(const char* szName)
        : m_sName(szName)

      {
      }

      void Reset()
      {
        m_szSectionStart = nullptr;
        m_Content = WStringView();
        m_uiFirstLine = 0;
      }

      WString m_sName;
      const char* m_szSectionStart = nullptr;
      WStringView m_Content;
      WUInt32 m_uiFirstLine = 0;
    };

    WStringBuilder m_sText;
    WHybridArray<WTextSection, 16> m_Sections;
  };

  /// Defines the standard section names in shader files.
  struct WShaderSections
  {
    enum Enum
    {
      PLATFORMS,         ///< Platform requirements
      PERMUTATIONS,      ///< Shader permutation variables
      MATERIALPARAMETER, ///< Material parameters exposed to materials
      MATERIALCONFIG,    ///< Material configuration
      MATERIALCONSTANTS, ///< Material constant definitions
      RENDERSTATE,       ///< Render state configuration
      SHADER,            ///< Shared shader code
      VERTEXSHADER,      ///< Vertex shader entry point
      HULLSHADER,        ///< Hull shader entry point (tessellation)
      DOMAINSHADER,      ///< Domain shader entry point (tessellation)
      GEOMETRYSHADER,    ///< Geometry shader entry point
      PIXELSHADER,       ///< Pixel shader entry point
      COMPUTESHADER,     ///< Compute shader entry point
      TEMPLATE_VARS      ///< Template variables for shader generation
    };
  };

  /// Extracts all standard sections from shader source code.
  W_RENDERERCORE_DLL void GetShaderSections(const char* szContent, WTextSectionizer& out_sections);

  /// Calculates a hash from permutation variables for shader variant identification.
  WUInt32 CalculateHash(const WArrayPtr<WPermutationVar>& vars);
} // namespace WShaderHelper
