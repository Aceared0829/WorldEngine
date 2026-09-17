#include <Utilities/UtilitiesPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Utilities/DGML/DGMLCreator.h>

void WDGMLGraphCreator::FillGraphFromWorld(WWorld* pWorld, WDGMLGraph& ref_graph)
{
  if (!pWorld)
  {
    WLog::Warning("WDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(WDGMLGraph& ref_graph)
      : m_Graph(ref_graph)
    {
      WDGMLGraph::NodeDesc nd;
      nd.m_Color = WColor::DarkRed;
      nd.m_Shape = WDGMLGraph::NodeShape::Button;
      m_WorldNodeId = ref_graph.AddNode("World", &nd);
    }

    WVisitorExecution::Enum Visit(WGameObject* pObject)
    {
      WStringBuilder name;
      name.SetFormat("GameObject: \"{0}\"", pObject->GetName().IsEmpty() ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      WDGMLGraph::NodeDesc gameobjectND;
      gameobjectND.m_Color = WColor::CornflowerBlue;
      gameobjectND.m_Shape = WDGMLGraph::NodeShape::Rectangle;
      auto gameObjectNodeId = m_Graph.AddNode(name.GetData(), &gameobjectND);

      m_VisitedObjects.Insert(pObject, gameObjectNodeId);

      // Add connection to parent if existent
      if (const WGameObject* parent = pObject->GetParent())
      {
        auto it = m_VisitedObjects.Find(parent);

        if (it.IsValid())
        {
          m_Graph.AddConnection(gameObjectNodeId, it.Value());
        }
      }
      else
      {
        // No parent -> connect to world
        m_Graph.AddConnection(gameObjectNodeId, m_WorldNodeId);
      }

      // Add components
      for (auto component : pObject->GetComponents())
      {
        auto sComponentName = component->GetDynamicRTTI()->GetTypeName();

        WDGMLGraph::NodeDesc componentND;
        componentND.m_Color = WColor::LimeGreen;
        componentND.m_Shape = WDGMLGraph::NodeShape::RoundedRectangle;
        auto componentNodeId = m_Graph.AddNode(sComponentName, &componentND);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return WVisitorExecution::Continue;
    }

    WDGMLGraph& m_Graph;

    WDGMLGraph::NodeId m_WorldNodeId;
    WMap<const WGameObject*, WDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(ref_graph);
  pWorld->Traverse(WWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), WWorld::BreadthFirst);
}
