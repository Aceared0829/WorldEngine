#pragma once

/// \file

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>
#include <Core/World/SpatialData.h>

// Avoid conflicts with windows.h
#ifdef SendMessage
#  undef SendMessage
#endif

/// Defines during re-parenting what transform is going to be preserved.
struct WTransformPreservation
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    PreserveLocal,
    PreserveGlobal,

    Default = PreserveLocal
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WTransformPreservation);

/// This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// WGameObjectHandle instead.
///
/// \see WWorld
/// \see WComponent
/// \see WGameObjectHandle
class W_CORE_DLL WGameObject final
{
private:
  enum
  {
#if W_ENABLED(W_PLATFORM_32BIT)
    NUM_INPLACE_COMPONENTS = 12
#else
    NUM_INPLACE_COMPONENTS = 6
#endif
  };

  friend class WWorld;
  friend class WInternal::WorldData;
  friend class WMemoryUtils;

  WGameObject();
  WGameObject(const WGameObject& other);
  ~WGameObject();

  void operator=(const WGameObject& other);

public:
  /// Iterates over all children of one object.
  ///
  /// Provides read-only access to child game objects. The iterator becomes invalid
  /// when the last child is reached or when the object structure is modified.
  class W_CORE_DLL ConstChildIterator
  {
  public:
    const WGameObject& operator*() const;
    const WGameObject* operator->() const;

    operator const WGameObject*() const;

    /// Advances the iterator to the next child object. The iterator will not be valid anymore, if the last child is reached.
    void Next();

    /// Checks whether this iterator points to a valid object.
    bool IsValid() const;

    /// Shorthand for 'Next'
    void operator++();

  private:
    friend class WGameObject;

    ConstChildIterator(WGameObject* pObject, const WWorld* pWorld);

    WGameObject* m_pObject = nullptr;
    const WWorld* m_pWorld = nullptr;
  };

  /// Mutable iterator for child game objects.
  ///
  /// Extends ConstChildIterator to provide write access to child objects.
  class W_CORE_DLL ChildIterator : public ConstChildIterator
  {
  public:
    WGameObject& operator*();
    WGameObject* operator->();

    operator WGameObject*();

  private:
    friend class WGameObject;

    ChildIterator(WGameObject* pObject, const WWorld* pWorld);
  };

  /// Returns a handle to this object.
  WGameObjectHandle GetHandle() const;

  /// Makes this object and all its children dynamic. Dynamic objects might move during runtime.
  void MakeDynamic();

  /// Makes this object static. Static objects don't move during runtime.
  void MakeStatic();

  /// Returns whether this object is dynamic.
  bool IsDynamic() const;

  /// Returns whether this object is static.
  bool IsStatic() const;

  /// Sets the 'active flag' of the game object, which affects its final 'active state'.
  ///
  /// The active flag affects the 'active state' of the game object and all its children and attached components.
  /// When a game object does not have the active flag, it is switched to 'inactive'. The same happens for all its children and
  /// all components attached to those game objects.
  /// Thus removing the active flag from a game object recursively deactivates the entire sub-tree of objects and components.
  ///
  /// When the active flag is set on a game object, and all of its parent nodes have the flag set as well, then the active state
  /// will be set to true on it and all its children and attached components.
  ///
  /// \sa IsActive(), WComponent::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// Checks whether the 'active flag' is set on this game object. Note that this does not mean that the game object is also in an 'active
  /// state'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// Checks whether this game object is in an active state.
  ///
  /// The active state is determined by the active state of the parent game object and the 'active flag' of this game object.
  /// Only if the parent game object is active (and thus all of its parent objects as well) and this game object has the active flag set,
  /// will this game object be active.
  ///
  /// \sa WGameObject::SetActiveFlag(), WComponent::IsActive()
  bool IsActive() const;

  /// Adds WObjectFlags::CreatedByPrefab to the object. See the flag for details.
  void SetCreatedByPrefab() { m_Flags.Add(WObjectFlags::CreatedByPrefab); }

  /// Checks whether the WObjectFlags::CreatedByPrefab flag is set on this object.
  bool WasCreatedByPrefab() const { return m_Flags.IsSet(WObjectFlags::CreatedByPrefab); }

  /// Adds WObjectFlags::HideShapeIcon to the object. See the flag for details.
  void SetHideShapeIcon() { m_Flags.Add(WObjectFlags::HideShapeIcon); }

  /// Checks whether the WObjectFlags::HideShapeIcon flag is set on this object.
  bool IsShapeIconHidden() const { return m_Flags.IsSet(WObjectFlags::HideShapeIcon); }

  /// Sets the name to identify this object. Does not have to be a unique name.
  void SetName(WStringView sName);
  void SetName(const WHashedString& sName);
  WStringView GetName() const;
  const WHashedString& GetNameHashed() const;
  bool HasName(const WTempHashedString& sName) const;

  /// Sets the global key to identify this object. Global keys must be unique within a world.
  ///
  /// If two objects use the same global key, the last one that registers it will be the referenced object.
  /// To prevent warnings about overwriting global keys, first clear the global key on the previous object.
  void SetGlobalKey(WStringView sGlobalKey);
  void SetGlobalKey(const WHashedString& sGlobalKey);
  WStringView GetGlobalKey() const;

  /// Enables or disabled notification message 'WMsgChildrenChanged' when children are added or removed. The message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

  /// Enables or disabled notification message 'WMsgParentChanged' when the parent changes. The message is sent to this object only.
  void EnableParentChangesNotifications();
  void DisableParentChangesNotifications();

  /// Sets the parent of this object to the given.
  void SetParent(const WGameObjectHandle& hParent, WTransformPreservation::Enum preserve = WTransformPreservation::PreserveGlobal);

  /// Gets the parent of this object or nullptr if this is a top-level object.
  WGameObject* GetParent();

  /// Gets the parent of this object or nullptr if this is a top-level object.
  const WGameObject* GetParent() const;

  /// Adds the given object as a child object.
  void AddChild(const WGameObjectHandle& hChild, WTransformPreservation::Enum preserve = WTransformPreservation::PreserveGlobal);

  /// Adds the given objects as child objects.
  void AddChildren(const WArrayPtr<const WGameObjectHandle>& children, WTransformPreservation::Enum preserve = WTransformPreservation::PreserveGlobal);

  /// Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const WGameObjectHandle& hChild, WTransformPreservation::Enum preserve = WTransformPreservation::PreserveGlobal);

  /// Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const WArrayPtr<const WGameObjectHandle>& children, WTransformPreservation::Enum preserve = WTransformPreservation::PreserveGlobal);

  /// Returns the number of children.
  WUInt32 GetChildCount() const;

  /// Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  WGameObject* FindChildByName(const WTempHashedString& sName, bool bRecursive = true); // [tested]

  /// Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  const WGameObject* FindChildByName(const WTempHashedString& sName, bool bRecursive = true) const; // [tested]

  /// Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  WGameObject* FindChildByPath(WStringView sPath); // [tested]

  /// Const overload of FindChildByPath()
  const WGameObject* FindChildByPath(WStringView sPath) const; // [tested]

  /// Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  WGameObject* SearchForChildByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent = nullptr); // [tested]

  /// Const overload of SearchForChildByNameSequence()
  const WGameObject* SearchForChildByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent = nullptr) const; // [tested]

  /// Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(WStringView sObjectSequence, const WRTTI* pExpectedComponent, WDynamicArray<WGameObject*>& out_objects);

  /// Sets the enabled flag on the child object with the given name and optionally disables all other children.
  void ActivateChildByName(const WTempHashedString& sName, bool bDeactivateOthers = true);


  WWorld* GetWorld();
  const WWorld* GetWorld() const;


  /// Defines update behavior for global transforms when changing the local transform on a static game object
  enum class UpdateBehaviorIfStatic
  {
    None,              ///< Only sets the local transform, does not update
    UpdateImmediately, ///< Updates the hierarchy underneath the object immediately
  };

  /// Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void SetLocalPosition(WVec3 vPosition);
  WVec3 GetLocalPosition() const;

  void SetLocalRotation(WQuat qRotation);
  WQuat GetLocalRotation() const;

  void SetLocalScaling(WVec3 vScaling);
  WVec3 GetLocalScaling() const;

  void SetLocalUniformScaling(float fScaling);
  float GetLocalUniformScaling() const;

  WTransform GetLocalTransform() const;

  void SetGlobalPosition(const WVec3& vPosition);
  WVec3 GetGlobalPosition() const;

  void SetGlobalRotation(const WQuat& qRotation);
  WQuat GetGlobalRotation() const;

  void SetGlobalScaling(const WVec3& vScaling);
  WVec3 GetGlobalScaling() const;

  void SetGlobalTransform(const WTransform& transform);
  WTransform GetGlobalTransform() const;

  /// Last frame's global transform (only valid if W_GAMEOBJECT_VELOCITY is set, otherwise the same as GetGlobalTransform())
  WTransform GetLastGlobalTransform() const;

  // Simd variants of above methods
  void SetLocalPosition(const WSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const WSimdVec4f& GetLocalPositionSimd() const;

  void SetLocalRotation(const WSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const WSimdQuat& GetLocalRotationSimd() const;

  void SetLocalScaling(const WSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const WSimdVec4f& GetLocalScalingSimd() const;

  void SetLocalUniformScaling(const WSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  WSimdFloat GetLocalUniformScalingSimd() const;

  WSimdTransform GetLocalTransformSimd() const;

  void SetGlobalPosition(const WSimdVec4f& vPosition);
  const WSimdVec4f& GetGlobalPositionSimd() const;

  void SetGlobalRotation(const WSimdQuat& qRotation);
  const WSimdQuat& GetGlobalRotationSimd() const;

  void SetGlobalScaling(const WSimdVec4f& vScaling);
  const WSimdVec4f& GetGlobalScalingSimd() const;

  void SetGlobalTransform(const WSimdTransform& transform);
  const WSimdTransform& GetGlobalTransformSimd() const;

  /// Sets the global rotation of this game object such that it 'looks at' the target position.
  ///
  /// Per convention, that means the +X axis will point towards the target position, +Y will point to the right,
  /// and +Z will point upwards.
  ///
  /// vUp is used to calculate the necessary right and up vectors.
  /// A custom vUp vector must be provided, in case the look-at position can be directly above the game object.
  void SetGlobalRotationToLookAt(const WVec3& vTargetPosition, const WVec3& vUp = WVec3::MakeAxisZ());

  /// Same as SetGlobalRotationToLookAt but also changes the position of this object.
  ///
  /// Note that the scale of this object gets set to 1.
  void SetGlobalTransformToLookAt(const WVec3& vOwnPosition, const WVec3& vTargetPosition, const WVec3& vUp = WVec3::MakeAxisZ());

  const WSimdTransform& GetLastGlobalTransformSimd() const;

  /// Returns the 'forwards' direction of the world's WCoordinateSystem, rotated into the object's global space
  WVec3 GetGlobalDirForwards() const;
  /// Returns the 'right' direction of the world's WCoordinateSystem, rotated into the object's global space
  WVec3 GetGlobalDirRight() const;
  /// Returns the 'up' direction of the world's WCoordinateSystem, rotated into the object's global space
  WVec3 GetGlobalDirUp() const;

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  /// The last global transform is used to calculate the object's velocity. By default this is set automatically to the global transform of the last frame.
  ///
  /// It might make sense to manually override the last global transform to e.g. indicate an object has been teleported instead of moved.
  void SetLastGlobalTransform(const WSimdTransform& transform);

  /// Returns the linear velocity of the object in units per second. This is only guaranteed to be correct in the PostTransform phase.
  WVec3 GetLinearVelocity() const;

  /// Returns the angular velocity of the object in radians per second. This is only guaranteed to be correct in the PostTransform phase.
  WVec3 GetAngularVelocity() const;
#endif

  /// Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();

  /// Enables or disabled notification message 'WMsgTransformChanged' when this object is static and its transform changes.
  /// The notification message is sent to this object and thus also to all its components.
  void EnableStaticTransformChangesNotifications();
  void DisableStaticTransformChangesNotifications();


  WBoundingBoxSphere GetLocalBounds() const;
  WBoundingBoxSphere GetGlobalBounds() const;

  const WSimdBBoxSphere& GetLocalBoundsSimd() const;
  const WSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// Schedules a local bounds update to be processed at the end of the current update phase.
  ///
  /// Unlike UpdateLocalBounds(), this function is safe to call from async update functions.
  void QueueLocalBoundsUpdate();

  /// Updates the global bounds immediately. Usually this done during the world update after the "Post-async" phase.
  /// Note that this function does not ensure that the global transform is up-to-date. Use UpdateGlobalTransformAndBounds if you want to update both.
  void UpdateGlobalBounds();

  /// Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// Returns a handle to the internal spatial data.
  WSpatialDataHandle GetSpatialData() const;

  /// Enables or disabled notification message 'WMsgComponentsChanged' when components are added or removed. The message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  [[nodiscard]] bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const WRTTI* pType, WComponent*& out_pComponent);

  /// Tries to find a component of the given base type in the objects components list and returns the first match.
  [[nodiscard]] bool TryGetComponentOfBaseType(const WRTTI* pType, const WComponent*& out_pComponent) const;

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(WDynamicArray<T*>& out_components);

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(WDynamicArray<const T*>& out_components) const;

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const WRTTI* pType, WDynamicArray<WComponent*>& out_components);

  /// Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const WRTTI* pType, WDynamicArray<const WComponent*>& out_components) const;

  /// Returns a list of all components attached to this object.
  WArrayPtr<WComponent* const> GetComponents();

  /// Returns a list of all components attached to this object.
  WArrayPtr<const WComponent* const> GetComponents() const;

  /// Returns the current version of components attached to this object.
  /// This version is increased whenever components are added or removed and can be used for cache validation.
  WUInt16 GetComponentVersion() const;


  /// Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(WMessage& ref_msg);

  /// Sends a message to all components of this object.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessage(WMessage& ref_msg) const;

  /// Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(WMessage& ref_msg);

  /// Sends a message to all components of this object and then recursively to all children.
  ///
  /// Returns true, if there was any recipient for this type of message.
  bool SendMessageRecursive(WMessage& ref_msg) const;


  /// Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType = WObjectMsgQueueType::NextFrame) const;

  /// Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const WMessage& msg, WTime delay, WObjectMsgQueueType::Enum queueType = WObjectMsgQueueType::NextFrame) const;

  /// Delivers an WMessage to the closest (parent) object whose components handle the given message type.
  ///
  /// Regular SendMessage() and PostMessage() send a message directly to the target object (and all attached components).
  /// SendMessageRecursive() and PostMessageRecursive() send a message 'down' the graph to the target object and all children.
  ///
  /// In contrast, SendEventMessage() / PostEventMessage() bubble the message 'up' the graph.
  /// They do so by inspecting the chain of parent objects until they find one or multiple components that handle this type of message.
  /// If such components are found, the message is delivered to them directly, and no other component.
  /// If an WEventMessageHandlerComponent is found that does not handle this type of message, the message is discarded and NOT tried to be delivered
  /// to anyone else.
  ///
  /// If no such component is found in all parent objects, the message is delivered to one WEventMessageHandlerComponent
  /// instances that is set to 'handle global events' (typically used for level-logic scripts), no matter where in the graph it resides.
  /// If multiple global event handler component exist that handle the same message type, it is delivered to all of them in a non-deterministic order.
  ///
  /// \param msg The message to deliver.
  /// \param senderComponent The component that triggered the event in the first place. May be nullptr.
  ///        Currently it is only used to prevent the message from being delivered to the sender component itself.
  bool SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent);

  /// \copydoc WGameObject::SendEventMessage()
  bool SendEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent) const;

  /// \copydoc WGameObject::SendEventMessage()
  ///
  /// \param queueType In which update phase to deliver the message.
  /// \param delay An optional delay before delivering the message.
  void PostEventMessage(WMessage& ref_msg, const WComponent* pSenderComponent, WTime delay, WObjectMsgQueueType::Enum queueType = WObjectMsgQueueType::NextFrame) const;


  /// Returns the tag set associated with this object.
  const WTagSet& GetTags() const;

  /// Sets the tag set associated with this object.
  void SetTags(const WTagSet& tags);

  /// Adds the given tag to the object's tags.
  void SetTag(const WTag& tag);

  /// Removes the given tag from the object's tags.
  void RemoveTag(const WTag& tag);

  /// Checks whether this object has the given tag.
  bool HasTag(const WTempHashedString& sTagName) const;

  /// Returns the 'team ID' that was given during creation (/see WGameObjectDesc)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const WUInt16& GetTeamID() const { return m_uiTeamID; }

  /// Changes the team ID for this object and all children recursively.
  void SetTeamID(WUInt16 uiId);

  /// Returns a random value that is chosen once during object creation and remains stable even throughout serialization.
  ///
  /// This value is intended to be used for choosing random variations of components. For instance, if a component has two
  /// different meshes it can use for variation, this seed should be used to decide which one to use.
  ///
  /// The stable random seed can also be set from the outside, which is what the editor does, to assign a truly stable seed value.
  /// Therefore, each object placed in the editor will always have the same seed value, and objects won't change their appearance
  /// on every run of the game.
  ///
  /// The stable seed is also propagated through prefab instances, such that every prefab instance gets a different value, but
  /// in a deterministic fashion.
  WUInt32 GetStableRandomSeed() const;

  /// Overwrites the object's random seed value.
  ///
  /// See \a GetStableRandomSeed() for details.
  ///
  /// It should not be necessary to manually change this value, unless you want to make the seed deterministic according to a custom rule.
  void SetStableRandomSeed(WUInt32 uiSeed);

  /// Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  /// This can be used to adjust the update logic of objects.
  /// An invisible object may stop updating entirely. An indirectly visible object may reduce its update rate.
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  WVisibilityState::Enum GetVisibilityState(WUInt32 uiNumFramesBeforeInvisible = 5) const;

private:
  friend class WComponentManagerBase;
  friend class WGameObjectTest;

  // only needed until reflection can deal with WStringView
  void SetNameInternal(const char* szName);
  const char* GetNameInternal() const;
  void SetGlobalKeyInternal(const char* szKey);
  const char* GetGlobalKeyInternal() const;

  bool SendMessageInternal(WMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(WMessage& msg, bool bWasPostedMsg) const;
  bool SendMessageRecursiveInternal(WMessage& msg, bool bWasPostedMsg);
  bool SendMessageRecursiveInternal(WMessage& msg, bool bWasPostedMsg) const;

  W_ALLOW_PRIVATE_PROPERTIES(WGameObject);

  void Reflection_SetTag(const char* szTagName);
  void Reflection_RemoveTag(const char* szTagName);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void Reflection_AddChild(WGameObject* pChild);
  void Reflection_DetachChild(WGameObject* pChild);
  WHybridArray<WGameObject*, 8> Reflection_GetChildren() const;
  void Reflection_AddComponent(WComponent* pComponent);
  void Reflection_RemoveComponent(WComponent* pComponent);
  WHybridArray<WComponent*, NUM_INPLACE_COMPONENTS> Reflection_GetComponents() const;

  WGameObject* Reflection_FindChildByName(const WTempHashedString& sName, bool bRecursive) { return FindChildByName(sName, bRecursive); }
  WGameObject* Reflection_FindChildByPath(WStringView sPath) { return FindChildByPath(sPath); }

  WObjectMode::Enum Reflection_GetMode() const;
  void Reflection_SetMode(WObjectMode::Enum mode);

  WGameObject* Reflection_GetParent() const;
  void Reflection_SetGlobalPosition(const WVec3& vPosition);
  void Reflection_SetGlobalRotation(const WQuat& qRotation);
  void Reflection_SetGlobalScaling(const WVec3& vScaling);
  void Reflection_SetGlobalTransform(const WTransform& transform);

  bool DetermineDynamicMode(WComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(WComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();
  void UpdateLastGlobalTransform();

  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);

  void AddComponent(WComponent* pComponent);
  void RemoveComponent(WComponent* pComponent);
  void FixComponentPointer(WComponent* pOldPtr, WComponent* pNewPtr);

  // Updates the active state of this object and all children and attached components recursively, depending on the enabled states.
  void UpdateActiveState(bool bParentActive);

  void SendNotificationMessage(WMessage& msg);

  struct W_CORE_DLL alignas(16) TransformationData
  {
    W_DECLARE_POD_TYPE();

    WGameObject* m_pObject;
    TransformationData* m_pParentData;

#if W_ENABLED(W_PLATFORM_32BIT)
    WUInt64 m_uiPadding;
#endif

    WSimdVec4f m_localPosition;
    WSimdQuat m_localRotation;
    WSimdVec4f m_localScaling; // x,y,z = non-uniform scaling, w = uniform scaling

    WSimdTransform m_globalTransform;

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
    WSimdTransform m_lastGlobalTransform;
#endif

    WSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    WSimdBBoxSphere m_globalBounds;

    WSpatialDataHandle m_hSpatialData;
    WUInt32 m_uiSpatialDataCategoryBitmask;

    WUInt32 m_uiStableRandomSeed = 0;

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
    WUInt32 m_uiLastGlobalTransformUpdateCounter = 0;
#else
    WUInt32 m_uiPadding2[1];
#endif

    /// Recomputes the local transform from this object's global transform and, if available, the parent's global transform.
    void UpdateLocalTransform();

    /// Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// In case there is a parent transform it also recursively calls itself on the parent transform to ensure everything is up-to-date.
    void UpdateGlobalTransformRecursive(WUInt32 uiUpdateCounter);

    /// Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformNonRecursive(WUInt32 uiUpdateCounter);

    /// Updates the global transform by copying the object's local transform into the global transform.
    /// This is for objects that have no parent.
    void UpdateGlobalTransformWithoutParent(WUInt32 uiUpdateCounter);

    /// Updates the global transform by combining the parents global transform with this object's local transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformWithParent(WUInt32 uiUpdateCounter);

    void UpdateGlobalBounds(WSpatialSystem* pSpatialSystem);
    void UpdateGlobalBounds();
    void UpdateGlobalBoundsAndSpatialData(WSpatialSystem& ref_spatialSystem);

    void UpdateLastGlobalTransform(WUInt32 uiUpdateCounter);

    void RecreateSpatialData(WSpatialSystem& ref_spatialSystem);
  };

  WGameObjectId m_InternalId;
  WHashedString m_sName;

#if W_ENABLED(W_PLATFORM_32BIT)
  WUInt32 m_uiNamePadding;
#endif

  WBitflags<WObjectFlags> m_Flags;

  WUInt32 m_uiParentIndex = 0;
  WUInt32 m_uiFirstChildIndex = 0;
  WUInt32 m_uiLastChildIndex = 0;

  WUInt32 m_uiNextSiblingIndex = 0;
  WUInt32 m_uiPrevSiblingIndex = 0;
  WUInt32 m_uiChildCount = 0;

  WUInt16 m_uiHierarchyLevel = 0;

  /// An int that will be passed on to objects spawned from this one, which allows to identify which team or player it belongs to.
  WUInt16 m_uiTeamID = 0;

  TransformationData* m_pTransformationData = nullptr;

#if W_ENABLED(W_PLATFORM_32BIT)
  WUInt32 m_uiPadding = 0;
#endif

  WSmallArrayBase<WComponent*, NUM_INPLACE_COMPONENTS> m_Components;

  struct ComponentUserData
  {
    WUInt16 m_uiVersion;
    WUInt16 m_uiUnused;
  };

  WTagSet m_Tags;
};

W_DECLARE_REFLECTABLE_TYPE(W_CORE_DLL, WGameObject);

#include <Core/World/Implementation/GameObject_inl.h>
