#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class WVisualizerAttribute;
class WDocumentObject;
struct WDocumentObjectPropertyEvent;
struct WQtDocumentWindowEvent;
class WObjectAccessorBase;

/// Base class for the editor side code that sets up a 'visualizer' for object properties.
///
/// Typically visualizers are configured with WVisualizerAttribute's on component types.
/// The adapter reads the attribute values and sets up the necessary code to render them in the engine.
/// This is usually achieved by creating WEngineGizmoHandle objects (which get automatically synchronized
/// with the engine process).
/// The adapter then reacts to editor side object changes and adjusts the engine side representation
/// as needed.
class W_EDITORFRAMEWORK_DLL WVisualizerAdapter
{
public:
  WVisualizerAdapter();
  virtual ~WVisualizerAdapter();

  void SetVisualizer(const WVisualizerAttribute* pAttribute, const WDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const WQtDocumentWindowEvent& e);
  void DocumentObjectMetaDataEventHandler(const WObjectMetaData<WUuid, WDocumentObjectMetaData>::EventData& e);

protected:
  virtual WTransform GetObjectTransform() const;
  WObjectAccessorBase* GetObjectAccessor() const;
  const WAbstractProperty* GetProperty(const char* szProperty) const;

  /// Called to actually properly set up the adapter. All setup code is implemented here.
  virtual void Finalize() = 0;
  /// Called when object properties have changed and the visualizer may need to react.
  virtual void Update() = 0;
  /// Called when the object has been moved somehow. More light weight than a full update.
  virtual void UpdateGizmoTransform() = 0;

  bool m_bVisualizerIsVisible;
  const WVisualizerAttribute* m_pVisualizerAttr;
  const WDocumentObject* m_pObject;
};
