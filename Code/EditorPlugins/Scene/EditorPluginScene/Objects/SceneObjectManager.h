#pragma once

#include <SharedPluginScene/Common/Messages.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WDocument;
class WScene2Document;

class WSceneDocumentSettingsBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSceneDocumentSettingsBase, WReflectedClass);
};

class WPrefabDocumentSettings : public WSceneDocumentSettingsBase
{
  W_ADD_DYNAMIC_REFLECTION(WPrefabDocumentSettings, WSceneDocumentSettingsBase);

public:
  WDynamicArray<WExposedSceneProperty> m_ExposedProperties;
};

class WLayerDocumentSettings : public WSceneDocumentSettingsBase
{
  W_ADD_DYNAMIC_REFLECTION(WLayerDocumentSettings, WSceneDocumentSettingsBase);
};

class WSceneDocumentRoot : public WDocumentRoot
{
  W_ADD_DYNAMIC_REFLECTION(WSceneDocumentRoot, WDocumentRoot);

public:
  WSceneDocumentSettingsBase* m_pSettings;
};

class WSceneObjectManager : public WDocumentObjectManager
{
public:
  WSceneObjectManager();
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override;

private:
  virtual WStatus InternalCanAdd(
    const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const override;
  virtual WStatus InternalCanSelect(const WDocumentObject* pObject) const override;
  virtual WStatus InternalCanMove(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const override;
};
