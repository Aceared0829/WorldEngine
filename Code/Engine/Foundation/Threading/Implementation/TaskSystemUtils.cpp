#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

const char* WWorkerThreadType::GetThreadTypeName(WWorkerThreadType::Enum threadType)
{
  switch (threadType)
  {
    case WWorkerThreadType::ShortTasks:
      return "Short Task";

    case WWorkerThreadType::LongTasks:
      return "Long Task";

    case WWorkerThreadType::FileAccess:
      return "File Access";

    default:
      W_REPORT_FAILURE("Invalid Thread Type");
      return "unknown";
  }
}

void WTaskSystem::WriteStateSnapshotToDGML(WDGMLGraph& ref_graph)
{
  W_LOCK(s_TaskSystemMutex);

  WHashTable<const WTaskGroup*, WDGMLGraph::NodeId> groupNodeIds;

  WStringBuilder title, tmp;

  WDGMLGraph::NodeDesc taskGroupND;
  taskGroupND.m_Color = WColor::CornflowerBlue;
  taskGroupND.m_Shape = WDGMLGraph::NodeShape::Rectangle;

  WDGMLGraph::NodeDesc taskNodeND;
  taskNodeND.m_Color = WColor::OrangeRed;
  taskNodeND.m_Shape = WDGMLGraph::NodeShape::RoundedRectangle;

  const WDGMLGraph::PropertyId startedByUserId = ref_graph.AddPropertyType("StartByUser");
  const WDGMLGraph::PropertyId activeDepsId = ref_graph.AddPropertyType("ActiveDependencies");
  const WDGMLGraph::PropertyId scheduledId = ref_graph.AddPropertyType("Scheduled");
  const WDGMLGraph::PropertyId finishedId = ref_graph.AddPropertyType("Finished");
  const WDGMLGraph::PropertyId multiplicityId = ref_graph.AddPropertyType("Multiplicity");
  const WDGMLGraph::PropertyId remainingRunsId = ref_graph.AddPropertyType("RemainingRuns");
  const WDGMLGraph::PropertyId priorityId = ref_graph.AddPropertyType("GroupPriority");

  const char* szTaskPriorityNames[WTaskPriority::ENUM_COUNT] = {};
  szTaskPriorityNames[WTaskPriority::EarlyThisFrame] = "EarlyThisFrame";
  szTaskPriorityNames[WTaskPriority::ThisFrame] = "ThisFrame";
  szTaskPriorityNames[WTaskPriority::LateThisFrame] = "LateThisFrame";
  szTaskPriorityNames[WTaskPriority::EarlyNextFrame] = "EarlyNextFrame";
  szTaskPriorityNames[WTaskPriority::NextFrame] = "NextFrame";
  szTaskPriorityNames[WTaskPriority::LateNextFrame] = "LateNextFrame";
  szTaskPriorityNames[WTaskPriority::In2Frames] = "In 2 Frames";
  szTaskPriorityNames[WTaskPriority::In3Frames] = "In 3 Frames";
  szTaskPriorityNames[WTaskPriority::In4Frames] = "In 4 Frames";
  szTaskPriorityNames[WTaskPriority::In5Frames] = "In 5 Frames";
  szTaskPriorityNames[WTaskPriority::In6Frames] = "In 6 Frames";
  szTaskPriorityNames[WTaskPriority::In7Frames] = "In 7 Frames";
  szTaskPriorityNames[WTaskPriority::In8Frames] = "In 8 Frames";
  szTaskPriorityNames[WTaskPriority::In9Frames] = "In 9 Frames";
  szTaskPriorityNames[WTaskPriority::LongRunningHighPriority] = "LongRunningHighPriority";
  szTaskPriorityNames[WTaskPriority::LongRunning] = "LongRunning";
  szTaskPriorityNames[WTaskPriority::FileAccessHighPriority] = "FileAccessHighPriority";
  szTaskPriorityNames[WTaskPriority::FileAccess] = "FileAccess";
  szTaskPriorityNames[WTaskPriority::ThisFrameMainThread] = "ThisFrameMainThread";
  szTaskPriorityNames[WTaskPriority::SomeFrameMainThread] = "SomeFrameMainThread";

  for (WUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const WTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    title.SetFormat("Group {}", g);

    const WDGMLGraph::NodeId taskGroupId = ref_graph.AddGroup(title, WDGMLGraph::GroupType::Expanded, &taskGroupND);
    groupNodeIds[&tg] = taskGroupId;

    ref_graph.AddNodeProperty(taskGroupId, startedByUserId, tg.m_bStartedByUser ? "true" : "false");
    ref_graph.AddNodeProperty(taskGroupId, priorityId, szTaskPriorityNames[tg.m_Priority]);
    ref_graph.AddNodeProperty(taskGroupId, activeDepsId, WFmt("{}", tg.m_iNumActiveDependencies));

    for (WUInt32 t = 0; t < tg.m_Tasks.GetCount(); ++t)
    {
      const WTask& task = *tg.m_Tasks[t];
      const WDGMLGraph::NodeId taskNodeId = ref_graph.AddNode(task.m_sTaskName, &taskNodeND);

      ref_graph.AddNodeToGroup(taskNodeId, taskGroupId);

      ref_graph.AddNodeProperty(taskNodeId, scheduledId, task.m_bTaskIsScheduled ? "true" : "false");
      ref_graph.AddNodeProperty(taskNodeId, finishedId, task.IsTaskFinished() ? "true" : "false");

      tmp.SetFormat("{}", task.GetMultiplicity());
      ref_graph.AddNodeProperty(taskNodeId, multiplicityId, tmp);

      tmp.SetFormat("{}", task.m_iRemainingRuns);
      ref_graph.AddNodeProperty(taskNodeId, remainingRunsId, tmp);
    }
  }

  for (WUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const WTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    const WDGMLGraph::NodeId ownNodeId = groupNodeIds[&tg];

    for (const WTaskGroupID& dependsOn : tg.m_DependsOnGroups)
    {
      WDGMLGraph::NodeId otherNodeId;

      // filter out already fulfilled dependencies
      if (dependsOn.m_pTaskGroup->m_uiGroupCounter != dependsOn.m_uiGroupCounter)
        continue;

      // filter out already fulfilled dependencies
      if (!groupNodeIds.TryGetValue(dependsOn.m_pTaskGroup, otherNodeId))
        continue;

      W_ASSERT_DEBUG(otherNodeId != ownNodeId, "");

      ref_graph.AddConnection(otherNodeId, ownNodeId);
    }
  }
}

void WTaskSystem::WriteStateSnapshotToFile(const char* szPath /*= nullptr*/)
{
  WStringBuilder sPath = szPath;

  if (sPath.IsEmpty())
  {
    sPath = ":appdata/TaskGraphs/";

    const WDateTime dt = WDateTime::MakeFromTimestamp(WTimestamp::CurrentTimestamp());

    sPath.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), WArgU(dt.GetMonth(), 2, true), WArgU(dt.GetDay(), 2, true), WArgU(dt.GetHour(), 2, true), WArgU(dt.GetMinute(), 2, true), WArgU(dt.GetSecond(), 2, true), WArgU(dt.GetMicroseconds() / 1000, 3, true));

    sPath.ChangeFileExtension("dgml");
  }

  WDGMLGraph graph;
  WTaskSystem::WriteStateSnapshotToDGML(graph);

  WDGMLGraphWriter::WriteGraphToFile(sPath, graph).IgnoreResult();

  WStringBuilder absPath;
  WFileSystem::ResolvePath(sPath, &absPath, nullptr).IgnoreResult();
  WLog::Info("Task graph snapshot saved to '{}'", absPath);
}
