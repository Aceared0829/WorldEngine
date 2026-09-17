#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ViewRenderMode.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WViewRenderMode, 1)
  W_ENUM_CONSTANT(WViewRenderMode::None)->AddAttributes(new WGroupAttribute("Default")),
  W_ENUM_CONSTANT(WViewRenderMode::WireframeColor)->AddAttributes(new WGroupAttribute("Wireframe")),
  W_ENUM_CONSTANT(WViewRenderMode::WireframeMonochrome),
  W_ENUM_CONSTANT(WViewRenderMode::DiffuseLitOnly)->AddAttributes(new WGroupAttribute("Lighting")),
  W_ENUM_CONSTANT(WViewRenderMode::SpecularLitOnly),
  W_ENUM_CONSTANT(WViewRenderMode::LightCount)->AddAttributes(new WGroupAttribute("Performance")),
  W_ENUM_CONSTANT(WViewRenderMode::DecalCount),
  W_ENUM_CONSTANT(WViewRenderMode::StaticVsDynamic),
  W_ENUM_CONSTANT(WViewRenderMode::TexCoordsUV0)->AddAttributes(new WGroupAttribute("TexCoords")),
  W_ENUM_CONSTANT(WViewRenderMode::TexCoordsUV1),
  W_ENUM_CONSTANT(WViewRenderMode::VertexColors0)->AddAttributes(new WGroupAttribute("VertexColors")),
  W_ENUM_CONSTANT(WViewRenderMode::VertexColors1),
  W_ENUM_CONSTANT(WViewRenderMode::VertexNormals)->AddAttributes(new WGroupAttribute("Normals")),
  W_ENUM_CONSTANT(WViewRenderMode::VertexTangents),
  W_ENUM_CONSTANT(WViewRenderMode::PixelNormals),
  W_ENUM_CONSTANT(WViewRenderMode::DiffuseColor)->AddAttributes(new WGroupAttribute("PixelColors")),
  W_ENUM_CONSTANT(WViewRenderMode::DiffuseColorRange),
  W_ENUM_CONSTANT(WViewRenderMode::SpecularColor),
  W_ENUM_CONSTANT(WViewRenderMode::EmissiveColor),
  W_ENUM_CONSTANT(WViewRenderMode::Roughness)->AddAttributes(new WGroupAttribute("Surface")),
  W_ENUM_CONSTANT(WViewRenderMode::Occlusion),
  W_ENUM_CONSTANT(WViewRenderMode::Depth),
  W_ENUM_CONSTANT(WViewRenderMode::BoneWeights)->AddAttributes(new WGroupAttribute("Animation")),
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
WTempHashedString WViewRenderMode::GetPermutationValue(Enum renderMode)
{
  if (renderMode >= WireframeColor && renderMode <= WireframeMonochrome)
  {
    return "RENDER_PASS_WIREFRAME";
  }
  else if (renderMode >= DiffuseLitOnly && renderMode < ENUM_COUNT)
  {
    return "RENDER_PASS_EDITOR";
  }

  return "";
}

// static
int WViewRenderMode::GetRenderPassForShader(Enum renderMode)
{
  switch (renderMode)
  {
    case WViewRenderMode::None:
      return -1;

    case WViewRenderMode::WireframeColor:
      return WIREFRAME_RENDER_PASS_COLOR;

    case WViewRenderMode::WireframeMonochrome:
      return WIREFRAME_RENDER_PASS_MONOCHROME;

    case WViewRenderMode::DiffuseLitOnly:
      return EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY;

    case WViewRenderMode::SpecularLitOnly:
      return EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY;

    case WViewRenderMode::LightCount:
      return EDITOR_RENDER_PASS_LIGHT_COUNT;

    case WViewRenderMode::DecalCount:
      return EDITOR_RENDER_PASS_DECAL_COUNT;

    case WViewRenderMode::TexCoordsUV0:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV0;

    case WViewRenderMode::TexCoordsUV1:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV1;

    case WViewRenderMode::VertexColors0:
      return EDITOR_RENDER_PASS_VERTEX_COLORS0;

    case WViewRenderMode::VertexColors1:
      return EDITOR_RENDER_PASS_VERTEX_COLORS1;

    case WViewRenderMode::VertexNormals:
      return EDITOR_RENDER_PASS_VERTEX_NORMALS;

    case WViewRenderMode::VertexTangents:
      return EDITOR_RENDER_PASS_VERTEX_TANGENTS;

    case WViewRenderMode::PixelNormals:
      return EDITOR_RENDER_PASS_PIXEL_NORMALS;

    case WViewRenderMode::DiffuseColor:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR;

    case WViewRenderMode::DiffuseColorRange:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;

    case WViewRenderMode::SpecularColor:
      return EDITOR_RENDER_PASS_SPECULAR_COLOR;

    case WViewRenderMode::EmissiveColor:
      return EDITOR_RENDER_PASS_EMISSIVE_COLOR;

    case WViewRenderMode::Roughness:
      return EDITOR_RENDER_PASS_ROUGHNESS;

    case WViewRenderMode::Occlusion:
      return EDITOR_RENDER_PASS_OCCLUSION;

    case WViewRenderMode::Depth:
      return EDITOR_RENDER_PASS_DEPTH;

    case WViewRenderMode::StaticVsDynamic:
      return EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC;

    case WViewRenderMode::BoneWeights:
      return EDITOR_RENDER_PASS_BONE_WEIGHTS;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      return -1;
  }
}

// static
void WViewRenderMode::GetDebugText(Enum renderMode, WStringBuilder& out_sDebugText)
{
  if (renderMode == DiffuseColorRange)
  {
    out_sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
  }
  else if (renderMode == StaticVsDynamic)
  {
    out_sDebugText = "Static objects are shown in green, dynamic objects are shown in red.";
  }
}



W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ViewRenderMode);
