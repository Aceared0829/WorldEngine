#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class WRasterizerObject;

/// Base class for all render data. Render data must contain all information that is needed to render the corresponding object.
class W_RENDERERCORE_DLL WRenderData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRenderData, WReflectedClass);

public:
  struct Category
  {
    Category();
    explicit Category(WUInt16 uiValue);

    bool operator==(const Category& other) const;
    bool operator!=(const Category& other) const;
    bool IsValid() const { return m_uiValue != 0xFFFF; }

    WUInt16 m_uiValue = 0xFFFF;
  };

  /// This function generates a 64bit sorting key for the given render data. Data with lower sorting key is rendered first.
  using SortingKeyFunc = WUInt64 (*)(const WRenderData*, const WCamera&);

  static Category RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc);
  static Category RegisterDerivedCategory(const char* szCategoryName, Category baseCategory);
  static Category RegisterRedirectedCategory(const char* szCategoryName, Category staticCategory, Category dynamicCategory);
  static Category FindCategory(WTempHashedString sCategoryName);
  static Category ResolveCategory(Category category, bool bDynamic);

  static WHashedString GetCategoryName(Category category);
  static void GetAllCategoryNames(WDynamicArray<WHashedString>& out_categoryNames);

public:
  struct Caching
  {
    enum Enum
    {
      Never,
      IfStatic
    };
  };

  struct Flags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      Dynamic = W_BIT(0),
      FlipWinding = W_BIT(1),

      Default = 0
    };

    struct Bits
    {
      StorageType Dynamic : 1;
      StorageType FlipWinding : 1;
    };
  };

  bool IsDynamic() const;
  bool IsStatic() const;
  bool FlipWinding() const;

  /// Returns the final sorting for this render data with the given category and camera.
  WUInt64 GetFinalSortingKey(Category category, const WCamera& camera) const;

  /// Returns whether this render data and the other render data can be batched together, e.g. rendered in one draw call.
  /// An implementation can assume that the other render data is of the same type as this render data.
  virtual bool CanBatch(const WRenderData& other) const { return false; }

  WBitflags<Flags> m_Flags;

  WVec3 m_vGlobalPosition = WVec3::MakeZero();
  float m_fSortingDepthOffset = 0.0f;

  WUInt32 m_uiSortingKey = 0;

  WGameObjectHandle m_hOwner;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const WGameObject* m_pOwner = nullptr; ///< Debugging only. It is not allowed to access the game object during rendering.
#endif

private:
  struct CategoryData
  {
    Category m_baseCategory;
    Category m_staticCategory;
    Category m_dynamicCategory;

    WHashedString m_sName;
    SortingKeyFunc m_sortingKeyFunc;
  };

  static WHybridArray<CategoryData, 32> s_CategoryData;
};

/// Base class for render data that make uses of the instance data offset buffer which will be generated during the extraction phase.
class W_RENDERERCORE_DLL WInstanceableRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WInstanceableRenderData, WRenderData);

public:
  struct DataOffsets
  {
    WUInt32 m_uiInstance = 0;
    WUInt32 m_uiCustomInstance = 0;
    WUInt32 m_uiMaterial = 0;
    WUInt32 m_uiSkinning = 0; // TODO: this could be removed if we switch to compute shader skinning
  };

  DataOffsets m_DataOffsets;

  WUInt32 m_uiNumInstances = 1;
  WGALDynamicBufferHandle m_hInstanceDataBuffer;

protected:
  bool CanBatchByBaseValues(const WInstanceableRenderData& other) const;
};

struct W_RENDERERCORE_DLL WDefaultRenderDataCategories
{
  static WRenderData::Category Light;
  static WRenderData::Category Decal;
  static WRenderData::Category ReflectionProbe;
  static WRenderData::Category Sky;
  static WRenderData::Category LitOpaque;
  static WRenderData::Category LitOpaqueStatic;
  static WRenderData::Category LitOpaqueDynamic;
  static WRenderData::Category LitMasked;
  static WRenderData::Category LitMaskedStatic;
  static WRenderData::Category LitMaskedDynamic;
  static WRenderData::Category LitMeshDecal;
  static WRenderData::Category LitTransparent;
  static WRenderData::Category LitForeground;
  static WRenderData::Category LensEffects;
  static WRenderData::Category SimpleOpaque;
  static WRenderData::Category SimpleTransparent;
  static WRenderData::Category SimpleForeground;
  static WRenderData::Category Selection;
  static WRenderData::Category GUI;
};

#define WInvalidRenderDataCategory WRenderData::Category()

struct W_RENDERERCORE_DLL WMsgExtractRenderData : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgExtractRenderData, WMessage);

  const WView* m_pView = nullptr;
  const WRenderDataManager* m_pRenderDataManager = nullptr;
  WRenderData::Category m_OverrideCategory = WInvalidRenderDataCategory;

  /// Adds render data for the current view. This data can be cached depending on the specified caching behavior.
  /// Non-cached data is only valid for this frame. Cached data must be manually deleted using the WRenderWorld::DeleteCachedRenderData
  /// function.
  void AddRenderData(const WRenderData* pRenderData, WRenderData::Category category, WRenderData::Caching::Enum cachingBehavior);

  /// Records that the given texture must be in `requiredState` when `category` is rendered. Invalid handles are ignored so safe to pass in without checking.
  /// Like render data, dependencies are cached for static objects when the component's render data is cached.
  /// \sa WRenderPipelinePass::DeclareRendererDependenciesForCategory
  void AddDependency(WGALTextureHandle hTexture, WRenderData::Category category, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

  /// Records that the given buffer must be in `requiredState` when `category` is rendered. Invalid handles are ignored so safe to pass in without checking.
  /// Like render data, dependencies are cached for static objects when the component's render data is cached.
  /// \sa WRenderPipelinePass::DeclareRendererDependenciesForCategory
  void AddDependency(WGALBufferHandle hBuffer, WRenderData::Category category, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage = WGALShaderStageFlags::Auto);

private:
  friend class WExtractor;

  struct Data
  {
    const WRenderData* m_pRenderData = nullptr;
    WRenderData::Category m_Category;
  };

  WHybridArray<Data, 16> m_ExtractedRenderData;
  WSmallArray<WTextureDependency, 4> m_TextureDependencies;
  WSmallArray<WBufferDependency, 4> m_BufferDependencies;

  WUInt32 m_uiNumCacheIfStatic = 0;
};

struct W_RENDERERCORE_DLL WMsgExtractOccluderData : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgExtractOccluderData, WMessage);

  void AddOccluder(const WRasterizerObject* pObject, const WTransform& transform)
  {
    auto& d = m_ExtractedOccluderData.ExpandAndGetRef();
    d.m_pObject = pObject;
    d.m_Transform = transform;
  }

private:
  friend class WRenderPipeline;

  struct Data
  {
    const WRasterizerObject* m_pObject = nullptr;
    WTransform m_Transform;
  };

  WHybridArray<Data, 16> m_ExtractedOccluderData;
};

struct WInstanceDataOffset
{
  W_DECLARE_POD_TYPE();

  WInstanceDataOffset()
    : m_uiOffset(WMath::Bitmask_LowN<WUInt32>(31))
    , m_uiIsDynamic(0)
  {
  }

  W_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == WMath::Bitmask_LowN<WUInt32>(31); }

  WUInt32 m_uiOffset : 31;
  WUInt32 m_uiIsDynamic : 1;
};

struct WCustomInstanceDataOffset
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE bool IsInvalidated() const { return m_uiOffset == WInvalidIndex; }

  WUInt32 m_uiOffset = WInvalidIndex;
};

struct W_RENDERERCORE_DLL WMsgCustomInstanceDataOffsetChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgCustomInstanceDataOffsetChanged, WMessage);

  WCustomInstanceDataOffset m_NewOffset;
};

#include <RendererCore/Pipeline/Implementation/RenderData_inl.h>
