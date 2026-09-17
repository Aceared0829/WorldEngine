#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Messages/EventMessageSender.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/Blackboard.h>

struct WMsgUpdateLocalBounds;
struct WMsgExtractRenderData;

using WBlackboardTemplateResourceHandle = WTypedResourceHandle<class WBlackboardTemplateResource>;

struct WBlackboardEntry
{
  WHashedString m_sName;
  WVariant m_InitialValue;
  WBitflags<WBlackboardEntryFlags> m_Flags;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WBlackboardEntry);

//////////////////////////////////////////////////////////////////////////

struct W_RENDERERCORE_DLL WMsgBlackboardEntryChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgBlackboardEntryChanged, WMessage);

  WHashedString m_sName;
  WVariant m_OldValue;
  WVariant m_NewValue;

private:
  const char* GetName() const { return m_sName; }
  void SetName(const char* szName) { m_sName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// This base component represents an WBlackboard, which can be used to share state between multiple components and objects.
///
/// The derived implementations may either create their own blackboards or reference other blackboards.
class W_RENDERERCORE_DLL WBlackboardComponent : public WComponent
{
  W_DECLARE_ABSTRACT_COMPONENT_TYPE(WBlackboardComponent, WComponent);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WBlackboardComponent

public:
  WBlackboardComponent();
  ~WBlackboardComponent();

  /// Try to find a WBlackboardComponent on pSearchObject or its parents with the given name and returns its blackboard.
  ///
  /// The blackboard name is only checked if the given name is not empty.
  /// If no matching blackboard component is found and NO name is given the world's blackboard is returned.
  /// If no matching blackboard component is found and a name is given, the function will call WBlackboard::GetOrCreateGlobal() with the given name.
  /// Thus you will always get a result, either from a component, the world or from the global storage.
  ///
  /// \sa WBlackboard::GetOrCreateGlobal()
  static WSharedPtr<WBlackboard> FindBlackboard(WGameObject& ref_searchObject, WStringView sBlackboardName = WStringView());

  /// Returns the blackboard owned by this component
  const WSharedPtr<WBlackboard>& GetBoard();
  WSharedPtr<const WBlackboard> GetBoard() const;

  void SetShowDebugInfo(bool bShow);                              // [ property ]
  bool GetShowDebugInfo() const;                                  // [ property ]

  void SetEntryValue(const char* szName, const WVariant& value); // [ scriptable ]
  WVariant GetEntryValue(const char* szName) const;              // [ scriptable ]

protected:
  static WBlackboard* Reflection_FindBlackboard(WGameObject* pSearchObject, WStringView sBlackboardName);

  void OnUpdateLocalBounds(WMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(WMsgExtractRenderData& msg) const;

  WSharedPtr<WBlackboard> m_pBoard;

  WBlackboardTemplateResourceHandle m_hTemplate;
};

//////////////////////////////////////////////////////////////////////////

using WLocalBlackboardComponentManager = WComponentManager<class WLocalBlackboardComponent, WBlockStorageType::Compact>;

/// This component creates its own WBlackboard, and thus locally holds state.
class W_RENDERERCORE_DLL WLocalBlackboardComponent : public WBlackboardComponent
{
  W_DECLARE_COMPONENT_TYPE(WLocalBlackboardComponent, WBlackboardComponent, WLocalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WBlackboardComponent

public:
  WLocalBlackboardComponent();
  WLocalBlackboardComponent(WLocalBlackboardComponent&& other);
  ~WLocalBlackboardComponent();

  WLocalBlackboardComponent& operator=(WLocalBlackboardComponent&& other);

  void SetSendEntryChangedMessage(bool bSend); // [ property ]
  bool GetSendEntryChangedMessage() const;     // [ property ]

  void SetBlackboardName(const char* szName);  // [ property ]
  const char* GetBlackboardName() const;       // [ property ]

private:
  WUInt32 Entries_GetCount() const;
  WBlackboardEntry Entries_GetValue(WUInt32 uiIndex) const;
  void Entries_SetValue(WUInt32 uiIndex, WBlackboardEntry entry);
  void Entries_Insert(WUInt32 uiIndex, WBlackboardEntry entry);
  void Entries_Remove(WUInt32 uiIndex);

  void OnEntryChanged(const WBlackboard::EntryEvent& e);
  void InitializeFromTemplate();
  bool IsEditor() const;

  // this array is not held during runtime, it is only needed during editor time until the component is serialized out
  WDynamicArray<WBlackboardEntry> m_InitialEntries;

  WEventMessageSender<WMsgBlackboardEntryChanged> m_EntryChangedSender; // [ event ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct WGlobalBlackboardInitMode
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    EnsureEntriesExist,    ///< Brief only adds entries to the blackboard, that haven't been added before. Doesn't change the values of existing entries.
    ResetEntryValues,      ///< Overwrites values of existing entries, to reset them to the start value defined in the template.
    ClearEntireBlackboard, ///< Removes all entries from the blackboard and only adds the ones from the template. This also gets rid of temporary values.

    Default = ClearEntireBlackboard
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WGlobalBlackboardInitMode);

using WGlobalBlackboardComponentManager = WComponentManager<class WGlobalBlackboardComponent, WBlockStorageType::Compact>;

/// This component references a global blackboard by name. If necessary, the blackboard will be created.
///
/// This allows to initialize a global blackboard with known values.
class W_RENDERERCORE_DLL WGlobalBlackboardComponent : public WBlackboardComponent
{
  W_DECLARE_COMPONENT_TYPE(WGlobalBlackboardComponent, WBlackboardComponent, WGlobalBlackboardComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WGlobalBlackboardComponent

public:
  WGlobalBlackboardComponent();
  WGlobalBlackboardComponent(WGlobalBlackboardComponent&& other);
  ~WGlobalBlackboardComponent();

  WGlobalBlackboardComponent& operator=(WGlobalBlackboardComponent&& other);

  void SetBlackboardName(const char* szName);    // [ property ]
  const char* GetBlackboardName() const;         // [ property ]

  WEnum<WGlobalBlackboardInitMode> m_InitMode; // [ property ]

private:
  void InitializeFromTemplate();

  WHashedString m_sName;
};
