#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class WQtVisualGraphNode;
class WQtVisualGraphPin;
class WQtVisualGraphConnection;
struct WSelectionManagerEvent;

/// Qt graphics scene for displaying and interacting with visual graphs.
///
/// This class manages the visual representation of node graphs, including rendering nodes, pins, and connections.
/// It handles user interactions such as node creation, connection dragging, and selection.
/// Works in conjunction with WVisualGraphObjectManager which manages the document-side graph data.
class W_GUIFOUNDATION_DLL WQtVisualGraphScene : public QGraphicsScene
{
  Q_OBJECT
public:
  enum Type
  {
    Node = QGraphicsItem::UserType + 1,
    Pin,
    Connection
  };

  explicit WQtVisualGraphScene(QObject* pParent = nullptr);
  ~WQtVisualGraphScene();

  virtual void InitScene(const WVisualGraphObjectManager* pManager);
  const WVisualGraphObjectManager* GetDocumentNodeManager() const;
  const WDocument* GetDocument() const;

  static WRttiMappedObjectFactory<WQtVisualGraphNode>& GetNodeFactory();
  static WRttiMappedObjectFactory<WQtVisualGraphPin>& GetPinFactory();
  static WRttiMappedObjectFactory<WQtVisualGraphConnection>& GetConnectionFactory();
  static WVec2 GetLastMouseInteractionPos() { return s_vLastMouseInteraction; }

  /// Visual style for rendering connections between pins
  struct ConnectionStyle
  {
    using StorageType = WUInt32;

    enum Enum
    {
      BezierCurve,
      StraightLine,
      SubwayLines,

      Default = BezierCurve
    };
  };

  void SetConnectionStyle(WEnum<ConnectionStyle> style);
  WEnum<ConnectionStyle> GetConnectionStyle() const { return m_ConnectionStyle; }

  struct ConnectionDecorationFlags
  {
    using StorageType = WUInt32;

    enum Enum
    {
      DirectionArrows = W_BIT(0), ///< Draw an arrow to indicate the connection's direction. Only works with straight lines atm.
      DrawDebugging = W_BIT(1),   ///< Draw animated effect to denote debugging.

      Default = 0
    };

    struct Bits
    {
      StorageType DirectionArrows : 1;
      StorageType DrawDebugging : 1;
    };
  };

  void SetConnectionDecorationFlags(WBitflags<ConnectionDecorationFlags> flags);
  WBitflags<ConnectionDecorationFlags> GetConnectionDecorationFlags() const { return m_ConnectionDecorationFlags; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

private:
  void Clear();
  void CreateQtNode(const WDocumentObject* pObject);
  void DeleteQtNode(const WDocumentObject* pObject);
  void CreateQtConnection(const WDocumentObject* pObject);
  void DeleteQtConnection(const WDocumentObject* pObject);
  void RecreateQtPins(const WDocumentObject* pObject);
  void CreateNodeObject(const WVisualGraphNodeDesc& nodeTemplate);
  void NodeEventsHandler(const WVisualGraphObjectManagerEvent& e);
  void PropertyEventsHandler(const WDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const WSelectionManagerEvent& e);
  void GetSelectedNodes(WDeque<WQtVisualGraphNode*>& selection) const;
  void MarkupConnectablePins(WQtVisualGraphPin* pSourcePin);
  void ResetConnectablePinMarkup();
  void OpenSearchMenu(QPoint screenPos);

protected:
  /// Which 'recently used' list the node creation menu records to. See WQtSearchableMenuRecentList.
  /// If empty, the document type name is used when the menu is first opened.
  WString m_sRecentListName;

  virtual WStatus RemoveNode(WQtVisualGraphNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void AddCommentAroundSelectionAction();
  virtual void ConnectPinsAction(const WVisualGraphPin& sourcePin, const WVisualGraphPin& targetPin);
  virtual void DisconnectPinsAction(WQtVisualGraphConnection* pConnection);
  virtual void DisconnectPinsAction(WQtVisualGraphPin* pPin);

private Q_SLOTS:
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);
  void OnSelectionChanged();

private:
  static WRttiMappedObjectFactory<WQtVisualGraphNode> s_NodeFactory;
  static WRttiMappedObjectFactory<WQtVisualGraphPin> s_PinFactory;
  static WRttiMappedObjectFactory<WQtVisualGraphConnection> s_ConnectionFactory;

protected:
  const WVisualGraphObjectManager* m_pManager = nullptr;

  WMap<const WDocumentObject*, WQtVisualGraphNode*> m_Nodes;
  WMap<const WDocumentObject*, WQtVisualGraphConnection*> m_Connections;

private:
  bool m_bIgnoreSelectionChange = false;
  WQtVisualGraphPin* m_pStartPin = nullptr;
  WQtVisualGraphConnection* m_pTempConnection = nullptr;
  WQtVisualGraphNode* m_pTempNode = nullptr;
  WDeque<const WDocumentObject*> m_Selection;
  WVec2 m_vMousePos = WVec2::MakeZero();
  QString m_sContextMenuSearchText;
  WDynamicArray<const WQtVisualGraphPin*> m_ConnectablePins;
  WEnum<ConnectionStyle> m_ConnectionStyle;
  WBitflags<ConnectionDecorationFlags> m_ConnectionDecorationFlags;

  WDynamicArray<WVisualGraphNodeDesc> m_NodeCreationTemplates;
  WDynamicArray<WString> m_NodeCreationTemplatePaths;

  static WVec2 s_vLastMouseInteraction;
};

W_DECLARE_FLAGS_OPERATORS(WQtVisualGraphScene::ConnectionDecorationFlags);
