#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WRandomGauss;

class WDuplicateObjectsCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WDuplicateObjectsCommand, WCommand);

public:
  WDuplicateObjectsCommand();

public:                        // Properties
  WString m_sGraphTextFormat;
  WString m_sParentNodes;     /// A stringyfied map in format "uuidObj1=uuidParent1;..." that defines the previous parents of all top level objects

  WUInt32 m_uiNumberOfCopies; /// if set to 0 (the default), all the 'advanced' duplication code is skipped and only a single straight copy is made

  WVec3 m_vAccumulativeTranslation;
  WVec3 m_vAccumulativeRotation;
  WVec3 m_vRandomRotation;
  WVec3 m_vRandomTranslation;
  bool m_bGroupDuplicates;

  WInt8 m_iRevolveAxis; ///< 0 = disabled, 1 = x, 2 = y, 3 = z
  float m_fRevolveRadius;
  WAngle m_RevolveStartAngle;
  WAngle m_RevolveAngleStep;

  /// When duplicating a single object, this is set to the original object's index + 1, so the duplicate ends up right after the original.
  /// A value of -1 (the default) appends to the end of the parent's children list.
  WInt32 m_iInsertIndex = -1;

private:
  virtual WStatus DoInternal(bool bRedo) override;

  void SetAsSelection();

  void DeserializeGraph(WAbstractObjectGraph& graph);

  void CreateOneDuplicate(WAbstractObjectGraph& graph, WDynamicArray<WDocument::PasteInfo>& out_toBePasted);
  void AdjustObjectPositions(const WArrayPtr<WDocument::PasteInfo>& duplicates, WUInt32 uiNumDuplicate, WRandomGauss& rngRotX, WRandomGauss& rngRotY, WRandomGauss& rngRotZ, WRandomGauss& rngTransX, WRandomGauss& rngTransY, WRandomGauss& rngTransZ);

  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    WDocumentObject* m_pObject;
    WDocumentObject* m_pParent;
    WString m_sParentProperty;
    WVariant m_Index;
    WUInt32 m_uiSelectionOrder = 0;
  };

  WDeque<const WDocumentObject*> m_OriginalSelection;
  WHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};
