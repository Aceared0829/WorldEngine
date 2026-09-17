#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>

/// Reports tag-set entries that are not registered in the project's tag configuration.
///
/// Unknown tags are ignored at runtime, so these are reported as warnings. Auto-fix removes them.
class W_EDITORFRAMEWORK_DLL WUnknownTagsAssetCheckRule : public WAssetCheckRule
{
  W_ADD_DYNAMIC_REFLECTION(WUnknownTagsAssetCheckRule, WAssetCheckRule);

public:
  virtual WStringView GetDisplayName() const override { return "Unknown Tags"; }
  virtual WStringView GetDescription() const override;
  virtual bool CanFix() const override { return true; }

protected:
  virtual void CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp) override;
};

/// Reports properties marked with WRequiredAttribute that are left empty, or, for game object /
/// component reference properties, that reference an object which does not exist (anymore).
///
/// There is no sensible value to auto-fix this with, so these are always reported as errors.
class W_EDITORFRAMEWORK_DLL WRequiredPropertyAssetCheckRule : public WAssetCheckRule
{
  W_ADD_DYNAMIC_REFLECTION(WRequiredPropertyAssetCheckRule, WAssetCheckRule);

public:
  virtual WStringView GetDisplayName() const override { return "Required Properties"; }
  virtual WStringView GetDescription() const override;

protected:
  virtual void CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp) override;
};

/// Reports WGameObjects that have no name, no components and no child objects, i.e. that have no
/// effect on the scene. Auto-fix removes them.
class W_EDITORFRAMEWORK_DLL WEmptyGameObjectAssetCheckRule : public WAssetCheckRule
{
  W_ADD_DYNAMIC_REFLECTION(WEmptyGameObjectAssetCheckRule, WAssetCheckRule);

public:
  virtual WStringView GetDisplayName() const override { return "Empty Game Objects"; }
  virtual WStringView GetDescription() const override;
  virtual bool CanFix() const override { return true; }
  virtual bool AppliesToDocumentType(WStringView sDocumentTypeName) const override;

  virtual void CheckDocument(WAssetCheckContext& ref_ctx) override;

protected:
  virtual void CheckObject(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject) override;

private:
  // Objects are collected while walking the tree and only removed afterwards, since removing an
  // object during the traversal would invalidate the sibling array a parent frame is iterating over.
  WDynamicArray<const WDocumentObject*> m_EmptyObjects;
};
