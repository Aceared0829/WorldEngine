#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <GameEngine/Animation/PropertyAnimResource.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

struct WGameObjectContextEvent;
class WPropertyAnimObjectAccessor;
class WPropertyAnimAssetDocument;
struct WCommandHistoryEvent;

class WPropertyAnimationTrack : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPropertyAnimationTrack, WReflectedClass);

public:
  WString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  WString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  WString m_sPropertyPath;
  WEnum<WPropertyAnimTarget> m_Target;

  WSingleCurveData m_FloatCurve;
  WColorGradientAssetData m_ColorGradient;
};

class WPropertyAnimationTrackGroup : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPropertyAnimationTrackGroup, WReflectedClass);

public:
  WPropertyAnimationTrackGroup() = default;
  WPropertyAnimationTrackGroup(const WPropertyAnimationTrackGroup&) = delete;
  WPropertyAnimationTrackGroup& operator=(const WPropertyAnimationTrackGroup& rhs) = delete;
  ~WPropertyAnimationTrackGroup();

  WUInt32 m_uiFramesPerSecond = 60;
  WUInt64 m_uiCurveDuration = 480;
  WEnum<WPropertyAnimMode> m_Mode;
  WDynamicArray<WPropertyAnimationTrack*> m_Tracks;
  WEventTrackData m_EventTrack;
};

struct WPropertyAnimAssetDocumentEvent
{
  enum class Type
  {
    AnimationLengthChanged,
    ScrubberPositionChanged,
    PlaybackChanged,
  };

  const WPropertyAnimAssetDocument* m_pDocument;
  Type m_Type;
};

class WPropertyAnimAssetDocument : public WSimpleAssetDocument<WPropertyAnimationTrackGroup, WGameObjectContextDocument>
{
  using BaseClass = WSimpleAssetDocument<WPropertyAnimationTrackGroup, WGameObjectContextDocument>;
  W_ADD_DYNAMIC_REFLECTION(WPropertyAnimAssetDocument, BaseClass);

public:
  WPropertyAnimAssetDocument(WStringView sDocumentPath);
  ~WPropertyAnimAssetDocument();

  void SetAnimationDurationTicks(WUInt64 uiNumTicks);
  WUInt64 GetAnimationDurationTicks() const;
  WTime GetAnimationDurationTime() const;
  void AdjustDuration();

  bool SetScrubberPosition(WUInt64 uiTick);
  WUInt64 GetScrubberPosition() const { return m_uiScrubberTickPos; }

  WEvent<const WPropertyAnimAssetDocumentEvent&> m_PropertyAnimEvents;

  void SetPlayAnimation(bool bPlay);
  bool GetPlayAnimation() const { return m_bPlayAnimation; }
  void SetRepeatAnimation(bool bRepeat);
  bool GetRepeatAnimation() const { return m_bRepeatAnimation; }
  void ExecuteAnimationPlaybackStep();

  const WPropertyAnimationTrack* GetTrack(const WUuid& trackGuid) const;
  WPropertyAnimationTrack* GetTrack(const WUuid& trackGuid);

  WStatus CanAnimate(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target) const;

  WUuid FindTrack(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target) const;
  WUuid CreateTrack(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target);

  WUuid FindCurveCp(const WUuid& trackGuid, WInt64 iTickX);
  WUuid InsertCurveCpAt(const WUuid& trackGuid, WInt64 iTickX, double fNewPosY);

  WUuid FindGradientColorCp(const WUuid& trackGuid, WInt64 iTickX);
  WUuid InsertGradientColorCpAt(const WUuid& trackGuid, WInt64 iTickX, const WColorGammaUB& color);

  WUuid FindGradientAlphaCp(const WUuid& trackGuid, WInt64 iTickX);
  WUuid InsertGradientAlphaCpAt(const WUuid& trackGuid, WInt64 iTickX, WUInt8 uiAlpha);

  WUuid FindGradientIntensityCp(const WUuid& trackGuid, WInt64 iTickX);
  WUuid InsertGradientIntensityCpAt(const WUuid& trackGuid, WInt64 iTickX, float fIntensity);

  WUuid InsertEventTrackCpAt(WInt64 iTickX, const char* szValue);

  virtual WManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return WManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

private:
  void GameObjectContextEventHandler(const WGameObjectContextEvent& e);
  void TreeStructureEventHandler(const WDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e);

  struct PropertyValue
  {
    WVariant m_InitialValue;
    WHybridArray<WUuid, 3> m_Tracks;
  };
  struct PropertyKeyHash
  {
    W_ALWAYS_INLINE static WUInt32 Hash(const WPropertyReference& key)
    {
      return WHashingUtils::xxHash32(&key.m_Object, sizeof(WUuid)) + WHashingUtils::xxHash32(&key.m_pProperty, sizeof(const WAbstractProperty*)) +
             (WUInt32)key.m_Index.ComputeHash();
    }

    W_ALWAYS_INLINE static bool Equal(const WPropertyReference& a, const WPropertyReference& b)
    {
      return a.m_Object == b.m_Object && a.m_pProperty == b.m_pProperty && a.m_Index == b.m_Index;
    }
  };

  void RebuildMapping();
  void RemoveTrack(const WUuid& track);
  void AddTrack(const WUuid& track);
  WStatus FindTrackKeys(const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath, WDynamicArray<WPropertyReference>& keys) const;
  void GenerateTrackInfo(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WStringBuilder& sObjectSearchSequence, WStringBuilder& sComponentType, WStringBuilder& sPropertyPath) const;
  void ApplyAnimation();
  void ApplyAnimation(const WPropertyReference& key, const PropertyValue& value);

  WHashTable<WPropertyReference, PropertyValue, PropertyKeyHash> m_PropertyTable;
  WHashTable<WUuid, WHybridArray<WPropertyReference, 1>> m_TrackTable;

  bool m_bPlayAnimation = false;
  bool m_bRepeatAnimation = false;
  WTime m_LastFrameTime;
  WUInt64 m_uiScrubberTickPos = 0;
};
