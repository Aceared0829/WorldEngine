#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>


class WTestDocumentObjectManager : public WDocumentObjectManager
{
public:
  WTestDocumentObjectManager();
  ~WTestDocumentObjectManager();
};


class WTestDocument : public WDocument
{
  W_ADD_DYNAMIC_REFLECTION(WTestDocument, WDocument);

public:
  WTestDocument(WStringView sDocumentPath, bool bUseIPCObjectMirror = false);
  ~WTestDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  void ApplyNativePropertyChangesToObjectManager(WDocumentObject* pObject);
  virtual WDocumentInfo* CreateDocumentInfo() override;

  WDocumentObjectMirror m_ObjectMirror;
  WRttiConverterContext m_Context;



private:
  bool m_bUseIPCObjectMirror;
};
