#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/RenderData.h>

class WStreamWriter;

class W_RENDERERCORE_DLL WExtractor : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WExtractor, WReflectedClass);
  W_DISALLOW_COPY_AND_ASSIGN(WExtractor);

public:
  WExtractor(const char* szName);
  virtual ~WExtractor();

  /// Sets the name of the extractor.
  void SetName(const char* szName);

  /// returns the name of the extractor.
  const char* GetName() const;

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) = 0;

  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) = 0;

  virtual WResult Serialize(WStreamWriter& inout_stream) const;
  virtual WResult Deserialize(WStreamReader& inout_stream);

protected:
  /// returns true if the given object should be filtered by view tags.
  bool FilterByViewTags(const WView& view, const WGameObject* pObject) const;

  /// extracts the render data for the given object.
  void ExtractRenderData(const WView& view, const WGameObject* pObject, WMsgExtractRenderData& msg, WExtractedRenderData& extractedRenderData) const;

private:
  friend class WRenderPipeline;
  friend class WRenderPipelinePassGraph;

  bool m_bActive;

  WHashedString m_sName;

protected:
  WHybridArray<WHashedString, 4> m_DependsOn;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  mutable WUInt32 m_uiNumCachedRenderData;
  mutable WUInt32 m_uiNumUncachedRenderData;
#endif
};


class W_RENDERERCORE_DLL WVisibleObjectsExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WVisibleObjectsExtractor, WExtractor);

public:
  WVisibleObjectsExtractor(const char* szName = "VisibleObjectsExtractor");
  ~WVisibleObjectsExtractor();

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;
};

class W_RENDERERCORE_DLL WSelectedObjectsExtractorBase : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WSelectedObjectsExtractorBase, WExtractor);

public:
  WSelectedObjectsExtractorBase(const char* szName = "SelectedObjectsExtractor");
  ~WSelectedObjectsExtractorBase();

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual const WDeque<WGameObjectHandle>* GetSelection() = 0;

  WRenderData::Category m_OverrideCategory;
};

/// Stores a list of game objects that should get highlighted by the renderer.
///
/// Store an instance somewhere in your game code:
/// WSelectedObjectsContext m_SelectedObjects;
/// Add handles to game object that should be get the highlighting outline (as the editor uses for selected objects).
/// On an WView call:
/// WView::SetExtractorProperty("HighlightObjects", "SelectionContext", &m_SelectedObjects);
/// The first name must be the name of an WSelectedObjectsExtractor that is instantiated by the render pipeline.
///
/// As long as there is also an WSelectionHighlightPass in the render pipeline, all objects in this selection will be rendered
/// with an outline.
class W_RENDERERCORE_DLL WSelectedObjectsContext : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSelectedObjectsContext, WReflectedClass);

public:
  WSelectedObjectsContext();
  ~WSelectedObjectsContext();

  void RemoveDeadObjects(const WWorld& world);
  void AddObjectAndChildren(const WWorld& world, const WGameObjectHandle& hObject);
  void AddObjectAndChildren(const WWorld& world, const WGameObject* pObject);

  bool m_bEnabled = true; ///< allows to temporarily 
  WDeque<WGameObjectHandle> m_Objects;
};

/// An extractor that can be instantiated in a render pipeline, to define manually which objects should be rendered with a selection outline.
///
/// \sa WSelectedObjectsContext
class W_RENDERERCORE_DLL WSelectedObjectsExtractor : public WSelectedObjectsExtractorBase
{
  W_ADD_DYNAMIC_REFLECTION(WSelectedObjectsExtractor, WSelectedObjectsExtractorBase);

public:
  WSelectedObjectsExtractor(const char* szName = "ExplicitlySelectedObjectsExtractor");
  ~WSelectedObjectsExtractor();

  virtual const WDeque<WGameObjectHandle>* GetSelection() override;
  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  /// The context is typically set through an WView, through WView::SetExtractorProperty("<name>", "SelectionContext", pointer);
  void SetSelectionContext(WSelectedObjectsContext* pSelectionContext) { m_pSelectionContext = pSelectionContext; } // [ property ]
  WSelectedObjectsContext* GetSelectionContext() const { return m_pSelectionContext; }                              // [ property ]

private:
  WSelectedObjectsContext* m_pSelectionContext = nullptr;
};
