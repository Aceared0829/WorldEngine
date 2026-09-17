#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <QAbstractItemModel>
#include <QIcon>

class WPropertyAnimAssetDocument;
struct WDocumentObjectPropertyEvent;
struct WDocumentObjectStructureEvent;
class WPropertyAnimationTrack;

struct WQtPropertyAnimModelTreeEntry
{
  WString m_sPathToItem;
  WInt32 m_iParent = -1;
  WUInt16 m_uiOwnRowIndex = 0;
  WPropertyAnimationTrack* m_pTrack = nullptr;
  WInt32 m_iTrackIdx = -1;
  WString m_sDisplay;
  WDynamicArray<WInt32> m_Children;
  QIcon m_Icon;

  bool operator==(const WQtPropertyAnimModelTreeEntry& rhs) const
  {
    return (m_iParent == rhs.m_iParent) && (m_uiOwnRowIndex == rhs.m_uiOwnRowIndex) && (m_pTrack == rhs.m_pTrack) &&
           (m_iTrackIdx == rhs.m_iTrackIdx) && (m_sDisplay == rhs.m_sDisplay) && (m_Children == rhs.m_Children);
  }

  bool operator!=(const WQtPropertyAnimModelTreeEntry& rhs) const { return !(*this == rhs); }
};

class WQtPropertyAnimModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  WQtPropertyAnimModel(WPropertyAnimAssetDocument* pDocument, QObject* pParent);
  ~WQtPropertyAnimModel();

  enum UserRoles
  {
    TrackPtr = Qt::UserRole + 1,
    TreeItem = Qt::UserRole + 2,
    TrackIdx = Qt::UserRole + 3,
    Path = Qt::UserRole + 4,
  };

  const WDeque<WQtPropertyAnimModelTreeEntry>& GetAllEntries() const { return m_AllEntries[m_iInUse]; }

private Q_SLOTS:
  void onBuildMappingTriggered();

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void DocumentStructureEventHandler(const WDocumentObjectStructureEvent& e);
  void DocumentPropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void TriggerBuildMapping();
  void BuildMapping();
  void BuildMapping(WInt32 iToUse);
  void BuildMapping(WInt32 iToUse, WInt32 iTrackIdx, WPropertyAnimationTrack* pTrack, WDynamicArray<WInt32>& treeItems, WInt32 iParentEntry,
    const char* szPath);

  bool m_bBuildMappingQueued = false;
  WInt32 m_iInUse = 0;
  WDynamicArray<WInt32> m_TopLevelEntries[2];
  WDeque<WQtPropertyAnimModelTreeEntry> m_AllEntries[2];

  WPropertyAnimAssetDocument* m_pAssetDoc = nullptr;
};
