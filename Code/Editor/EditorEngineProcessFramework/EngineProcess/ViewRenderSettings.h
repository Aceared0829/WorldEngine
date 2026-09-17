#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>

struct W_EDITORENGINEPROCESSFRAMEWORK_DLL WSceneViewPerspective
{
  using StorageType = WUInt8;

  enum Enum
  {
    Orthogonal_Front,
    Orthogonal_Right,
    Orthogonal_Top,
    Perspective,

    Default = Perspective
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_EDITORENGINEPROCESSFRAMEWORK_DLL, WSceneViewPerspective);

struct W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineViewConfig
{
  WViewRenderMode::Enum m_RenderMode = WViewRenderMode::Default;
  WSceneViewPerspective::Enum m_Perspective = WSceneViewPerspective::Default;
  WCameraUsageHint::Enum m_CameraUsageHint = WCameraUsageHint::EditorView;
  bool m_bUseCameraTransformOnDevice = true;

  WCamera m_Camera;
  WEngineViewConfig* m_pLinkedViewConfig = nullptr; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fFov = 0.0f, float fNearPlane = 0.1f, float fFarPlane = 1000.0f);
};
struct W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineViewLightSettingsEvent
{
  enum class Type
  {
    SkyBoxChanged,
    SkyLightChanged,
    SkyLightCubeMapChanged,
    SkyLightIntensityChanged,
    DirectionalLightChanged,
    DirectionalLightAngleChanged,
    DirectionalLightShadowsChanged,
    DirectionalLightIntensityChanged,
    FogChanged,
    DefaultValuesChanged,
  };

  Type m_Type;
};

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineViewLightSettings : public WEditorEngineSyncObject
{
  W_ADD_DYNAMIC_REFLECTION(WEngineViewLightSettings, WEditorEngineSyncObject);

public:
  WEngineViewLightSettings(bool bEnable = true);
  ~WEngineViewLightSettings();

  bool GetSkyBox() const;
  void SetSkyBox(bool bVal);

  bool GetSkyLight() const;
  void SetSkyLight(bool bVal);

  const char* GetSkyLightCubeMap() const;
  void SetSkyLightCubeMap(const char* szVal);

  float GetSkyLightIntensity() const;
  void SetSkyLightIntensity(float fVal);

  bool GetDirectionalLight() const;
  void SetDirectionalLight(bool bVal);

  WAngle GetDirectionalLightAngle() const;
  void SetDirectionalLightAngle(WAngle val);

  bool GetDirectionalLightShadows() const;
  void SetDirectionalLightShadows(bool bVal);

  float GetDirectionalLightIntensity() const;
  void SetDirectionalLightIntensity(float fVal);

  bool GetFog() const;
  void SetFog(bool bVal);

  mutable WEvent<const WEngineViewLightSettingsEvent&> m_EngineViewLightSettingsEvents;

  virtual bool SetupForEngine(WWorld* pWorld, WUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(WWorld* pWorld) override;

private:
  void SetModifiedInternal(WEngineViewLightSettingsEvent::Type type);

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  WString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;

  bool m_bDirectionalLight = true;
  WAngle m_DirectionalLightAngle = WAngle::MakeFromDegree(70.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;

  bool m_bFog = false;

  // Engine side data
  WWorld* m_pWorld = nullptr;
  WGameObjectHandle m_hSkyBoxObject;
  WComponentHandle m_hSkyBox;
  WGameObjectHandle m_hGameObject;
  WComponentHandle m_hDirLight;
  WComponentHandle m_hSkyLight;
  WComponentHandle m_hFog;
};
