#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <Utilities/FileFormats/OBJLoader.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGizmoHandle, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Visible", m_bVisible),
    W_MEMBER_PROPERTY("Transformation", m_Transformation),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEngineGizmoHandle, 1, WRTTIDefaultAllocator<WEngineGizmoHandle>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("HandleType", m_iHandleType),
    W_MEMBER_PROPERTY("HandleMesh", m_sGizmoHandleMesh),
    W_MEMBER_PROPERTY("Color", m_Color),
    W_MEMBER_PROPERTY("ConstantSize", m_bConstantSize),
    W_MEMBER_PROPERTY("AlwaysOnTop", m_bAlwaysOnTop),
    W_MEMBER_PROPERTY("Visualizer", m_bVisualizer),
    W_MEMBER_PROPERTY("Ortho", m_bShowInOrtho),
    W_MEMBER_PROPERTY("Pickable", m_bIsPickable),
    W_MEMBER_PROPERTY("FaceCam", m_bFaceCamera),
    W_ARRAY_MEMBER_PROPERTY("Lines", m_Lines),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WGizmoHandle::WGizmoHandle()
{
  m_Transformation.SetIdentity();
  m_Transformation.m_vScale.SetZero(); // make sure it is different from anything valid
}

void WGizmoHandle::SetVisible(bool bVisible)
{
  if (bVisible != m_bVisible)
  {
    m_bVisible = bVisible;
    SetModified(true);
  }
}

void WGizmoHandle::SetTransformation(const WTransform& m)
{
  if (m_Transformation != m)
  {
    m_Transformation = m;
    if (m_bVisible)
      SetModified(true);
  }
}

void WGizmoHandle::SetTransformation(const WMat4& m)
{
  WTransform t = WTransform::MakeFromMat4(m);
  SetTransformation(t);
}

static WMeshBufferResourceHandle CreateMeshBufferResource(WGeometry& inout_geom, const char* szResourceName, const char* szDescription, WGALPrimitiveTopology::Enum topology)
{
  inout_geom.ComputeFaceNormals();
  inout_geom.ComputeSmoothVertexNormals();

  WMeshBufferResourceDescriptor desc;
  desc.AddCommonStreams();
  desc.AddStream(WMeshVertexStreamType::Color0);
  desc.AllocateStreamsFromGeometry(inout_geom, topology);
  desc.ComputeBounds();

  return WResourceManager::CreateResource<WMeshBufferResource>(szResourceName, std::move(desc), szDescription);
}

static WMeshBufferResourceHandle CreateMeshBufferArrow()
{
  const char* szResourceName = "{B9DC6776-38D8-4C1F-994F-225E69E71283}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength = 1.0f;

  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));

  WGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(WVec3(fLength * 0.5f, 0, 0));
  geom.AddCone(fThickness * 3.0f, fThickness * 6.0f, true, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Arrow", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferPiston()
{
  const char* szResourceName = "{E2B59B8F-8F61-48C0-AE37-CF31107BA2CE}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.02f;
  const float fLength = 1.0f;

  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));

  WGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(WVec3(fLength * 0.5f, 0, 0));
  geom.AddBox(WVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Piston", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferHalfPiston()
{
  const char* szResourceName = "{BA17D025-B280-4940-8DFD-5486B0E4B41B}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fThickness = 0.04f;
  const float fLength = 1.0f;

  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));
  opt.m_Transform.SetTranslationVector(WVec3(fLength * 0.5f, 0, 0));

  WGeometry geom;
  geom.AddCylinderOnePiece(fThickness, fThickness, fLength * 0.5f, fLength * 0.5f, 16, opt);

  opt.m_Transform.SetTranslationVector(WVec3(fLength, 0, 0));
  geom.AddBox(WVec3(fThickness * 5.0f), false, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfPiston", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferRect()
{
  const char* szResourceName = "{75597E89-CDEE-4C90-A377-9441F64B9DB2}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  // weird size because of translate gizmo, should be fixed through scaling there instead
  const float fLength = 2.0f / 3.0f;

  WGeometry geom;
  geom.AddRect(WVec2(fLength));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Rect", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferLineRect()
{
  const char* szResourceName = "{A1EA52B0-DA73-4176-B50D-3470DDB053F8}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WMat4 m;
  m.SetIdentity();

  WGeometry geom;

  const WVec2 halfSize(1.0f);

  geom.AddVertex(m, WVec3(-halfSize.x, -halfSize.y, 0), WVec3(0, 0, 1), WVec2(0, 1));
  geom.AddVertex(m, WVec3(halfSize.x, -halfSize.y, 0), WVec3(0, 0, 1), WVec2(0, 0));
  geom.AddVertex(m, WVec3(halfSize.x, halfSize.y, 0), WVec3(0, 0, 1), WVec2(1, 0));
  geom.AddVertex(m, WVec3(-halfSize.x, halfSize.y, 0), WVec3(0, 0, 1), WVec2(1, 1));

  geom.AddLine(0, 1);
  geom.AddLine(1, 2);
  geom.AddLine(2, 3);
  geom.AddLine(3, 0);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineRect", WGALPrimitiveTopology::Lines);
}

static WMeshBufferResourceHandle CreateMeshBufferRing()
{
  const char* szResourceName = "{EA8677E3-F623-4FD8-BFAB-349CE1BEB3CA}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  const float fInnerRadius = 1.3f;
  const float fOuterRadius = fInnerRadius + 0.1f;

  WMat4 m;
  m.SetIdentity();

  WGeometry geom;
  geom.AddTorus(fInnerRadius, fOuterRadius, 32, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Ring", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferBox()
{
  const char* szResourceName = "{F14D4CD3-8F21-442B-B07F-3567DBD58A3F}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddBox(WVec3(1.0f), false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Box", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferLineBox()
{
  const char* szResourceName = "{55DF000E-EE88-4BDC-8A7B-FA496941064E}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddLineBox(WVec3(1.0f));

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineBox", WGALPrimitiveTopology::Lines);
}

static WMeshBufferResourceHandle CreateMeshBufferSphere()
{
  const char* szResourceName = "{A88779B0-4728-4411-A9D7-532AFE6F4704}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddGeodesicSphere(1.0f, 2);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Sphere", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferCylinderZ()
{
  const char* szResourceName = "{3BBE2251-0DE4-4B71-979E-A407D8F5CB59}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddCylinderOnePiece(1.0f, 1.0f, 0.5f, 0.5f, 16);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_CylinderZ", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferLineCylinderZ()
{
  const char* szResourceName = "{6978C491-0E1B-4471-A2A1-0CBEFFEBDAC5}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddLineCylinder(1.0f, 1.0f, 0.5f, 0.5f, 16);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_LineCylinderZ", WGALPrimitiveTopology::Lines);
}

static WMeshBufferResourceHandle CreateMeshBufferHalfSphereZ()
{
  const char* szResourceName = "{05BDED8B-96C1-4F2E-8F1B-5C07B3C28D22}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  geom.AddHalfSphere(1.0f, 16, 8, false);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_HalfSphereZ", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferBoxFaces()
{
  const char* szResourceName = "{BD925A8E-480D-41A6-8F62-0AC5F72DA4F6}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry geom;
  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4::MakeTranslation(WVec3(0, 0, 0.5f));

  geom.AddRect(WVec2(0.5f), 1, 1, opt);

  opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(180.0));
  opt.m_Transform.SetTranslationVector(WVec3(0, 0, -0.5f));
  geom.AddRect(WVec2(0.5f), 1, 1, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxFaces", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferBoxEdges()
{
  const char* szResourceName = "{FE700F28-514E-4193-A0F6-4351E0BAC222}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WMat4 rot;

  WGeometry geom;
  WGeometry::GeoOptions opt;

  for (WUInt32 i = 0; i < 4; ++i)
  {
    rot = WMat4::MakeRotationY(WAngle::MakeFromDegree(90.0f * i));

    opt.m_Transform = WMat4::MakeTranslation(WVec3(0.5f - 0.125f, 0, 0.5f));
    opt.m_Transform = rot * opt.m_Transform;
    geom.AddRect(WVec2(0.25f, 0.5f), 1, 1, opt);

    opt.m_Transform = WMat4::MakeTranslation(WVec3(-0.5f + 0.125f, 0, 0.5f));
    geom.AddRect(WVec2(0.25f, 0.5f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxEdges", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferBoxCorners()
{
  const char* szResourceName = "{FBDB6A82-D4B0-447F-815B-228D340451CB}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WMat4 rot[6];
  rot[0].SetIdentity();
  rot[1] = WMat4::MakeRotationX(WAngle::MakeFromDegree(90));
  rot[2] = WMat4::MakeRotationX(WAngle::MakeFromDegree(180));
  rot[3] = WMat4::MakeRotationX(WAngle::MakeFromDegree(270));
  rot[4] = WMat4::MakeRotationY(WAngle::MakeFromDegree(90));
  rot[5] = WMat4::MakeRotationY(WAngle::MakeFromDegree(-90));

  WGeometry geom;
  WGeometry::GeoOptions opt;

  for (WUInt32 i = 0; i < 6; ++i)
  {
    opt.m_Transform = WMat4::MakeTranslation(WVec3(0.5f - 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(WVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = WMat4::MakeTranslation(WVec3(0.5f - 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(WVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = WMat4::MakeTranslation(WVec3(-0.5f + 0.125f, 0.5f - 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(WVec2(0.25f, 0.25f), 1, 1, opt);

    opt.m_Transform = WMat4::MakeTranslation(WVec3(-0.5f + 0.125f, -0.5f + 0.125f, 0.5f));
    opt.m_Transform = rot[i] * opt.m_Transform;
    geom.AddRect(WVec2(0.25f, 0.25f), 1, 1, opt);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_BoxCorners", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferCone()
{
  const char* szResourceName = "{BED97C9E-4E7A-486C-9372-1FB1A5FAE786}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WGeometry::GeoOptions opt;
  opt.m_Transform = WMat4::MakeRotationY(WAngle::MakeFromDegree(270.0f));
  opt.m_Transform.SetTranslationVector(WVec3(1.0f, 0, 0));

  WGeometry geom;
  geom.AddCone(1.0f, 1.0f, false, 16, opt);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Cone", WGALPrimitiveTopology::Triangles);
}

static WMeshBufferResourceHandle CreateMeshBufferFrustum()
{
  const char* szResourceName = "{61A7BE38-797D-4BFC-AED6-33CE4F4C6FF6}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WMat4 m;
  m.SetIdentity();

  WGeometry geom;

  geom.AddVertex(m, WVec3(0, 0, 0), WVec3(0, 0, 1));

  geom.AddVertex(m, WVec3(1.0f, -1.0f, 1.0f), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(1.0f, 1.0f, 1.0f), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(1.0f, -1.0f, -1.0f), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(1.0f, 1.0f, -1.0f), WVec3(0, 0, 1));

  geom.AddLine(0, 1);
  geom.AddLine(0, 2);
  geom.AddLine(0, 3);
  geom.AddLine(0, 4);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Frustum", WGALPrimitiveTopology::Lines);
}

static WMeshBufferResourceHandle CreateMeshBufferCross()
{
  const char* szResourceName = "{3D2A1B4C-8F5E-4D7A-B963-2C1E4F0A8B7D}";

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WMat4 m;
  m.SetIdentity();

  WGeometry geom;

  // X axis
  geom.AddVertex(m, WVec3(-1.0f, 0, 0), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(1.0f, 0, 0), WVec3(0, 0, 1));
  // Y axis
  geom.AddVertex(m, WVec3(0, -1.0f, 0), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(0, 1.0f, 0), WVec3(0, 0, 1));
  // Z axis
  geom.AddVertex(m, WVec3(0, 0, -1.0f), WVec3(0, 0, 1));
  geom.AddVertex(m, WVec3(0, 0, 1.0f), WVec3(0, 0, 1));

  geom.AddLine(0, 1);
  geom.AddLine(2, 3);
  geom.AddLine(4, 5);

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_Cross", WGALPrimitiveTopology::Lines);
}

static WMeshBufferResourceHandle CreateMeshBufferFromFile(const char* szFile)
{
  const char* szResourceName = szFile;

  WMeshBufferResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshBufferResource>(szResourceName);

  if (hMesh.IsValid())
    return hMesh;

  WOBJLoader obj;
  obj.LoadOBJ(szFile, true).AssertSuccess("Couldn't load gizmo model '{}'", szFile);

  WMat4 m;
  m.SetIdentity();

  WGeometry geom;
  for (WUInt32 v = 0; v < obj.m_Positions.GetCount(); ++v)
  {
    geom.AddVertex(obj.m_Positions[v], WVec3::MakeZero(), WVec2::MakeZero(), WColor::White);
  }

  WStaticArray<WUInt32, 3> triangle;
  triangle.SetCount(3);
  for (WUInt32 f = 0; f < obj.m_Faces.GetCount(); ++f)
  {
    triangle[0] = obj.m_Faces[f].m_Vertices[0].m_uiPositionID;
    triangle[1] = obj.m_Faces[f].m_Vertices[1].m_uiPositionID;
    triangle[2] = obj.m_Faces[f].m_Vertices[2].m_uiPositionID;

    geom.AddPolygon(triangle, false);
  }

  return CreateMeshBufferResource(geom, szResourceName, "GizmoHandle_FromFile", WGALPrimitiveTopology::Triangles);
}

static WMeshResourceHandle CreateMeshResource(const char* szMeshResourceName, WMeshBufferResourceHandle hMeshBuffer, const char* szMaterial)
{
  const WStringBuilder sIdentifier(szMeshResourceName, "-with-", szMaterial);

  WMeshResourceHandle hMesh = WResourceManager::GetExistingResource<WMeshResource>(sIdentifier);

  if (hMesh.IsValid())
    return hMesh;

  WResourceLock<WMeshBufferResource> pMeshBuffer(hMeshBuffer, WResourceAcquireMode::AllowLoadingFallback);

  WMeshResourceDescriptor md;
  md.UseExistingMeshBuffer(hMeshBuffer);
  md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
  md.SetMaterial(0, szMaterial);
  md.ComputeBounds();

  return WResourceManager::GetOrCreateResource<WMeshResource>(sIdentifier, std::move(md), pMeshBuffer->GetResourceDescription());
}

WEngineGizmoHandle::WEngineGizmoHandle() = default;

WEngineGizmoHandle::~WEngineGizmoHandle()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

void WEngineGizmoHandle::ConfigureHandle(WGizmo* pParentGizmo, WEngineGizmoHandleType type, const WColor& col, WBitflags<WGizmoFlags> flags, const char* szCustomMesh)
{
  SetParentGizmo(pParentGizmo);

  m_iHandleType = (int)type;
  m_sGizmoHandleMesh = szCustomMesh;
  m_Color = col;

  m_bConstantSize = flags.IsSet(WGizmoFlags::ConstantSize);
  m_bAlwaysOnTop = flags.IsSet(WGizmoFlags::OnTop);
  m_bVisualizer = flags.IsSet(WGizmoFlags::Visualizer);
  m_bShowInOrtho = flags.IsSet(WGizmoFlags::ShowInOrtho);
  m_bIsPickable = flags.IsSet(WGizmoFlags::Pickable);
  m_bFaceCamera = flags.IsSet(WGizmoFlags::FaceCamera);
}

bool WEngineGizmoHandle::SetupForEngine(WWorld* pWorld, WUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;

  if (!m_hGameObject.IsInvalidated())
    return false;

  WMeshBufferResourceHandle hMeshBuffer;
  const char* szMeshGuid = "";

  switch (m_iHandleType)
  {
    case WEngineGizmoHandleType::Arrow:
    {
      hMeshBuffer = CreateMeshBufferArrow();
      szMeshGuid = "{9D02CF27-7A15-44EA-A372-C417AF2A8E9B}";
    }
    break;
    case WEngineGizmoHandleType::Rect:
    {
      hMeshBuffer = CreateMeshBufferRect();
      szMeshGuid = "{3DF4DDDA-F598-4A37-9691-D4C3677905A8}";
    }
    break;
    case WEngineGizmoHandleType::LineRect:
    {
      hMeshBuffer = CreateMeshBufferLineRect();
      szMeshGuid = "{96129543-897C-4DEE-922D-931BC91C5725}";
    }
    break;
    case WEngineGizmoHandleType::Ring:
    {
      hMeshBuffer = CreateMeshBufferRing();
      szMeshGuid = "{629AD0C6-C81B-4850-A5BC-41494DC0BF95}";
    }
    break;
    case WEngineGizmoHandleType::Box:
    {
      hMeshBuffer = CreateMeshBufferBox();
      szMeshGuid = "{13A59253-4A98-4638-8B94-5AA370E929A7}";
    }
    break;
    case WEngineGizmoHandleType::Piston:
    {
      hMeshBuffer = CreateMeshBufferPiston();
      szMeshGuid = "{44A4FE37-6AE3-44C1-897D-E8B95AE53EF6}";
    }
    break;
    case WEngineGizmoHandleType::HalfPiston:
    {
      hMeshBuffer = CreateMeshBufferHalfPiston();
      szMeshGuid = "{64A45DD0-D7F9-4D1D-9F68-782FA3274200}";
    }
    break;
    case WEngineGizmoHandleType::Sphere:
    {
      hMeshBuffer = CreateMeshBufferSphere();
      szMeshGuid = "{FC322E80-5EB0-452F-9D8E-9E65FCFDA652}";
    }
    break;
    case WEngineGizmoHandleType::CylinderZ:
    {
      hMeshBuffer = CreateMeshBufferCylinderZ();
      szMeshGuid = "{893384EA-2F43-4265-AF75-662E2C81C167}";
    }
    break;
    case WEngineGizmoHandleType::LineCylinderZ:
    {
      hMeshBuffer = CreateMeshBufferLineCylinderZ();
      szMeshGuid = "{F2131237-9D5D-4067-AF66-A5C9180BDF39}";
    }
    break;
    case WEngineGizmoHandleType::HalfSphereZ:
    {
      hMeshBuffer = CreateMeshBufferHalfSphereZ();
      szMeshGuid = "{0FC9B680-7B6B-40B6-97BD-CBFFA47F0EFF}";
    }
    break;
    case WEngineGizmoHandleType::BoxCorners:
    {
      hMeshBuffer = CreateMeshBufferBoxCorners();
      szMeshGuid = "{89CCC389-11D5-43F4-9C18-C634EE3154B9}";
    }
    break;
    case WEngineGizmoHandleType::BoxEdges:
    {
      hMeshBuffer = CreateMeshBufferBoxEdges();
      szMeshGuid = "{21508253-2E74-44CE-9399-523214BE7C3D}";
    }
    break;
    case WEngineGizmoHandleType::BoxFaces:
    {
      hMeshBuffer = CreateMeshBufferBoxFaces();
      szMeshGuid = "{FD1A3C29-F8F0-42B0-BBB0-D0A2B28A65A0}";
    }
    break;
    case WEngineGizmoHandleType::LineBox:
    {
      hMeshBuffer = CreateMeshBufferLineBox();
      szMeshGuid = "{4B136D72-BF43-4C4B-96D7-51C5028A7006}";
    }
    break;
    case WEngineGizmoHandleType::Cone:
    {
      hMeshBuffer = CreateMeshBufferCone();
      szMeshGuid = "{9A48962D-127A-445C-899A-A054D6AD8A9A}";
    }
    break;
    case WEngineGizmoHandleType::Frustum:
    {
      szMeshGuid = "{22EC5D48-E8BE-410B-8EAD-51B7775BA058}";
      hMeshBuffer = CreateMeshBufferFrustum();
    }
    break;
    case WEngineGizmoHandleType::Cross:
    {
      hMeshBuffer = CreateMeshBufferCross();
      szMeshGuid = "{1A3B5C7D-9E2F-4A6B-8C0D-E1F2A3B4C5D6}";
    }
    break;
    case WEngineGizmoHandleType::FromFile:
    {
      szMeshGuid = m_sGizmoHandleMesh;
      hMeshBuffer = CreateMeshBufferFromFile(m_sGizmoHandleMesh);
    }
    break;

    case WEngineGizmoHandleType::CustomLines:
      // no mesh needed, OnMsgExtractRenderData in WGizmoComponent handles this case
      break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
  }

  WStringBuilder sName;
  sName.SetFormat("Gizmo{0}", m_iHandleType);

  WGameObjectDesc god;
  god.m_LocalPosition = m_Transformation.m_vPosition;
  god.m_LocalRotation = m_Transformation.m_qRotation;
  god.m_LocalScaling = m_Transformation.m_vScale;
  god.m_sName.Assign(sName.GetData());
  god.m_bDynamic = true;

  WGameObject* pObject;
  m_hGameObject = pWorld->CreateObject(god, pObject);

  if (!m_bShowInOrtho)
  {
    const WTag& tagNoOrtho = WTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    pObject->SetTag(tagNoOrtho);
  }

  {
    const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    pObject->SetTag(tagEditor);
  }

  WGizmoComponent::CreateComponent(pObject, m_pGizmoComponent);
  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->m_bIsPickable = m_bIsPickable;
  m_pGizmoComponent->SetUniqueID(uiNextComponentPickingID);

  if (hMeshBuffer.IsValid())
  {
    WMeshResourceHandle hMesh;

    if (m_bVisualizer)
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/Visualizer.WMaterial");
    }
    else if (m_bConstantSize)
    {
      if (m_bFaceCamera)
      {
        hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSizeCamFacing.WMaterial");
      }
      else
      {
        hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandleConstantSize.WMaterial");
      }
    }
    else
    {
      hMesh = CreateMeshResource(szMeshGuid, hMeshBuffer, "Editor/Materials/GizmoHandle.WMaterial");
    }

    m_pGizmoComponent->SetMesh(hMesh);
  }

  return true;
}

void WEngineGizmoHandle::UpdateForEngine(WWorld* pWorld)
{
  if (m_hGameObject.IsInvalidated())
    return;

  WGameObject* pObject;
  if (!pWorld->TryGetObject(m_hGameObject, pObject))
    return;

  pObject->SetLocalPosition(m_Transformation.m_vPosition);
  pObject->SetLocalRotation(m_Transformation.m_qRotation);
  pObject->SetLocalScaling(m_Transformation.m_vScale);

  m_pGizmoComponent->m_GizmoColor = m_Color;
  m_pGizmoComponent->SetActiveFlag(m_bVisible);
  m_pGizmoComponent->m_Lines = m_Lines;
}

void WEngineGizmoHandle::SetColor(const WColor& col)
{
  m_Color = col;
  SetModified();
}

void WEngineGizmoHandle::SetLines(WArrayPtr<const WVec3> lines)
{
  m_Lines = lines;
  SetModified();
}
