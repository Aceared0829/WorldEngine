#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Utilities/DGMLWriter.h>

WDGMLGraph::WDGMLGraph(WDGMLGraph::Direction graphDirection /*= LeftToRight*/, WDGMLGraph::Layout graphLayout /*= Tree*/)
  : m_Direction(graphDirection)
  , m_Layout(graphLayout)
{
}

WDGMLGraph::NodeId WDGMLGraph::AddNode(WStringView sTitle, const NodeDesc* pDesc)
{
  return AddGroup(sTitle, GroupType::None, pDesc);
}

WDGMLGraph::NodeId WDGMLGraph::AddGroup(WStringView sTitle, GroupType type, const NodeDesc* pDesc /*= nullptr*/)
{
  WDGMLGraph::Node& Node = m_Nodes.ExpandAndGetRef();

  Node.m_Title = sTitle;
  Node.m_GroupType = type;

  if (pDesc)
  {
    Node.m_Desc = *pDesc;
  }

  return m_Nodes.GetCount() - 1;
}

void WDGMLGraph::AddNodeToGroup(NodeId node, NodeId group)
{
  W_ASSERT_DEBUG(m_Nodes[group].m_GroupType != GroupType::None, "The given group node has not been created as a group node");

  m_Nodes[node].m_ParentGroup = group;
}

WDGMLGraph::CategoryId WDGMLGraph::AddConnectionCategory(WStringView sName, const WColor& color)
{
  auto& cat = m_ConnectionCategories.ExpandAndGetRef();
  cat.m_sName = sName;
  cat.m_Color = color;
  return m_ConnectionCategories.GetCount() - 1;
}

WDGMLGraph::ConnectionId WDGMLGraph::AddConnection(WDGMLGraph::NodeId source, WDGMLGraph::NodeId target, WStringView sLabel, CategoryId category)
{
  WDGMLGraph::Connection& connection = m_Connections.ExpandAndGetRef();

  connection.m_Source = source;
  connection.m_Target = target;
  connection.m_sLabel = sLabel;
  connection.m_uiCategory = category;

  return m_Connections.GetCount() - 1;
}

WDGMLGraph::PropertyId WDGMLGraph::AddPropertyType(WStringView sName)
{
  auto& prop = m_PropertyTypes.ExpandAndGetRef();
  prop.m_Name = sName;
  return m_PropertyTypes.GetCount() - 1;
}

void WDGMLGraph::AddNodeProperty(NodeId node, PropertyId property, const WFormatString& fmt)
{
  WStringBuilder tmp;

  auto& prop = m_Nodes[node].m_Properties.ExpandAndGetRef();
  prop.m_PropertyId = property;
  prop.m_sValue = fmt.GetText(tmp);
}

WResult WDGMLGraphWriter::WriteGraphToFile(WStringView sFileName, const WDGMLGraph& graph)
{
  WStringBuilder sGraph;

  // Write to memory object and then to file
  if (WriteGraphToString(sGraph, graph).Succeeded())
  {
    WStringBuilder sTemp;

    WFileWriter fileWriter;
    if (!fileWriter.Open(sFileName.GetData(sTemp)).Succeeded())
      return W_FAILURE;

    fileWriter.WriteBytes(sGraph.GetData(), sGraph.GetElementCount()).IgnoreResult();

    fileWriter.Close();

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WDGMLGraphWriter::WriteGraphToString(WStringBuilder& ref_sStringBuilder, const WDGMLGraph& graph)
{
  const char* szDirection = nullptr;
  const char* szLayout = nullptr;

  switch (graph.m_Direction)
  {
    case WDGMLGraph::Direction::TopToBottom:
      szDirection = "TopToBottom";
      break;
    case WDGMLGraph::Direction::BottomToTop:
      szDirection = "BottomToTop";
      break;
    case WDGMLGraph::Direction::LeftToRight:
      szDirection = "LeftToRight";
      break;
    case WDGMLGraph::Direction::RightToLeft:
      szDirection = "RightToLeft";
      break;
  }

  switch (graph.m_Layout)
  {
    case WDGMLGraph::Layout::Free:
      szLayout = "None";
      break;
    case WDGMLGraph::Layout::Tree:
      szLayout = "Sugiyama";
      break;
    case WDGMLGraph::Layout::DependencyMatrix:
      szLayout = "DependencyMatrix";
      break;
  }

  ref_sStringBuilder.AppendFormat("<DirectedGraph xmlns=\"http://schemas.microsoft.com/vs/2009/dgml\" GraphDirection=\"{0}\" Layout=\"{1}\">\n", szDirection, szLayout);

  // Write out all connection categories
  if (!graph.m_ConnectionCategories.IsEmpty())
  {
    ref_sStringBuilder.Append("\t<Categories>\n");
    for (WUInt32 i = 0; i < graph.m_ConnectionCategories.GetCount(); ++i)
    {
      const auto& cat = graph.m_ConnectionCategories[i];
      WColorGammaUB rgba(cat.m_Color);
      WStringBuilder sStroke;
      sStroke.SetFormat("#FF{0}{1}{2}", WArgU(rgba.r, 2, true, 16, true), WArgU(rgba.g, 2, true, 16, true), WArgU(rgba.b, 2, true, 16, true));
      ref_sStringBuilder.AppendFormat("\t\t<Category Id=\"C_{0}\" Label=\"{1}\" Stroke=\"{2}\" />\n", i, cat.m_sName, sStroke);
    }
    ref_sStringBuilder.Append("\t</Categories>\n");
  }

  // Write out all the properties
  if (!graph.m_PropertyTypes.IsEmpty())
  {
    ref_sStringBuilder.Append("\t<Properties>\n");

    for (WUInt32 i = 0; i < graph.m_PropertyTypes.GetCount(); ++i)
    {
      const auto& prop = graph.m_PropertyTypes[i];

      ref_sStringBuilder.AppendFormat("\t\t<Property Id=\"P_{0}\" Label=\"{1}\" DataType=\"String\"/>\n", i, prop.m_Name);
    }

    ref_sStringBuilder.Append("\t</Properties>\n");
  }

  // Write out all the nodes
  if (!graph.m_Nodes.IsEmpty())
  {
    WStringBuilder ColorValue;
    WStringBuilder PropertiesString;
    WStringBuilder SanitizedName;
    const char* szGroupString;

    ref_sStringBuilder.Append("\t<Nodes>\n");
    for (WUInt32 i = 0; i < graph.m_Nodes.GetCount(); ++i)
    {
      const WDGMLGraph::Node& node = graph.m_Nodes[i];

      SanitizedName = node.m_Title;
      SanitizedName.ReplaceAll("&", "&#038;");
      SanitizedName.ReplaceAll("<", "&lt;");
      SanitizedName.ReplaceAll(">", "&gt;");
      SanitizedName.ReplaceAll("\"", "&quot;");
      SanitizedName.ReplaceAll("'", "&apos;");
      SanitizedName.ReplaceAll("\n", "&#xA;");

      ColorValue = "#FF";
      WColorGammaUB RGBA(node.m_Desc.m_Color);
      ColorValue.AppendFormat("{0}{1}{2}", WArgU(RGBA.r, 2, true, 16, true), WArgU(RGBA.g, 2, true, 16, true), WArgU(RGBA.b, 2, true, 16, true));

      WStringBuilder StyleString;
      switch (node.m_Desc.m_Shape)
      {
        case WDGMLGraph::NodeShape::None:
          StyleString = "Shape=\"None\"";
          break;
        case WDGMLGraph::NodeShape::Rectangle:
          StyleString = "NodeRadius=\"0\"";
          break;
        case WDGMLGraph::NodeShape::RoundedRectangle:
          StyleString = "NodeRadius=\"4\"";
          break;
        case WDGMLGraph::NodeShape::Button:
          StyleString = "";
          break;
      }

      switch (node.m_GroupType)
      {

        case WDGMLGraph::GroupType::Expanded:
          szGroupString = " Group=\"Expanded\"";
          break;

        case WDGMLGraph::GroupType::Collapsed:
          szGroupString = " Group=\"Collapsed\"";
          break;

        case WDGMLGraph::GroupType::None:
        default:
          szGroupString = nullptr;
          break;
      }

      PropertiesString.Clear();
      for (const auto& prop : node.m_Properties)
      {
        PropertiesString.AppendFormat(" {0}=\"{1}\"", graph.m_PropertyTypes[prop.m_PropertyId].m_Name, prop.m_sValue);
      }

      ref_sStringBuilder.AppendFormat("\t\t<Node Id=\"N_{0}\" Label=\"{1}\" Background=\"{2}\" {3}{4}{5}/>\n", i, SanitizedName, ColorValue, StyleString, szGroupString, PropertiesString);
    }
    ref_sStringBuilder.Append("\t</Nodes>\n");
  }

  // Write out the links
  if (!graph.m_Connections.IsEmpty())
  {
    ref_sStringBuilder.Append("\t<Links>\n");
    {
      for (WUInt32 i = 0; i < graph.m_Connections.GetCount(); ++i)
      {
        const auto& conn = graph.m_Connections[i];
        if (conn.m_uiCategory != WInvalidIndex)
        {
          ref_sStringBuilder.AppendFormat("\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" Label=\"{2}\" Category=\"C_{3}\" />\n",
            conn.m_Source, conn.m_Target, conn.m_sLabel, conn.m_uiCategory);
        }
        else
        {
          ref_sStringBuilder.AppendFormat("\t\t<Link Source=\"N_{0}\" Target=\"N_{1}\" Label=\"{2}\" />\n",
            conn.m_Source, conn.m_Target, conn.m_sLabel);
        }
      }

      for (WUInt32 i = 0; i < graph.m_Nodes.GetCount(); ++i)
      {
        const WDGMLGraph::Node& node = graph.m_Nodes[i];

        if (node.m_ParentGroup != 0xFFFFFFFF)
        {
          ref_sStringBuilder.AppendFormat("\t\t<Link Category=\"Contains\" Source=\"N_{0}\" Target=\"N_{1}\" />\n", node.m_ParentGroup, i);
        }
      }
    }
    ref_sStringBuilder.Append("\t</Links>\n");
  }

  ref_sStringBuilder.Append("</DirectedGraph>\n");

  return W_SUCCESS;
}
