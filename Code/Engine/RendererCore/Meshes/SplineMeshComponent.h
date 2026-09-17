#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

#include <Foundation/Containers/ArrayMap.h>

struct WMsgSplineChanged;
struct WMsgExtractGeometry;
struct WMsgGenericEvent;
struct WSpline;
class WAbstractObjectNode;
class WSplineComponent;

/// Determines how many meshes are distributed along the spline and how they are scaled.
struct WSplineMeshDistributionMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    FitToSegment,          ///< Each mesh is stretched to fit exactly into one spline segment.
    ScaleEvenly,           ///< Mesh length is used to determine the number of meshes distributed along the whole spline.
    ScaleEvenlyPerSegment, ///< Mesh length is used to determine the number of meshes distributed along each spline segment. This is useful when parts should align with spline nodes.

    Default = ScaleEvenly
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WSplineMeshDistributionMode);

//////////////////////////////////////////////////////////////////////////

/// Message sent by the WSplineMeshComponent to request collision mesh generation.
struct W_RENDERERCORE_DLL WMsgGenerateSplineMeshCollision : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgGenerateSplineMeshCollision, WMessage);

  WComponentHandle m_hSplineComponent;
  WSmallArray<WMeshResourceHandle, 4> m_RenderMeshes;
  WSmallArray<WVec2, 4> m_ScaleOffsets;
  float m_fLocalOffsetY = 0.0f;
  float m_fLocalOffsetZ = 0.0f;
};

//////////////////////////////////////////////////////////////////////////

struct W_RENDERERCORE_DLL WSplineMeshPart
{
  WMeshResourceHandle m_hMesh;
  float m_fPaddingFront = 0.0f; ///< Adds padding in front of this part. Can be negative to overlap parts.
  float m_fPaddingBack = 0.0f;  ///< Adds padding at the back of this part. Can be negative to overlap parts.

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

  bool IsValid() const
  {
    return m_hMesh.IsValid();
  }
  WResult ComputeLengthAndOffset(WVec2& out_vLengthAndOffset) const;
  WVec2 AddPadding(const WVec2& vLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack) const;
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WSplineMeshPart);

//////////////////////////////////////////////////////////////////////////

using WSplineMeshComponentManager = WComponentManager<class WSplineMeshComponent, WBlockStorageType::Compact>;

/// A component that generates a mesh along a spline using the specified mesh parts.
///
/// The spline is taken from an WSplineComponent on the owner game object or one of its parents.
/// Generation only happens in the editor and the generated mesh is cached to disk so it can be loaded at runtime.
class W_RENDERERCORE_DLL WSplineMeshComponent : public WMeshComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WSplineMeshComponent, WMeshComponentBase, WSplineMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WSplineMeshComponent

public:
  WSplineMeshComponent();
  ~WSplineMeshComponent();

  /// Sets a specialized mesh part to be used at the start of the spline. Can be empty.
  void SetStartPart(const WSplineMeshPart& part);                     // [ property ]
  const WSplineMeshPart& GetStartPart() const { return m_StartPart; } // [ property ]

  /// Sets the list of mesh parts to be used in the middle of the spline. At least one part must be specified.
  ///
  /// If multiple parts are specified either a random one or the best fitting one is chosen,
  /// depending on the distribution mode and the spline and part length.
  void SetMiddleParts(WArrayPtr<const WSplineMeshPart> middleParts);                // [ property ]
  WArrayPtr<const WSplineMeshPart> GetMiddleParts() const { return m_MiddleParts; } // [ property ]

  /// Sets a specialized mesh part to be used at the end of the spline. Can be empty.
  void SetEndPart(const WSplineMeshPart& part);                   // [ property ]
  const WSplineMeshPart& GetEndPart() const { return m_EndPart; } // [ property ]

  /// Sets how the meshes are distributed along the spline.
  void SetDistributionMode(WEnum<WSplineMeshDistributionMode> mode);                            // [ property ]
  WEnum<WSplineMeshDistributionMode> GetDistributionMode() const { return m_DistributionMode; } // [ property ]

  /// Sets the random seed used when selecting middle parts randomly.
  ///
  /// Negative values indicate to use the stable random seed from the owner object.
  /// Positive values or zero specify an explicit seed value.
  void SetSeed(WInt32 iSeed);                // [ property ]
  WInt32 GetSeed() const { return m_iSeed; } // [ property ]

  /// Sets an offset that is applied to each generated mesh vertex in the local Y direction of the spline
  /// effectively moving the mesh to the left or right of the spline.
  void SetOffsetY(float fOffsetY);                // [ property ]
  float GetOffsetY() const { return m_fOffsetY; } // [ property ]

  /// Sets an offset that is applied to each generated mesh vertex in the local Z direction of the spline
  /// effectively moving the mesh up or down relative to the spline.
  void SetOffsetZ(float fOffsetZ);                // [ property ]
  float GetOffsetZ() const { return m_fOffsetZ; } // [ property ]

  /// Helper function to generate a spline mesh descriptor from the given spline and meshes.
  static WResult GenerateSplineMeshDesc(const WSpline& spline, const WArrayMap<float, float>& distanceToKey, WArrayPtr<WCpuMeshResource*> meshes, WArrayPtr<WVec2> scaleOffsets, float fLocalOffsetY, float fLocalOffsetZ, WMeshResourceDescriptor& out_splineMeshDesc);

private:
  WUInt32 MiddleParts_GetCount() const { return m_MiddleParts.GetCount(); }
  const WSplineMeshPart& MiddleParts_GetValue(WUInt32 uiIndex) const { return m_MiddleParts[uiIndex]; }
  void MiddleParts_SetValue(WUInt32 uiIndex, const WSplineMeshPart& value);
  void MiddleParts_Insert(WUInt32 uiIndex, const WSplineMeshPart& value);
  void MiddleParts_Remove(WUInt32 uiIndex);

  void OnObjectCreated(const WAbstractObjectNode& node);
  void OnMsgSplineChanged(WMsgSplineChanged& ref_msg);           // [ msg handler ]
  void OnMsgExtractGeometry(WMsgExtractGeometry& ref_msg) const; // [ msg handler ]
  void OnMsgGenericEvent(WMsgGenericEvent& ref_msg);             // [ msg handler ]

  void GenerateMeshPath(const WSplineComponent& splineComponent, WStringBuilder& out_sSplineMeshPath) const;

  WResult GenerateDistribution(const WSplineComponent& splineComponent, WDynamicArray<WMeshResourceHandle>& out_Meshes, WDynamicArray<WVec2>& out_scaleOffsets) const;
  WUInt32 FindBestMiddlePart(float fRequestedLength, WArrayPtr<const WVec2> middleLengthAndOffset, bool bAllowOverlapFront, bool bAllowOverlapBack, int& inout_iRandomPos, WUInt32 uiSeed) const;

  void UpdateSplineMesh();

  void StartGenerateTask(WSharedPtr<WTask>&& pTask);

  const WSplineComponent* GetSplineComponent() const;

  WSplineMeshPart m_StartPart;
  WSmallArray<WSplineMeshPart, 1> m_MiddleParts;
  WSplineMeshPart m_EndPart;

  WInt32 m_iSeed = -1;
  WEnum<WSplineMeshDistributionMode> m_DistributionMode;

  WUInt16 m_uiLastSplineChangeCounter = 0;

  float m_fOffsetY = 0.0f;
  float m_fOffsetZ = 0.0f;

  WUInt64 m_uiStableId = 0;

  WSharedPtr<WTask> m_pGenerationTask;
  WSharedPtr<WTask> m_pNextGenerationTask;
  WTaskGroupID m_TaskGroupID;
};
