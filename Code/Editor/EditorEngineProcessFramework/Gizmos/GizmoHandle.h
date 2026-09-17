#pragma once

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Math/Mat4.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WWorld;
class WGizmoComponent;
class WGizmo;

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WGizmoHandle : public WEditorEngineSyncObject
{
  W_ADD_DYNAMIC_REFLECTION(WGizmoHandle, WEditorEngineSyncObject);

public:
  WGizmoHandle();

  WGizmo* GetOwnerGizmo() const { return m_pParentGizmo; }

  void SetVisible(bool bVisible);

  void SetTransformation(const WTransform& m);
  void SetTransformation(const WMat4& m);

  const WTransform& GetTransformation() const { return m_Transformation; }

protected:
  bool m_bVisible = false;
  WTransform m_Transformation;

  void SetParentGizmo(WGizmo* pParentGizmo) { m_pParentGizmo = pParentGizmo; }

private:
  WGizmo* m_pParentGizmo = nullptr;
};


enum WEngineGizmoHandleType
{
  Arrow,
  Ring,
  Rect,
  LineRect,
  Box,
  Piston,
  HalfPiston,
  Sphere,
  CylinderZ,
  LineCylinderZ,
  HalfSphereZ,
  BoxCorners,
  BoxEdges,
  BoxFaces,
  LineBox,
  Cone,
  Frustum,
  Cross,
  FromFile,
  CustomLines,
};

struct WGizmoFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    Default = 0,

    ConstantSize = W_BIT(0),
    OnTop = W_BIT(1),
    Visualizer = W_BIT(2),
    ShowInOrtho = W_BIT(3),
    Pickable = W_BIT(4),
    FaceCamera = W_BIT(5),
  };

  struct Bits
  {
    StorageType ConstantSize : 1;
    StorageType OnTop : 1;
    StorageType Visualizer : 1;
    StorageType ShowInOrtho : 1;
    StorageType Pickable : 1;
    StorageType FaceCamera : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WGizmoFlags);

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WEngineGizmoHandle : public WGizmoHandle
{
  W_ADD_DYNAMIC_REFLECTION(WEngineGizmoHandle, WGizmoHandle);

public:
  WEngineGizmoHandle();
  ~WEngineGizmoHandle();

  void ConfigureHandle(WGizmo* pParentGizmo, WEngineGizmoHandleType type, const WColor& col, WBitflags<WGizmoFlags> flags, const char* szCustomMesh = nullptr);

  virtual bool SetupForEngine(WWorld* pWorld, WUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(WWorld* pWorld) override;

  void SetColor(const WColor& col);
  void SetLines(WArrayPtr<const WVec3> lines);

protected:
  bool m_bConstantSize = true;
  bool m_bAlwaysOnTop = false;
  bool m_bVisualizer = false;
  bool m_bShowInOrtho = false;
  bool m_bIsPickable = true;
  bool m_bFaceCamera = false;
  WInt32 m_iHandleType = -1;
  WString m_sGizmoHandleMesh;
  WGameObjectHandle m_hGameObject;
  WGizmoComponent* m_pGizmoComponent = nullptr;
  WColor m_Color = WColor::CornflowerBlue; /* The Original! */
  WWorld* m_pWorld = nullptr;
  WDynamicArray<WVec3> m_Lines;
};
