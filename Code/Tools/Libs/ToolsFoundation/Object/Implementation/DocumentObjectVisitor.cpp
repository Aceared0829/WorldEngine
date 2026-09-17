#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

WDocumentObjectVisitor::WDocumentObjectVisitor(
  const WDocumentObjectManager* pManager, WStringView sChildrenProperty /*= "Children"*/, WStringView sRootProperty /*= "Children"*/)
  : m_pManager(pManager)
  , m_sChildrenProperty(sChildrenProperty)
  , m_sRootProperty(sRootProperty)
{
  const WAbstractProperty* pRootProp = m_pManager->GetRootObject()->GetType()->FindPropertyByName(sRootProperty);
  W_ASSERT_DEV(pRootProp, "Given root property '{0}' does not exist on root object", sRootProperty);
  W_ASSERT_DEV(pRootProp->GetCategory() == WPropertyCategory::Set || pRootProp->GetCategory() == WPropertyCategory::Array,
    "Traverser only works on arrays and sets.");

  // const WAbstractProperty* pChildProp = pRootProp->GetSpecificType()->FindPropertyByName(szChildrenProperty);
  // W_ASSERT_DEV(pChildProp, "Given child property '{0}' does not exist", szChildrenProperty);
  // W_ASSERT_DEV(pChildProp->GetCategory() == WPropertyCategory::Set || pRootProp->GetCategory() == WPropertyCategory::Array, "Traverser
  // only works on arrays and sets.");
}

void WDocumentObjectVisitor::Visit(const WDocumentObject* pObject, bool bVisitStart, VisitorFunction function)
{
  WStringView sProperty = m_sChildrenProperty;
  if (pObject == nullptr || pObject == m_pManager->GetRootObject())
  {
    pObject = m_pManager->GetRootObject();
    sProperty = m_sRootProperty;
  }

  if (!bVisitStart || function(pObject))
  {
    TraverseChildren(pObject, sProperty, function);
  }
}

void WDocumentObjectVisitor::TraverseChildren(const WDocumentObject* pObject, WStringView sProperty, VisitorFunction& function)
{
  const WInt32 iChildren = pObject->GetTypeAccessor().GetCount(sProperty);
  for (WInt32 i = 0; i < iChildren; i++)
  {
    WVariant obj = pObject->GetTypeAccessor().GetValue(sProperty, i);
    W_ASSERT_DEBUG(obj.IsValid() && obj.IsA<WUuid>(), "null obj found during traversal.");
    const WDocumentObject* pChild = m_pManager->GetObject(obj.Get<WUuid>());
    if (function(pChild))
    {
      TraverseChildren(pChild, m_sChildrenProperty, function);
    }
  }
}
