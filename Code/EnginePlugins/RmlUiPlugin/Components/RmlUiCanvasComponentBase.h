#pragma once

#include <RmlUiPlugin/Components/RmlUiMessages.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiInput.h>

#include <Core/Messages/EventMessageSender.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WMsgExtractRenderData;
class WRmlUiContext;
class WRmlUiDataBinding;
class WBlackboard;

using WRmlUiResourceHandle = WTypedResourceHandle<class WRmlUiResource>;

class W_RMLUIPLUGIN_DLL WRmlUiCanvasComponentBase : public WRenderComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WRmlUiCanvasComponentBase, WRenderComponent);

public:
  WRmlUiCanvasComponentBase();
  ~WRmlUiCanvasComponentBase();

  WRmlUiCanvasComponentBase& operator=(WRmlUiCanvasComponentBase&& rhs);

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnDeactivated() override;

  virtual void Update();

  virtual bool ReceiveInput(const WVec2& vMousePosInsideCanvas, WRmlUiInputSnapshot input);

  W_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(RmlResource, m_hResource, SetRmlResource);
  void SetRmlResource(const WRmlUiResourceHandle& hResource);                // [ property ]
  const WRmlUiResourceHandle& GetRmlResource() const { return m_hResource; } // [ property ]

  /// Look for a blackboard component on the owner object and its parent and bind their blackboards during initialization of this component.
  void SetAutobindBlackboards(bool bAutobind);                           // [ property ]
  bool GetAutobindBlackboards() const { return m_bAutobindBlackboards; } // [ property ]

  /// If enabled, the component will send an WMsgRmlUiEventMessage for each RmlUI event that is triggered on the context.
  void SetSendEventMessage(bool bSendEventMessage);                // [ property ]
  bool GetSendEventMessage() const { return m_bSendEventMessage; } // [ property ]

  void SetOnDemandUpdate(bool bOnDemandUpdate);                    // [ property ]
  bool GetOnDemandUpdate() const { return m_bOnDemandUpdate; }     // [ property ]

  WUInt32 AddDataBinding(WUniquePtr<WRmlUiDataBinding>&& pDataBinding);
  void RemoveDataBinding(WUInt32 uiDataBindingIndex);

  /// Adds the given blackboard as data binding. The name of the board is used as model name for the binding.
  WUInt32 AddBlackboardBinding(const WSharedPtr<WBlackboard>& pBlackboard);
  void RemoveBlackboardBinding(WUInt32 uiDataBindingIndex);

  WRmlUiContext* GetOrCreateRmlContext();
  WRmlUiContext* GetRmlContext() { return m_pContext; }

  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

protected:
  virtual void OnMsgReload(WMsgRmlUiReload& msg);                            // [ msg handler ]
  virtual void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const = 0; // [ msg handler ]

  void UpdateCachedValues();
  void UpdateAutobinding();
  void UpdateEventHandler();

  void EventHandler(const WHashedString& sIdentifier, Rml::Event& event);

  WRmlUiResourceHandle m_hResource;
  WEvent<const WResourceEvent&, WMutex>::Unsubscriber m_ResourceEventUnsubscriber;

  WVec2U32 m_vSize = WVec2U32::MakeZero();
  WVec2U32 m_vReferenceResolution = WVec2U32::MakeZero();
  bool m_bAutobindBlackboards = false;
  bool m_bSendEventMessage = false;
  bool m_bOnDemandUpdate = true;
  bool m_bNeedsUpdate = false;
  WUInt16 m_uiContextID = 0;

  WRmlUiContext* m_pContext = nullptr;
  WRmlUiInputProvider m_InputProvider;

  WDynamicArray<WUniquePtr<WRmlUiDataBinding>> m_DataBindings;
  WDynamicArray<WUInt32> m_AutoBindings;

  WEventMessageSender<WMsgRmlUiEvent> m_EventMessageSender; // [ event ]
};
