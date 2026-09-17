#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashSet.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct WDocumentObjectStructureEvent;
struct WPhantomRttiManagerEvent;
class WRTTI;

/// This singleton lives in the editor process and monitors all WSceneDocument's for components with the WLongOpAttribute.
///
/// All such components will be automatically registered in the WLongOpControllerManager, such that their functionality
/// is exposed to the user.
///
/// Since this class adapts the components with the WLongOpAttribute to the WLongOpControllerManager, it does not have any public
/// functionality.
class WLongOpsAdapter
{
  W_DECLARE_SINGLETON(WLongOpsAdapter);

public:
  WLongOpsAdapter();
  ~WLongOpsAdapter();

private:
  void DocumentManagerEventHandler(const WDocumentManager::Event& e);
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e);
  void CheckAllTypes();
  void ObjectAdded(const WDocumentObject* pObject);
  void ObjectRemoved(const WDocumentObject* pObject);

  WHashSet<const WRTTI*> m_TypesWithLongOps;
};
