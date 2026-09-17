#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class WBlackboard;
class WView;
struct WResourceEvent;

class W_RENDERERCORE_DLL WCameraComponentManager : public WComponentManager<class WCameraComponent, WBlockStorageType::Compact>
{
public:
  WCameraComponentManager(WWorld* pWorld);
  ~WCameraComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const WWorldModule::UpdateContext& context);

  void ReinitializeAllRenderTargetCameras();

  const WCameraComponent* GetCameraByUsageHint(WCameraUsageHint::Enum usageHint) const;
  WCameraComponent* GetCameraByUsageHint(WCameraUsageHint::Enum usageHint);

private:
  friend class WCameraComponent;

  void AddRenderTargetCamera(WCameraComponent* pComponent);
  void RemoveRenderTargetCamera(WCameraComponent* pComponent);

  void OnViewCreated(WView* pView);
  void OnCameraConfigsChanged(void* dummy);

  WDynamicArray<WComponentHandle> m_ModifiedCameras;
  WDynamicArray<WComponentHandle> m_RenderTargetCameras;
};

/// Adds a camera to the scene.
///
/// Cameras have different use cases which are selected through the WCameraUsageHint property.
/// A game needs (exactly) one camera with the usage hint "MainView", since that is what the renderer uses to render the output.
/// Other cameras are optional or for specialized use cases.
///
/// The camera component defines the field-of-view, near and far clipping plane distances,
/// which render pipeline to use, which objects to include and exclude in the rendered image and various other options.
///
/// A camera object may be created and controlled through a player prefab, for example in a first person or third person game.
/// It may also be created by an WGameState and controlled by its game logic, for example in top-down games that don't
/// really have a player object.
///
/// Ultimately camera components don't have functionality, they mostly exist and store some data.
/// It is the game state's decision how the game camera works. By default, the game state iterates over all camera components
/// and picks the best one (usually the "MainView") to place the renderer camera.
class W_RENDERERCORE_DLL WCameraComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WCameraComponent, WComponent, WCameraComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WCameraComponent

public:
  WCameraComponent();
  ~WCameraComponent();

  /// Sets what the camera should be used for.
  void SetUsageHint(WEnum<WCameraUsageHint> val);                      // [ property ]
  WEnum<WCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]

  /// Sets the asset name (or path) to a render target resource, in case this camera should render to texture.
  void SetRenderTargetFile(WStringView sFile); // [ property ]
  WStringView GetRenderTargetFile() const;     // [ property ]

  /// An offset to render only to a part of a texture.
  void SetRenderTargetRectOffset(WVec2 value);                                  // [ property ]
  WVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  /// A size to render only to a part of a texture.
  void SetRenderTargetRectSize(WVec2 value);                                // [ property ]
  WVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  /// Specifies whether the camera should be perspective or orthogonal and how to use the aspect ratio.
  void SetCameraMode(WEnum<WCameraMode> val);                 // [ property ]
  WEnum<WCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]

  /// Configures the distance of the near plane. Objects in front of the near plane get culled and clipped.
  void SetNearPlane(float fVal);                      // [ property ]
  float GetNearPlane() const { return m_fNearPlane; } // [ property ]

  /// Configures the distance of the far plane. Objects behin the far plane get culled and clipped.
  void SetFarPlane(float fVal);                     // [ property ]
  float GetFarPlane() const { return m_fFarPlane; } // [ property ]

  /// Sets the opening angle of the perspective view frustum. Whether this means the horizontal or vertical angle is determined by the camera mode.
  void SetFieldOfView(float fVal);                                   // [ property ]
  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]

  /// Sets the size of the orthogonal view frustum. Whether this means the horizontal or vertical size is determined by the camera mode.
  void SetOrthoDimension(float fVal);                           // [ property ]
  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]

  /// Returns the handle to the render pipeline that is in use.
  WRenderPipelineResourceHandle GetRenderPipeline() const;

  /// Returns the blackboard that is in use.
  WSharedPtr<WBlackboard> GetBlackboard() const;

  /// Returns a handle to the view that the camera renders to.
  WViewHandle GetRenderTargetView() const;

  /// Sets the name of the render pipeline to use.
  void SetRenderPipelineEnum(const char* szFile);                           // [ property ]
  const char* GetRenderPipelineEnum() const;                                // [ property ]

  void SetBlackboardName(const char* szName);                               // [ property ]
  const char* GetBlackboardName() const { return m_sBlackboardName; }       // [ property ]

  void SetAperture(float fAperture);                                        // [ property ]
  float GetAperture() const { return m_fAperture; }                         // [ property ]

  void SetShutterTime(WTime shutterTime);                                  // [ property ]
  WTime GetShutterTime() const { return m_ShutterTime; }                   // [ property ]

  void SetISO(float fISO);                                                  // [ property ]
  float GetISO() const { return m_fISO; }                                   // [ property ]

  void SetExposureCompensation(float fEC);                                  // [ property ]
  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]

  float GetEV100() const;                                                   // [ property ]
  float GetExposure() const;                                                // [ property ]

  /// If non-empty, only objects with these tags will be included in this camera's output.
  WTagSet m_IncludeTags; // [ property ]

  /// If non-empty, objects with these tags will be excluded from this camera's output.
  WTagSet m_ExcludeTags; // [ property ]

  void ApplySettingsToView(WView* pView) const;

private:
  void UpdateRenderTargetCamera();
  void ShowStats(WView* pView);

  void ResourceChangeEventHandler(const WResourceEvent& e);

  WEnum<WCameraUsageHint> m_UsageHint;
  WEnum<WCameraMode> m_Mode;
  WRenderToTexture2DResourceHandle m_hRenderTarget;
  float m_fNearPlane = 0.25f;
  float m_fFarPlane = 1000.0f;
  float m_fPerspectiveFieldOfView = 60.0f;
  float m_fOrthoDimension = 10.0f;
  WRenderPipelineResourceHandle m_hCachedRenderPipeline;

  float m_fAperture = 1.0f;
  WTime m_ShutterTime = WTime::MakeFromSeconds(1.0f);
  float m_fISO = 100.0f;
  float m_fExposureCompensation = 0.0f;

  void MarkAsModified();
  void MarkAsModified(WCameraComponentManager* pCameraManager);

  bool m_bIsModified = false;
  bool m_bShowStats = false;
  bool m_bRenderTargetInitialized = false;

  // -1 for none, 0 to 9 for ALT+Number
  WInt8 m_iEditorShortcut = -1; // [ property ]

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  WViewHandle m_hRenderTargetView;
  WVec2 m_vRenderTargetRectOffset = WVec2(0.0f);
  WVec2 m_vRenderTargetRectSize = WVec2(1.0f);
  WCamera m_RenderTargetCamera;
  WHashedString m_sRenderPipeline;
  WHashedString m_sBlackboardName;

  WSharedPtr<WBlackboard> m_pBlackboard;
};
