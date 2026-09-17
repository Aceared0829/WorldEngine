#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RendererCoreDLL.h>

class Rasterizer;
class WRasterizerObject;
class WColorLinearUB;
class WCamera;
class WSimdBBox;

class W_RENDERERCORE_DLL WRasterizerView final
{
  W_DISALLOW_COPY_AND_ASSIGN(WRasterizerView);

public:
  WRasterizerView();
  ~WRasterizerView();

  /// Changes the resolution of the view. Has to be called at least once before starting to render anything.
  void SetResolution(WUInt32 uiWidth, WUInt32 uiHeight, float fAspectRatio);

  WUInt32 GetResolutionX() const { return m_uiResolutionX; }
  WUInt32 GetResolutionY() const { return m_uiResolutionY; }

  /// Prepares the view to rasterize a new scene.
  void BeginScene();

  /// Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(WArrayPtr<WColorLinearUB> targetBuffer) const;

  /// Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const WCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const WRasterizerObject* pObject, const WTransform& transform)
  {
    auto& inst = m_Instances.ExpandAndGetRef();
    inst.m_pObject = pObject;
    inst.m_Transform = transform;
  }

  /// Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const WSimdBBox& aabb) const;

  /// Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(WUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const WTransform& modelTransform);

  bool m_bAnyOccludersRasterized = false;
  const WCamera* m_pCamera = nullptr;
  WUInt32 m_uiResolutionX = 0;
  WUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;
  WUniquePtr<Rasterizer> m_pRasterizer;

  struct Instance
  {
    WTransform m_Transform;
    const WRasterizerObject* m_pObject;
  };

  WDeque<Instance> m_Instances;
  WMat4 m_mViewProjection;
};

class WRasterizerViewPool
{
public:
  WRasterizerView* GetRasterizerView(WUInt32 uiWidth, WUInt32 uiHeight, float fAspectRatio);
  void ReturnRasterizerView(WRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool m_bInUse = false;
    WRasterizerView m_RasterizerView;
  };

  WMutex m_Mutex;
  WDeque<PoolEntry> m_Entries;
};
