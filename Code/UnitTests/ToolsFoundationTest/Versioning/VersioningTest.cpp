#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

W_CREATE_SIMPLE_TEST_GROUP(Versioning);

struct WPatchTestBase
{
public:
  WPatchTestBase()
  {
    m_string = "Base";
    m_string2 = "";
  }

  WString m_string;
  WString m_string2;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WPatchTestBase);

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WPatchTestBase, WNoBase, 1, WRTTIDefaultAllocator<WPatchTestBase>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("String", m_string),
    W_MEMBER_PROPERTY("String2", m_string2),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct WPatchTest : public WPatchTestBase
{
public:
  WPatchTest() { m_iInt32 = 1; }

  WInt32 m_iInt32;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WPatchTest);

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WPatchTest, WPatchTestBase, 1, WRTTIDefaultAllocator<WPatchTest>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Int", m_iInt32),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class WPatchTestP : public WGraphPatch
  {
  public:
    WPatchTestP()
      : WGraphPatch("WPatchTestP", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      pNode->RenameProperty("Int", "IntRenamed");
      pNode->ChangeProperty("IntRenamed", 2);
    }
  };
  WPatchTestP g_WPatchTestP;

  /// Patch base class
  class WPatchTestBaseBP : public WGraphPatch
  {
  public:
    WPatchTestBaseBP()
      : WGraphPatch("WPatchTestBaseBP", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String", "BaseClassPatched");
    }
  };
  WPatchTestBaseBP g_WPatchTestBaseBP;

  /// Rename class
  class WPatchTestRN : public WGraphPatch
  {
  public:
    WPatchTestRN()
      : WGraphPatch("WPatchTestRN", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      ref_context.RenameClass("WPatchTestRN2");
      pNode->ChangeProperty("String", "RenameExecuted");
    }
  };
  WPatchTestRN g_WPatchTestRN;

  /// Patch renamed class to v3
  class WPatchTestRN2 : public WGraphPatch
  {
  public:
    WPatchTestRN2()
      : WGraphPatch("WPatchTestRN2", 3)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String2", "Patched");
    }
  };
  WPatchTestRN2 g_WPatchTestRN2;

  /// Change base class
  class WPatchTestCB : public WGraphPatch
  {
  public:
    WPatchTestCB()
      : WGraphPatch("WPatchTestCB", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      WVersionKey bases[] = {{"WPatchTestBaseBP", 1}};
      ref_context.ChangeBaseClass(bases);
      pNode->ChangeProperty("String2", "ChangedBase");
    }
  };
  WPatchTestCB g_WPatchTestCB;

  void ReplaceTypeName(WAbstractObjectGraph& ref_graph, WAbstractObjectGraph& ref_typesGraph, const char* szOldName, const char* szNewName)
  {
    for (auto it : ref_graph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if (szOldName == pNode->GetType())
        pNode->SetType(szNewName);
    }

    for (auto it : ref_typesGraph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if ("WReflectedTypeDescriptor" == pNode->GetType())
      {
        if (auto* pProp = pNode->FindProperty("TypeName"))
        {
          if (WStringUtils::IsEqual(szOldName, pProp->m_Value.Get<WString>()))
            pProp->m_Value = szNewName;
        }
        if (auto* pProp = pNode->FindProperty("ParentTypeName"))
        {
          if (WStringUtils::IsEqual(szOldName, pProp->m_Value.Get<WString>()))
            pProp->m_Value = szNewName;
        }
      }
    }
  }

  WAbstractObjectNode* SerializeObject(WAbstractObjectGraph& ref_graph, WAbstractObjectGraph& ref_typesGraph, const WRTTI* pRtti, void* pObject)
  {
    WAbstractObjectNode* pNode = nullptr;
    {
      // Object
      WRttiConverterContext context;
      WRttiConverterWriter rttiConverter(&ref_graph, &context, true, true);
      context.RegisterObject(WUuid::MakeStableUuidFromString(pRtti->GetTypeName()), pRtti, pObject);
      pNode = rttiConverter.AddObjectToGraph(pRtti, pObject, "ROOT");
    }
    {
      // Types
      WSet<const WRTTI*> types;
      types.Insert(pRtti);
      WReflectionUtils::GatherDependentTypes(pRtti, types);
      WToolsSerializationUtils::SerializeTypes(types, ref_typesGraph);
    }
    return pNode;
  }

  void PatchGraph(WAbstractObjectGraph& ref_graph, WAbstractObjectGraph& ref_typesGraph)
  {
    WGraphVersioning::GetSingleton()->PatchGraph(&ref_typesGraph);
    WGraphVersioning::GetSingleton()->PatchGraph(&ref_graph, &ref_typesGraph);
  }
} // namespace

W_CREATE_SIMPLE_TEST(Versioning, GraphPatch)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "PatchClass")
  {
    WAbstractObjectGraph graph;
    WAbstractObjectGraph typesGraph;

    WPatchTest data;
    data.m_iInt32 = 5;
    WAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, WGetStaticRTTI<WPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "WPatchTest", "WPatchTestP");
    PatchGraph(graph, typesGraph);

    WAbstractObjectNode::Property* pInt = pNode->FindProperty("IntRenamed");
    W_TEST_INT(2, pInt->m_Value.Get<WInt32>());
    W_TEST_BOOL(pNode->FindProperty("Int") == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PatchBaseClass")
  {
    WAbstractObjectGraph graph;
    WAbstractObjectGraph typesGraph;

    WPatchTest data;
    data.m_string = "Unpatched";
    WAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, WGetStaticRTTI<WPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "WPatchTestBase", "WPatchTestBaseBP");
    PatchGraph(graph, typesGraph);

    WAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    W_TEST_STRING(pString->m_Value.Get<WString>(), "BaseClassPatched");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "RenameClass")
  {
    WAbstractObjectGraph graph;
    WAbstractObjectGraph typesGraph;

    WPatchTest data;
    data.m_string = "NotRenamed";
    WAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, WGetStaticRTTI<WPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "WPatchTest", "WPatchTestRN");
    PatchGraph(graph, typesGraph);

    WAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    W_TEST_BOOL(pString->m_Value.Get<WString>() == "RenameExecuted");
    W_TEST_STRING(pNode->GetType(), "WPatchTestRN2");
    W_TEST_INT(pNode->GetTypeVersion(), 3);
    WAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    W_TEST_BOOL(pString2->m_Value.Get<WString>() == "Patched");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ChangeBaseClass")
  {
    WAbstractObjectGraph graph;
    WAbstractObjectGraph typesGraph;

    WPatchTest data;
    data.m_string = "NotPatched";
    WAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, WGetStaticRTTI<WPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "WPatchTest", "WPatchTestCB");
    PatchGraph(graph, typesGraph);

    WAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    W_TEST_STRING(pString->m_Value.Get<WString>(), "BaseClassPatched");
    W_TEST_INT(pNode->GetTypeVersion(), 2);
    WAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    W_TEST_STRING(pString2->m_Value.Get<WString>(), "ChangedBase");
  }
}
