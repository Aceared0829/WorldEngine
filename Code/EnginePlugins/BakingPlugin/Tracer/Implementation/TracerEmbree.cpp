#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

#include <embree3/rtcore.h>

namespace
{
  static RTCDevice s_rtcDevice;
  static WHashTable<WHashedString, RTCScene, WHashHelper<WHashedString>, WStaticsAllocatorWrapper> s_rtcMeshCache;

  const char* rtcErrorCodeToString[] = {
    "RTC_NO_ERROR",
    "RTC_UNKNOWN_ERROR",
    "RTC_INVALID_ARGUMENT",
    "RTC_INVALID_OPERATION",
    "RTC_OUT_OF_MEMORY",
    "RTC_UNSUPPORTED_CPU",
    "RTC_CANCELLED"};

  const char* GetStringFromRTCErrorCode(RTCError code)
  {
    return (code >= 0 && code < W_ARRAY_SIZE(rtcErrorCodeToString)) ? rtcErrorCodeToString[code] : "RTC invalid error code";
  }

  static void ErrorCallback(void* userPtr, RTCError code, const char* str)
  {
    WLog::Error("Embree: {}: {}", GetStringFromRTCErrorCode(code), str);
  }

  static WResult InitDevice()
  {
    if (s_rtcDevice == nullptr)
    {
      if (s_rtcDevice = rtcNewDevice("threads=1"))
      {
        WLog::Info("Created new Embree Device (Version {})", RTC_VERSION_STRING);

        rtcSetDeviceErrorFunction(s_rtcDevice, &ErrorCallback, nullptr);

        bool bRay4Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY4_SUPPORTED);
        bool bRay8Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY8_SUPPORTED);
        bool bRay16Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY16_SUPPORTED);
        bool bRayStreamSupported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_RAY_STREAM_SUPPORTED);

        WLog::Info("Supported ray packets: Ray4:{}, Ray8:{}, Ray16:{}, RayStream:{}", bRay4Supported, bRay8Supported, bRay16Supported, bRayStreamSupported);
      }
      else
      {
        WLog::Error("Failed to create Embree Device. Error: {}", GetStringFromRTCErrorCode(rtcGetDeviceError(nullptr)));
        return W_FAILURE;
      }
    }

    return W_SUCCESS;
  }

  static void DeinitDevice()
  {
    for (auto it : s_rtcMeshCache)
    {
      rtcReleaseScene(it.Value());
    }
    s_rtcMeshCache.Clear();

    rtcReleaseDevice(s_rtcDevice);
    s_rtcDevice = nullptr;
  }

  static RTCScene GetOrCreateMesh(const WCpuMeshResourceHandle& hMeshResource)
  {
    WHashedString sResourceId;
    sResourceId.Assign(hMeshResource.GetResourceID());

    RTCScene scene = nullptr;
    if (s_rtcMeshCache.TryGetValue(sResourceId, scene))
    {
      return scene;
    }

    WResourceLock<WCpuMeshResource> pCpuMesh(hMeshResource, WResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != WResourceAcquireResult::Final)
    {
      WLog::Warning("Failed to retrieve CPU mesh '{}'", sResourceId);
      return nullptr;
    }

    RTCGeometry triangleMesh = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    {
      const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

      const WVec3* pPositions = mbDesc.GetPositionData().GetPtr();

      WUInt32 uiNormalStride = 0;
      const WUInt8* pNormals = mbDesc.GetNormalData(&uiNormalStride).GetPtr();
      WGALResourceFormat::Enum normalFormat = mbDesc.GetVertexStreamConfig().GetNormalFormat();

      WVec3* rtcPositions = static_cast<WVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(WVec3), mbDesc.GetVertexCount()));

      rtcSetGeometryVertexAttributeCount(triangleMesh, 1);
      WVec3* rtcNormals = static_cast<WVec3*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, sizeof(WVec3), mbDesc.GetVertexCount()));

      // write out all vertices
      WVec3 vNormal;
      for (WUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
      {
        WMeshBufferUtils::DecodeNormal(WMakeArrayPtr(pNormals, sizeof(WVec3)), normalFormat, vNormal).IgnoreResult();

        rtcPositions[i] = *pPositions;
        rtcNormals[i] = vNormal;

        ++pPositions;
        pNormals = WMemoryUtils::AddByteOffset(pNormals, uiNormalStride);
      }

      WVec3U32* rtcIndices = static_cast<WVec3U32*>(rtcSetNewGeometryBuffer(triangleMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(WVec3U32), mbDesc.GetPrimitiveCount()));

      bool flip = false;
      if (mbDesc.Uses32BitIndices())
      {
        const WUInt32* pTypedIndices = reinterpret_cast<const WUInt32*>(mbDesc.GetIndexBufferData().GetPtr());

        for (WUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }
      else
      {
        const WUInt16* pTypedIndices = reinterpret_cast<const WUInt16*>(mbDesc.GetIndexBufferData().GetPtr());

        for (WUInt32 p = 0; p < mbDesc.GetPrimitiveCount(); ++p)
        {
          rtcIndices[p].x = pTypedIndices[p * 3 + (flip ? 2 : 0)];
          rtcIndices[p].y = pTypedIndices[p * 3 + 1];
          rtcIndices[p].z = pTypedIndices[p * 3 + (flip ? 0 : 2)];
        }
      }

      rtcCommitGeometry(triangleMesh);
    }

    scene = rtcNewScene(s_rtcDevice);
    {
      W_VERIFY(rtcAttachGeometry(scene, triangleMesh) == 0, "Geometry id must be 0");
      rtcReleaseGeometry(triangleMesh);

      rtcCommitScene(scene);
    }

    s_rtcMeshCache.Insert(sResourceId, scene);
    return scene;
  }

} // namespace

struct WTracerEmbree::Data
{
  ~Data()
  {
    ClearScene();
  }

  void ClearScene()
  {
    m_rtcInstancedGeometry.Clear();

    if (m_rtcScene != nullptr)
    {
      rtcReleaseScene(m_rtcScene);
      m_rtcScene = nullptr;
    }
  }

  RTCScene m_rtcScene = nullptr;

  struct InstancedGeometry
  {
    RTCGeometry m_mesh;
    WSimdVec4f m_normalTransform0;
    WSimdVec4f m_normalTransform1;
    WSimdVec4f m_normalTransform2;
  };

  WDynamicArray<InstancedGeometry, WAlignedAllocatorWrapper> m_rtcInstancedGeometry;
};

WTracerEmbree::WTracerEmbree()
{
  m_pData = W_DEFAULT_NEW(Data);
}

WTracerEmbree::~WTracerEmbree() = default;

WResult WTracerEmbree::BuildScene(const WBakingScene& scene)
{
  W_SUCCEED_OR_RETURN(InitDevice());

  m_pData->ClearScene();
  m_pData->m_rtcScene = rtcNewScene(s_rtcDevice);

  for (auto& meshObject : scene.GetMeshObjects())
  {
    RTCScene mesh = GetOrCreateMesh(meshObject.m_hMeshResource);
    if (mesh == nullptr)
    {
      continue;
    }

    WMat4 transform = meshObject.m_GlobalTransform.GetAsMat4();

    RTCGeometry instance = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);
    {
      rtcSetGeometryInstancedScene(instance, mesh);
      rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform);

      rtcCommitGeometry(instance);
    }

    WUInt32 uiInstanceID = rtcAttachGeometry(m_pData->m_rtcScene, instance);
    rtcReleaseGeometry(instance);

    WMat3 normalTransform = transform.GetRotationalPart().GetInverse(0.0f).GetTranspose();

    W_ASSERT_DEBUG(uiInstanceID == m_pData->m_rtcInstancedGeometry.GetCount(), "");
    auto& instancedGeometry = m_pData->m_rtcInstancedGeometry.ExpandAndGetRef();
    instancedGeometry.m_mesh = rtcGetGeometry(mesh, 0);
    instancedGeometry.m_normalTransform0 = WSimdConversion::ToVec3(normalTransform.GetColumn(0));
    instancedGeometry.m_normalTransform1 = WSimdConversion::ToVec3(normalTransform.GetColumn(1));
    instancedGeometry.m_normalTransform2 = WSimdConversion::ToVec3(normalTransform.GetColumn(2));
  }

  rtcCommitScene(m_pData->m_rtcScene);

  return W_SUCCESS;
}

W_DEFINE_AS_POD_TYPE(RTCRayHit);

void WTracerEmbree::TraceRays(WArrayPtr<const Ray> rays, WArrayPtr<Hit> hits)
{
  const WUInt32 uiNumRays = rays.GetCount();

  WHybridArray<RTCRayHit, 256, WAlignedAllocatorWrapper> rtcRayHits;
  rtcRayHits.SetCountUninitialized(uiNumRays);

  for (WUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& ray = rays[i];
    auto& rtcRayHit = rtcRayHits[i];

    rtcRayHit.ray.org_x = ray.m_vStartPos.x;
    rtcRayHit.ray.org_y = ray.m_vStartPos.y;
    rtcRayHit.ray.org_z = ray.m_vStartPos.z;
    rtcRayHit.ray.tnear = 0.0f;

    rtcRayHit.ray.dir_x = ray.m_vDir.x;
    rtcRayHit.ray.dir_y = ray.m_vDir.y;
    rtcRayHit.ray.dir_z = ray.m_vDir.z;
    rtcRayHit.ray.time = 0.0f;

    rtcRayHit.ray.tfar = ray.m_fDistance;
    rtcRayHit.ray.mask = 0;
    rtcRayHit.ray.id = i;
    rtcRayHit.ray.flags = 0;

    rtcRayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
  }

  RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  rtcIntersect1M(m_pData->m_rtcScene, &context, rtcRayHits.GetData(), uiNumRays, sizeof(RTCRayHit));

  for (WUInt32 i = 0; i < uiNumRays; ++i)
  {
    auto& rtcRayHit = rtcRayHits[i];
    auto& ray = rays[i];
    auto& hit = hits[i];

    if (rtcRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
    {
      auto& instancedGeometry = m_pData->m_rtcInstancedGeometry[rtcRayHit.hit.instID[0]];

      WSimdVec4f objectSpaceNormal;
      rtcInterpolate0(instancedGeometry.m_mesh, rtcRayHit.hit.primID, rtcRayHit.hit.u, rtcRayHit.hit.v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, reinterpret_cast<float*>(&objectSpaceNormal), 3);

      WSimdVec4f worldSpaceNormal = instancedGeometry.m_normalTransform0 * objectSpaceNormal.x();
      worldSpaceNormal += instancedGeometry.m_normalTransform1 * objectSpaceNormal.y();
      worldSpaceNormal += instancedGeometry.m_normalTransform2 * objectSpaceNormal.z();

      hit.m_vNormal = WSimdConversion::ToVec3(worldSpaceNormal.GetNormalized<3>());
      hit.m_fDistance = rtcRayHit.ray.tfar;
      hit.m_vPosition = ray.m_vStartPos + ray.m_vDir * hit.m_fDistance;
    }
    else
    {
      hit.m_vPosition.SetZero();
      hit.m_vNormal.SetZero();
      hit.m_fDistance = -1.0f;
    }
  }
}
