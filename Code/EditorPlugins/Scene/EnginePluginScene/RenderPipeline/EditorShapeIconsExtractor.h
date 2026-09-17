#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Textures/Texture2DResource.h>

class WSceneContext;

class WEditorShapeIconsExtractor : public WExtractor
{
  W_ADD_DYNAMIC_REFLECTION(WEditorShapeIconsExtractor, WExtractor);

public:
  WEditorShapeIconsExtractor(const char* szName = "EditorShapeIconsExtractor");
  ~WEditorShapeIconsExtractor();

  virtual void Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData) override {}

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  void SetSceneContext(WSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  WSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void ExtractShapeIcon(const WGameObject* pObject, const WView& view, const WRenderDataManager* pRenderDataManager, WExtractedRenderData& extractedRenderData, WRenderData::Category category);
  const WTypedMemberProperty<WColor>* FindColorProperty(const WRTTI* pRtti) const;
  const WTypedMemberProperty<WColorGammaUB>* FindColorGammaProperty(const WRTTI* pRtti) const;
  void FillShapeIconInfo();

  float m_fSize;
  float m_fMaxScreenSize;
  WSceneContext* m_pSceneContext;

  struct ShapeIconInfo
  {
    WTexture2DResourceHandle m_hTexture;
    const WTypedMemberProperty<WColor>* m_pColorProperty;
    const WTypedMemberProperty<WColorGammaUB>* m_pColorGammaProperty;
    WColor m_FallbackColor = WColor::White;
    bool m_bAlwaysVisible = false;
  };

  WHashTable<const WRTTI*, ShapeIconInfo> m_ShapeIconInfos;
};
