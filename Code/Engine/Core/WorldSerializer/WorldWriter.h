#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/TagSet.h>

/// Stores an entire WWorld in a stream.
///
/// Used for exporting a world in binary form either as a level or as a prefab (though there is no
/// difference).
/// Can be used for saving a game, if the exact state of the world shall be stored (e.g. like in an FPS).
class W_CORE_DLL WWorldWriter
{
public:
  /// Writes all content in \a world to \a stream.
  ///
  /// All game objects with tags that overlap with \a pExclude will be ignored.
  void WriteWorld(WStreamWriter& inout_stream, WWorld& ref_world, const WTagSet* pExclude = nullptr);

  /// Only writes the given root objects and all their children to the stream.
  void WriteObjects(WStreamWriter& inout_stream, const WDeque<const WGameObject*>& rootObjects);

  /// Only writes the given root objects and all their children to the stream.
  void WriteObjects(WStreamWriter& inout_stream, WArrayPtr<const WGameObject*> rootObjects);

  /// Writes the given game object handle to the stream.
  ///
  /// \note If the handle belongs to an object that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteGameObjectHandle(const WGameObjectHandle& hObject);

  /// Writes the given component handle to the stream.
  ///
  /// \note If the handle belongs to a component that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteComponentHandle(const WComponentHandle& hComponent);

  /// Accesses the stream to which data is written. Use this in component serialization functions
  /// to write data to the stream.
  WStreamWriter& GetStream() const { return *m_pStream; }

  /// Returns an array containing all game object pointers that were written to the stream as root objects
  const WDeque<const WGameObject*>& GetAllWrittenRootObjects() const { return m_AllRootObjects; }

  /// Returns an array containing all game object pointers that were written to the stream as child objects
  const WDeque<const WGameObject*>& GetAllWrittenChildObjects() const { return m_AllChildObjects; }

private:
  void Clear();
  WResult WriteToStream();
  void AssignGameObjectIndices();
  void AssignComponentHandleIndices(const WMap<WString, const WRTTI*>& sortedTypes);
  void IncludeAllComponentBaseTypes();
  void IncludeAllComponentBaseTypes(const WRTTI* pRtti);
  void Traverse(WGameObject* pObject);

  WVisitorExecution::Enum ObjectTraverser(WGameObject* pObject);
  void WriteGameObject(const WGameObject* pObject);
  void WriteComponentTypeInfo(const WRTTI* pRtti);
  void WriteComponentCreationData(const WDeque<const WComponent*>& components);
  void WriteComponentSerializationData(const WDeque<const WComponent*>& components);

  WStreamWriter* m_pStream = nullptr;
  const WTagSet* m_pExclude = nullptr;

  WDeque<const WGameObject*> m_AllRootObjects;
  WDeque<const WGameObject*> m_AllChildObjects;
  WMap<WGameObjectHandle, WUInt32> m_WrittenGameObjectHandles;

  struct Components
  {
    WUInt16 m_uiSerializedTypeIndex = 0;
    WDeque<const WComponent*> m_Components;
    WMap<WComponentHandle, WUInt32> m_HandleToIndex;
  };

  WHashTable<const WRTTI*, Components> m_AllComponents;
};
