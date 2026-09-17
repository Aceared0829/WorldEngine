#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class WPropertyAnimAssetDocument;
class WPropertyAnimObjectManager;

class WPropertyAnimObjectAccessor : public WObjectCommandAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WPropertyAnimObjectAccessor, WObjectCommandAccessor);

public:
  WPropertyAnimObjectAccessor(WPropertyAnimAssetDocument* pDoc, WCommandHistory* pHistory);

  virtual WStatus GetValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) override;
  virtual WStatus SetValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;

  virtual WStatus InsertValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;

  virtual WStatus AddObject(const WDocumentObject* pParent, const WAbstractProperty* pParentProp, const WVariant& index, const WRTTI* pType,
    WUuid& inout_objectGuid) override;
  virtual WStatus RemoveObject(const WDocumentObject* pObject) override;
  virtual WStatus MoveObject(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, const WAbstractProperty* pParentProp, const WVariant& index) override;

private:
  bool IsTemporary(const WDocumentObject* pObject) const;
  bool IsTemporary(const WDocumentObject* pParent, const WAbstractProperty* pParentProp) const;
  using OnAddTrack = WDelegate<void(const WUuid&)>;
  WUuid FindOrAddTrack(
    const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target, OnAddTrack onAddTrack);

  WStatus SetCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WPropertyAnimTarget::Enum target,
    double fOldValue, double fNewValue);
  WStatus SetOrInsertCurveCp(const WUuid& track, double fValue);

  WStatus SetColorCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, const WColorGammaUB& oldValue,
    const WColorGammaUB& newValue);
  WStatus SetOrInsertColorCurveCp(const WUuid& track, const WColorGammaUB& value);

  WStatus SetAlphaCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, WUInt8 oldValue, WUInt8 newValue);
  WStatus SetOrInsertAlphaCurveCp(const WUuid& track, WUInt8 value);

  WStatus SetIntensityCurveCp(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index, float oldValue, float newValue);
  WStatus SetOrInsertIntensityCurveCp(const WUuid& track, float value);

  void SeparateColor(const WColor& color, WColorGammaUB& gamma, WUInt8& alpha, float& intensity);

  WUniquePtr<WObjectAccessorBase> m_pObjAccessor;
  WPropertyAnimAssetDocument* m_pDocument = nullptr;
  WPropertyAnimObjectManager* m_pObjectManager = nullptr;
};
