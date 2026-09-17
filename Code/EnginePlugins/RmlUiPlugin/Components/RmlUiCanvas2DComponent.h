#pragma once

#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using WRmlUiCanvas2DComponentManager = WComponentManagerSimple<class WRmlUiCanvas2DComponent, WComponentUpdateType::Always, WBlockStorageType::Compact, WWorldUpdatePhase::PostTransform>;

class W_RMLUIPLUGIN_DLL WRmlUiCanvas2DComponent : public WRmlUiCanvasComponentBase
{
  W_DECLARE_COMPONENT_TYPE(WRmlUiCanvas2DComponent, WRmlUiCanvasComponentBase, WRmlUiCanvas2DComponentManager);

public:
  WRmlUiCanvas2DComponent();
  ~WRmlUiCanvas2DComponent();

  WRmlUiCanvas2DComponent& operator=(WRmlUiCanvas2DComponent&& rhs);

  virtual void Deinitialize() override;
  virtual void OnActivated() override;

  void Update() final override;

  void SetOffset(const WVec2I32& vOffset);                                   // [ property ]
  const WVec2I32& GetOffset() const { return m_vOffset; }                    // [ property ]

  void SetSize(const WVec2U32& vSize);                                       // [ property ]
  const WVec2U32& GetSize() const { return m_vSize; }                        // [ property ]

  void SetAnchorPoint(const WVec2& vAnchorPoint);                            // [ property ]
  const WVec2& GetAnchorPoint() const { return m_vAnchorPoint; }             // [ property ]

  void SetPassInput(bool bPassInput);                                         // [ property ]
  bool GetPassInput() const { return m_bPassInput; }                          // [ property ]

  void SetCustomScale(float fScale);                                          // [ property ]
  float GetCustomScale() const { return m_fCustomScale; }                     // [ property ]

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const override;
  bool UpdateSizeOffsetAndTexture(WVec2& out_viewSize);

  float m_fCustomScale = 1.0f;
  WVec2I32 m_vOffset = WVec2I32::MakeZero();
  WVec2 m_vAnchorPoint = WVec2::MakeZero();
  bool m_bPassInput = true;

  WVec2 m_vFinalOffset = WVec2::MakeZero();
  WGALTextureHandle m_hTexture;
};
