#pragma once

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>

class WSaveDocumentTask final : public WTask
{
public:
  WSaveDocumentTask();
  ~WSaveDocumentTask();

  WDeferredFileWriter file;
  WAbstractObjectGraph headerGraph;
  WAbstractObjectGraph objectGraph;
  WAbstractObjectGraph typesGraph;
  WDocument* m_document = nullptr;

  virtual void Execute() override;
};

class WAfterSaveDocumentTask final : public WTask
{
public:
  WAfterSaveDocumentTask();
  ~WAfterSaveDocumentTask();

  WDocument* m_document = nullptr;
  WDocument::AfterSaveCallback m_callback;

  virtual void Execute() override;
};
