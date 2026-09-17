#include <GameEngineTest/GameEngineTestPCH.h>

#include "ComponentSerializationTest.h"

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Reflection.h>

namespace ComponentSerializationTestDetail
{
  /// Message of the mismatch that WWorldReader reported during the current round trip, empty if it
  /// did not report one.
  static WStringBuilder g_sReportedFailure;
  static WAssertHandler g_PreviousAssertHandler = nullptr;

  /// Catches the failure that WWorldReader::InstantiationContext::DeserializeComponents() reports
  /// when a component type read a different number of bytes than were stored for it.
  ///
  /// The report goes through the assert handler, which the test framework normally turns into a
  /// failed test and a debug break. Routing it here instead keeps the test running so that all
  /// remaining component types are still checked, and turns the message into a regular test failure.
  static bool SerializationAssertHandler(const char* szSourceFile, WUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
  {
    g_sReportedFailure = szAssertMsg;

    // Everything that is not the size mismatch is a real problem and has to reach the normal handler.
    if (g_sReportedFailure.FindSubString("deserialized") == nullptr)
    {
      if (g_PreviousAssertHandler != nullptr)
        return g_PreviousAssertHandler(szSourceFile, uiLine, szFunction, szExpression, szAssertMsg);

      return true;
    }

    // Do not break into the debugger, the caller turns this into a test failure.
    return false;
  }

  enum class Result
  {
    Ok,
    Skipped,     ///< No component of this type could be created, nothing was tested.
    SizeMismatch ///< Serialize and Deserialize disagree about the amount of data.
  };

  /// Serializes a single component of the given type into a world of its own and reads it back.
  ///
  /// WWorldReader compares, per component type, how many bytes were stored against how many
  /// DeserializeComponent() consumed, and reports a failure through W_REPORT_FAILURE if they differ.
  /// That macro is active in every build configuration and goes through the assert handler, which is
  /// replaced here for the duration of the read. Without that the test framework would turn the
  /// report into a debug break and abort the run, so the component types after a broken one would
  /// never be checked.
  static Result RoundTripComponentType(const WRTTI* pRtti, WStringBuilder& out_sFailure)
  {
    out_sFailure.Clear();
    g_sReportedFailure.Clear();

    WDefaultMemoryStreamStorage storage;

    // Write a world that contains a single component of the type under test.
    {
      WWorldDesc desc("ComponentSerializationTest - Write");
      WWorld world(desc);
      W_LOCK(world.GetWriteMarker());

      WComponentManagerBase* pManager = world.GetOrCreateManagerForComponentType(pRtti);
      if (pManager == nullptr)
        return Result::Skipped;

      WGameObject* pObject = nullptr;
      WGameObjectDesc objectDesc;
      objectDesc.m_bDynamic = true;
      objectDesc.m_sName.Assign("TestObject");
      world.CreateObject(objectDesc, pObject);

      if (pManager->CreateComponent(pObject).IsInvalidated())
        return Result::Skipped;

      WMemoryStreamWriter writer(&storage);
      WWorldWriter worldWriter;
      worldWriter.WriteWorld(writer, world);
    }

    // Read it back into a fresh world, watching for the reader's own size check.
    {
      WWorldDesc desc("ComponentSerializationTest - Read");
      WWorld world(desc);
      W_LOCK(world.GetWriteMarker());

      WMemoryStreamReader reader(&storage);
      WWorldReader worldReader;
      if (worldReader.ReadWorldDescription(reader).Failed())
        return Result::Skipped;

      // The reader silently skips types it does not know about, in which case nothing was deserialized.
      if (!worldReader.HasComponentOfType(pRtti))
        return Result::Skipped;

      g_PreviousAssertHandler = WGetAssertHandler();
      WSetAssertHandler(SerializationAssertHandler);
      W_SCOPE_EXIT(WSetAssertHandler(g_PreviousAssertHandler));

      worldReader.InstantiateWorld(world);
    }

    if (!g_sReportedFailure.IsEmpty())
    {
      out_sFailure = g_sReportedFailure;
      return Result::SizeMismatch;
    }

    return Result::Ok;
  }
} // namespace ComponentSerializationTestDetail

static WGameEngineTestComponentSerialization s_GameEngineTestComponentSerialization;

const char* WGameEngineTestComponentSerialization::GetTestName() const
{
  return "Component Serialization Tests";
}

WGameEngineTestApplication* WGameEngineTestComponentSerialization::CreateApplication()
{
  // Uses a project without any plugin configuration: this test only needs a running application with
  // a graphics device, because creating a component also creates its manager and some managers
  // allocate GPU resources.
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "DynamicTextureAtlas");
  return m_pOwnApplication;
}

void WGameEngineTestComponentSerialization::SetupSubTests()
{
  AddSubTest("Serialize / Deserialize Roundtrip", SubTests::SerializeDeserializeRoundtrip);
}

WTestAppRun WGameEngineTestComponentSerialization::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  W_IGNORE_UNUSED(iIdentifier);
  W_IGNORE_UNUSED(uiInvocationCount);

  WDynamicArray<const WRTTI*> componentTypes;

  WRTTI::ForEachDerivedType<WComponent>(
    [&](const WRTTI* pRtti)
    {
      componentTypes.PushBack(pRtti);
    },
    // Components are constructed through their component manager, not through the RTTI allocator
    // (W_BEGIN_COMPONENT_TYPE uses WRTTINoAllocator), so ExcludeNonAllocatable must not be used here.
    WRTTI::ForEachOptions::ExcludeAbstract);

  // Sort by name so that the output is stable between runs.
  componentTypes.Sort([](const WRTTI* a, const WRTTI* b)
    { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  W_TEST_BOOL_MSG(!componentTypes.IsEmpty(), "No component types found, the test would silently pass.");

  WUInt32 uiTested = 0;
  WUInt32 uiSkipped = 0;

  for (const WRTTI* pRtti : componentTypes)
  {
    WStringBuilder sFailure;

    const auto res = ComponentSerializationTestDetail::RoundTripComponentType(pRtti, sFailure);

    if (res == ComponentSerializationTestDetail::Result::Skipped)
    {
      ++uiSkipped;
      continue;
    }

    ++uiTested;

    if (res != ComponentSerializationTestDetail::Result::Ok)
    {
      // The reported message already names the type and its version.
      W_TEST_FAILURE("Component serialization mismatch", "%s", sFailure.GetData());
    }
  }

  WLog::Info("Tested {} component types, skipped {}.", uiTested, uiSkipped);

  W_TEST_BOOL_MSG(uiTested > 0, "No component type could actually be tested.");

  return WTestAppRun::Quit;
}
