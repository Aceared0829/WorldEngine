#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgGenerateSplineMeshCollision;
class WMeshComponentBase;
class WProcVertexColorComponent;
using WCpuMeshResourceHandle = WTypedResourceHandle<class WCpuMeshResource>;

class W_PROCGENPLUGIN_DLL WProcVertexColorComponentManager : public WComponentManager<WProcVertexColorComponent, WBlockStorageType::Compact>
{
  W_DISALLOW_COPY_AND_ASSIGN(WProcVertexColorComponentManager);

public:
  WProcVertexColorComponentManager(WWorld* pWorld);
  ~WProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class WProcVertexColorComponent;

  struct UpdateContext
  {
    WProcVertexColorComponent* m_pComponent = nullptr;
    WCpuMeshResourceHandle m_hCpuMesh;
    WUInt32 m_uiVertexColorOffset = 0;
  };

  void UpdateVertexColors(const WWorldModule::UpdateContext& context);
  bool UpdateComponentOutputs(WProcVertexColorComponent& component);
  void UpdateComponentVertexColors(const UpdateContext& context, WGALDynamicBuffer& buffer);

  void EnqueueUpdate(WProcVertexColorComponent& component);
  void RemoveComponent(WProcVertexColorComponent& component);

  void OnResourceEvent(const WResourceEvent& resourceEvent);

  void OnAreaInvalidated(const WProcGenInternal::InvalidatedArea& area);

  WGALDynamicBufferHandle GetVertexColorBuffer();

  WDynamicArray<WComponentHandle> m_ComponentsToUpdate;
  WDynamicArray<UpdateContext> m_UpdateContexts;

  WDynamicArray<WSharedPtr<WProcGenInternal::VertexColorTask>> m_UpdateTasks;
  WTaskGroupID m_UpdateTaskGroupID;
  WUInt32 m_uiNextTaskIndex = 0;

  WUInt32 m_uiCustomDataIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct WProcVertexColorOutputDesc
{
  WHashedString m_sName;
  WProcVertexColorMapping m_Mapping;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct WMsgTransformChanged;

class W_PROCGENPLUGIN_DLL WProcVertexColorComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WProcVertexColorComponent, WComponent, WProcVertexColorComponentManager);

public:
  WProcVertexColorComponent();
  ~WProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(WStringView sFile);
  WStringView GetResourceFile() const;

  void SetResource(const WProcGenGraphResourceHandle& hResource);
  const WProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const WProcVertexColorOutputDesc& GetOutputDesc(WUInt32 uiIndex) const;
  void SetOutputDesc(WUInt32 uiIndex, const WProcVertexColorOutputDesc& outputDesc);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  void OnMsgTransformChanged(WMsgTransformChanged& ref_msg);                               // [ msg handler ]
  void OnMsgCustomInstanceDataOffsetChanged(WMsgCustomInstanceDataOffsetChanged& ref_msg); // [ msg handler ]
  void OnMsgGenerateSplineMeshCollision(WMsgGenerateSplineMeshCollision& ref_msg);         // [ msg handler ]

private:
  WUInt32 OutputDescs_GetCount() const;
  void OutputDescs_Insert(WUInt32 uiIndex, const WProcVertexColorOutputDesc& outputDesc);
  void OutputDescs_Remove(WUInt32 uiIndex);

  bool HasValidOutputs() const;

  WMeshComponentBase* GetMeshComponent();

  WProcGenGraphResourceHandle m_hResource;
  WSmallArray<WProcVertexColorOutputDesc, 1> m_OutputDescs;

  WSmallArray<WSharedPtr<const WProcGenInternal::VertexColorOutput>, 1> m_Outputs;

  WCustomInstanceDataOffset m_CustomInstanceDataOffset;
};
