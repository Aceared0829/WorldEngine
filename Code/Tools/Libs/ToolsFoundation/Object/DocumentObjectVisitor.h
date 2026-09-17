#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Delegate.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocumentObjectManager;
class WDocumentObject;

/// Implements visitor pattern for content of the document object manager.
class W_TOOLSFOUNDATION_DLL WDocumentObjectVisitor
{
public:
  /// Constructor
  ///
  /// \param pManager
  ///   Manager that will be iterated through.
  /// \param szChildrenProperty
  ///   Name of the property that is used for finding children on an object.
  /// \param szRootProperty
  ///   Same as szChildrenProperty, but for the root object of the document.
  WDocumentObjectVisitor(
    const WDocumentObjectManager* pManager, WStringView sChildrenProperty = "Children", WStringView sRootProperty = "Children");

  using VisitorFunction = WDelegate<bool(const WDocumentObject*)>;
  /// Executes depth first traversal starting at the given node.
  ///
  /// \param pObject
  ///   Object to start traversal at.
  /// \param bVisitStart
  ///   If true, function will be executed for the start object as well.
  /// \param function
  ///   Functions executed for each visited object. Should true if the object's children should be traversed.
  void Visit(const WDocumentObject* pObject, bool bVisitStart, VisitorFunction function);

private:
  void TraverseChildren(const WDocumentObject* pObject, WStringView sProperty, VisitorFunction& function);

  const WDocumentObjectManager* m_pManager = nullptr;
  WString m_sChildrenProperty;
  WString m_sRootProperty;
};
