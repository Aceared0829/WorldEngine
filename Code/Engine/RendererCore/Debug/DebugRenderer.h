#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

template <typename Type>
class WRectTemplate;
using WRectFloat = WRectTemplate<float>;

class WFormatString;
class WFrustum;
class WRenderViewContext;

/// Horizontal alignment of debug text
struct WDebugTextHAlign
{
  using StorageType = WUInt8;

  enum Enum
  {
    Left,
    Center,
    Right,

    Default = Left
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WDebugTextHAlign);

/// Vertical alignment of debug text
struct WDebugTextVAlign
{
  using StorageType = WUInt8;

  enum Enum
  {
    Top,
    Center,
    Bottom,

    Default = Top
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WDebugTextVAlign);

/// Screen placement of debug text
struct WDebugTextPlacement
{
  using StorageType = WUInt8;

  enum Enum
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT,

    Default = TopLeft
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WDebugTextPlacement);

struct W_RENDERERCORE_DLL WDebugRendererLine
{
  W_DECLARE_POD_TYPE();

  WDebugRendererLine();
  WDebugRendererLine(const WVec3& vStart, const WVec3& vEnd);
  WDebugRendererLine(const WVec3& vStart, const WVec3& vEnd, const WColor& color);

  WVec3 m_start;
  WVec3 m_end;

  WColor m_startColor = WColor::White;
  WColor m_endColor = WColor::White;
};

struct W_RENDERERCORE_DLL WDebugRendererTriangle
{
  W_DECLARE_POD_TYPE();

  WDebugRendererTriangle();
  WDebugRendererTriangle(const WVec3& v0, const WVec3& v1, const WVec3& v2);

  WVec3 m_position[3];
  WColor m_color = WColor::White;
};

struct W_RENDERERCORE_DLL WDebugRendererTexturedTriangle
{
  W_DECLARE_POD_TYPE();

  WVec3 m_position[3];
  WVec2 m_texcoord[3];
  WColor m_color = WColor::White;
};

/// Enables a function to take an WMat3, WMat4 or WTransfrom.
struct WMatOrTransform
{
  WMatOrTransform(const WMat4& mMat4)
    : m_Mat4(mMat4)
  {
  }

  WMatOrTransform(const WMat3& mMat3)
  {
    m_Mat4.SetIdentity();
    m_Mat4.SetRotationalPart(mMat3);
  }

  WMatOrTransform(const WTransform& transform)
  {
    m_Mat4 = transform.GetAsMat4();
  }

  WMat4 m_Mat4;
};


/// Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the WDebugRendererContext constructors for what can be implicitly
/// used as a context.
class W_RENDERERCORE_DLL WDebugRenderer
{
public:
  /// Renders the given set of lines for one frame.
  static void DrawLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders lines that are always visible on top of all geometry (no depth test), with distance-based fade-out.
  ///
  /// Useful for showing lines through walls. To get distinct colors for visible vs. occluded portions, additionally
  /// call DrawLines() with the same lines but a different color: the depth-tested DrawLines() result overrides
  /// the always-on-top DrawLinesOccluded() result wherever the lines are not occluded.
  /// Rendering of the occluded layer can be disabled globally via the CVar 'Debug.RenderOccluded'.
  static void DrawLinesOccluded(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders the given set of lines in 2D (screen-space) for one frame.
  static void Draw2DLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color);

  /// Renders a cross for one frame.
  static void DrawCross(const WDebugRendererContext& context, const WVec3& vGlobalPosition, float fLineLength, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders a wireframe box for one frame.
  static void DrawLineBox(const WDebugRendererContext& context, const WBoundingBox& box, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const WDebugRendererContext& context, const WBoundingBox& box, float fCornerFraction, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const WDebugRendererContext& context, const WBoundingSphere& sphere, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const WDebugRendererContext& context, float fLength, float fRadius, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders an upright wireframe cylinder for one frame.
  static void DrawLineCylinderZ(const WDebugRendererContext& context, float fLength, float fRadius, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const WDebugRendererContext& context, const WFrustum& frustum, const WColor& color, bool bDrawPlaneNormals = false);

  /// Renders a solid box for one frame.
  static void DrawSolidBox(const WDebugRendererContext& context, const WBoundingBox& box, const WColor& color, WMatOrTransform mTransform = WMat4::MakeIdentity());

  /// Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const WDebugRendererContext& context, WArrayPtr<WDebugRendererTriangle> triangles, const WColor& color, bool bTwoSided = false);

  /// Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const WDebugRendererContext& context, WArrayPtr<WDebugRendererTexturedTriangle> triangles, const WColor& color, const WTexture2DResourceHandle& hTexture, bool bTwoSided = false);

  /// Renders a filled 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color);

  /// Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color, const WTexture2DResourceHandle& hTexture, WVec2 vScale = WVec2(1, 1));

  /// Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color, WGALTextureHandle hResourceView, WVec2 vScale = WVec2(1, 1));

  /// Renders a wireframe 2D rectangle in screen-space for one frame.
  static void Draw2DLineRectangle(const WDebugRendererContext& context, const WRectFloat& rectInPixel, float fDepth, const WColor& color);

  /// Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  static WUInt32 Draw2DText(const WDebugRendererContext& context, const WFormatString& text, const WVec2I32& vPositionInPixel, const WColor& color, WUInt32 uiSizeInPixel = 16, WDebugTextHAlign::Enum horizontalAlignment = WDebugTextHAlign::Left, WDebugTextVAlign::Enum verticalAlignment = WDebugTextVAlign::Top);

  /// Draws a piece of text in one of the screen corners.
  ///
  /// Text positioning is automatic, all lines are placed in each corner such that they don't overlap.
  /// Text from different corners may overlap, though.
  ///
  /// For text formatting options, see Draw2DText().
  ///
  /// The \a groupName parameter is used to insert whitespace between unrelated pieces of text,
  /// it is not displayed anywhere, though.
  ///
  /// Text size cannot be changed.
  static void DrawInfoText(const WDebugRendererContext& context, WDebugTextPlacement::Enum placement, WStringView sGroupName, const WFormatString& text, const WColor& color = WColor::White);

  /// Same as DrawInfoText but displays the text for a certain duration.
  static void AddPersistentInfoText(const WDebugRendererContext& context, WDebugTextPlacement::Enum placement, const WFormatString& text, WTime duration, const WColor& color = WColor::White);

  /// Displays a string in 3D space for one frame.
  static WUInt32 Draw3DText(const WDebugRendererContext& context, const WFormatString& text, const WVec3& vGlobalPosition, const WColor& color, WUInt32 uiSizeInPixel = 16, WDebugTextHAlign::Enum horizontalAlignment = WDebugTextHAlign::Center, WDebugTextVAlign::Enum verticalAlignment = WDebugTextVAlign::Bottom);

  /// Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const WDebugRendererContext& context, float fSize, const WColor& color, WMatOrTransform mTransform, WTime duration);

  /// Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const WDebugRendererContext& context, float fRadius, const WColor& color, WMatOrTransform mTransform, WTime duration);

  /// Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const WDebugRendererContext& context, const WVec3& vHalfSize, const WColor& color, WMatOrTransform mTransform, WTime duration);

  /// Renders lines at the given location for as many frames until \a duration has passed.
  static void AddPersistentLines(const WDebugRendererContext& context, WArrayPtr<const WDebugRendererLine> lines, const WColor& color, WMatOrTransform mTransform, WTime duration);

  /// Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  static void DrawAngle(const WDebugRendererContext& context, WAngle startAngle, WAngle endAngle, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform, WVec3 vForwardAxis = WVec3::MakeAxisX(), WVec3 vRotationAxis = WVec3::MakeAxisZ());

  /// Renders a cone with the tip at the center position, opening up with the given angle.
  static void DrawOpeningCone(const WDebugRendererContext& context, WAngle halfAngle, const WColor& colorInside, const WColor& colorOutside, WMatOrTransform mTransform, WVec3 vForwardAxis = WVec3::MakeAxisX());

  /// Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  static void DrawLimitCone(const WDebugRendererContext& context, WAngle halfAngle1, WAngle halfAngle2, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform);

  /// Renders a cylinder starting at the center position, along the +X axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  static void DrawCylinder(const WDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const WColor& solidColor, const WColor& lineColor, WMatOrTransform mTransform, bool bCapStart = false, bool bCapEnd = false, WBasisAxis::Enum cylinderAxis = WBasisAxis::PositiveX);

  /// Renders a line arrow.
  static void DrawArrow(const WDebugRendererContext& context, float fSize, const WColor& color, WMatOrTransform mTransform, WVec3 vForwardAxis = WVec3::MakeAxisX());

  /// Returns the width of single glyph in pixels for the given text size
  static float GetTextGlyphWidth(WUInt32 uiSizeInPixel = 16);

  /// Returns the line height in pixels for the given text size
  static float GetTextLineHeight(WUInt32 uiSizeInPixel = 16);

  /// Returns the global debug text scale
  static float GetTextScale();

  /// Sets the global debug text scale
  static void SetTextScale(float fScale);

private:
  friend class WSimpleRenderPass;

  static void RenderScreenSpace(const WRenderViewContext& renderViewContext);
  static void RenderInternalScreenSpace(const WDebugRendererContext& context, const WRenderViewContext& renderViewContext);

  static void RenderWorldSpace(const WRenderViewContext& renderViewContext);
  static void RenderInternalWorldSpace(const WDebugRendererContext& context, const WRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

/// Helper class to expose debug rendering to scripting
class W_RENDERERCORE_DLL WScriptExtensionClass_Debug
{
public:
  /// Returns the resolution of the first main view that it can find.
  static WVec2 GetResolution();

  static void DrawCross(const WWorld* pWorld, const WVec3& vPosition, float fSize, const WColor& color, const WTransform& transform);
  static void DrawLineBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform);
  static void DrawLineSphere(const WWorld* pWorld, const WVec3& vPosition, float fRadius, const WColor& color, const WTransform& transform);

  static void DrawSolidBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform);

  static void Draw2DText(const WWorld* pWorld, WStringView sText, const WVec3& vPositionInPixel, const WColor& color, WUInt32 uiSizeInPixel, WEnum<WDebugTextHAlign> horizontalAlignment);
  static void Draw3DText(const WWorld* pWorld, WStringView sText, const WVec3& vPosition, const WColor& color, WUInt32 uiSizeInPixel);
  static void DrawInfoText(const WWorld* pWorld, WStringView sText, WEnum<WDebugTextPlacement> placement, WStringView sGroupName, const WColor& color);

  static void AddPersistentCross(const WWorld* pWorld, const WVec3& vPosition, float fSize, const WColor& color, const WTransform& transform, WTime duration);
  static void AddPersistentLineBox(const WWorld* pWorld, const WVec3& vPosition, const WVec3& vHalfExtents, const WColor& color, const WTransform& transform, WTime duration);
  static void AddPersistentLineSphere(const WWorld* pWorld, const WVec3& vPosition, float fRadius, const WColor& color, const WTransform& transform, WTime duration);

  static void DrawLine(const WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, const WColor& startColor, const WColor& endColor);

  static void Draw2DLine(const WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, const WColor& startColor, const WColor& endColor);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WScriptExtensionClass_Debug);

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
