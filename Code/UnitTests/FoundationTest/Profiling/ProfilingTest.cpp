#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/ThreadUtils.h>

namespace
{
  void WriteOutProfilingCapture(const char* szFilePath)
  {
    WStringBuilder outputPath = WTestFramework::GetInstance()->GetAbsOutputPath();
    W_TEST_BOOL(WFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

    WFileWriter fileWriter;
    if (fileWriter.Open(szFilePath) == W_SUCCESS)
    {
      WProfilingSystem::ProfilingData profilingData;
      WProfilingSystem::Capture(profilingData);
      profilingData.Write(fileWriter).IgnoreResult();
      WLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
} // namespace

W_CREATE_SIMPLE_TEST_GROUP(Profiling);

W_CREATE_SIMPLE_TEST(Profiling, Profiling)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Nested scopes")
  {
    WProfilingSystem::Clear();

    {
      W_PROFILE_SCOPE("Prewarm scope");
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(1));
    }

    WTime endTime = WTime::Now() + WTime::MakeFromMilliseconds(1);

    {
      W_PROFILE_SCOPE("Outer scope");

      {
        W_PROFILE_SCOPE("Inner scope");

        while (WTime::Now() < endTime)
        {
        }
      }
    }

    WriteOutProfilingCapture(":output/profilingScopes.json");
  }
}
