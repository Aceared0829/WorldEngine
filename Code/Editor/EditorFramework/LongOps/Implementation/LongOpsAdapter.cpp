#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/LongOps/LongOpsAdapter.h>

W_IMPLEMENT_SINGLETON(WLongOpsAdapter);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, LongOpsAdapter)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager",
    "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    W_DEFAULT_NEW(WLongOpsAdapter);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (WLongOpsAdapter::GetSingleton())
    {
      auto ptr = WLongOpsAdapter::GetSingleton();
      W_DEFAULT_DELETE(ptr);
    }
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WLongOpsAdapter::WLongOpsAdapter()
  : m_SingletonRegistrar(this)
{
  WDocumentManager::s_Events.AddEventHandler(WMakeDelegate(&WLongOpsAdapter::DocumentManagerEventHandler, this));
  WPhantomRttiManager::s_Events.AddEventHandler(WMakeDelegate(&WLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
}

WLongOpsAdapter::~WLongOpsAdapter()
{
  WPhantomRttiManager::s_Events.RemoveEventHandler(WMakeDelegate(&WLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
  WDocumentManager::s_Events.RemoveEventHandler(WMakeDelegate(&WLongOpsAdapter::DocumentManagerEventHandler, this));
}

void WLongOpsAdapter::DocumentManagerEventHandler(const WDocumentManager::Event& e)
{
  if (e.m_Type == WDocumentManager::Event::Type::DocumentOpened)
  {
    const WRTTI* pRttiScene = WRTTI::FindTypeByName("WSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      CheckAllTypes();

      e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(WMakeDelegate(&WLongOpsAdapter::StructureEventHandler, this));

      ObjectAdded(e.m_pDocument->GetObjectManager()->GetRootObject());
    }
  }

  if (e.m_Type == WDocumentManager::Event::Type::DocumentClosing)
  {
    const WRTTI* pRttiScene = WRTTI::FindTypeByName("WSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      WLongOpControllerManager::GetSingleton()->CancelAndRemoveAllOpsForDocument(e.m_pDocument->GetGuid());

      e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WLongOpsAdapter::StructureEventHandler, this));
    }
  }
}

void WLongOpsAdapter::StructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == WDocumentObjectStructureEvent::Type::AfterObjectAdded)
  {
    ObjectAdded(e.m_pObject);
  }

  if (e.m_EventType == WDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    ObjectRemoved(e.m_pObject);
  }
}

void WLongOpsAdapter::PhantomTypeRegistryEventHandler(const WPhantomRttiManagerEvent& e)
{
  const bool bExists = m_TypesWithLongOps.Contains(e.m_pChangedType);

  if (bExists && e.m_Type == WPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    m_TypesWithLongOps.Remove(e.m_pChangedType);
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to remove
  }

  if (!bExists && e.m_Type == WPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetAttributeByType<WLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(e.m_pChangedType);
      // if this ever becomes relevant:
      // iterate over all open documents and figure out which long ops to add
    }
  }

  if (e.m_Type == WPhantomRttiManagerEvent::Type::TypeChanged)
  {
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to add or remove
  }
}

void WLongOpsAdapter::CheckAllTypes()
{
  WRTTI::ForEachType(
    [&](const WRTTI* pRtti)
    {
      if (pRtti->GetAttributeByType<WLongOpAttribute>() != nullptr)
      {
        m_TypesWithLongOps.Insert(pRtti);
      }
    });
}

void WLongOpsAdapter::ObjectAdded(const WDocumentObject* pObject)
{
  const WRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<WComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const WPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = WDynamicCast<const WLongOpAttribute*>(pAttr))
          {
            WLongOpControllerManager::GetSingleton()->RegisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }

    return;
  }

  if (pRtti->IsDerivedFrom<WGameObject>() || pObject->GetParent() == nullptr /*document root object*/)
  {
    for (const WDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectAdded(pChild);
    }
  }
}

void WLongOpsAdapter::ObjectRemoved(const WDocumentObject* pObject)
{
  const WRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<WComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const WPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = WDynamicCast<const WLongOpAttribute*>(pAttr))
          {
            WLongOpControllerManager::GetSingleton()->UnregisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
  }
  else if (pRtti->IsDerivedFrom<WGameObject>())
  {
    for (const WDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectRemoved(pChild);
    }
  }
}
