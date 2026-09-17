#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>

WResult WProfilingUtils::SaveProfilingCapture(WStringView sCapturePath)
{
  WFileWriter fileWriter;
  if (fileWriter.Open(sCapturePath) == W_SUCCESS)
  {
    WProfilingSystem::ProfilingData profilingData;
    WProfilingSystem::Capture(profilingData);
    // Set sort index to WInvalidIndex so that the runtime process is always at the bottom and editor is always on top when opening the trace.
    profilingData.m_uiProcessSortIndex = WInvalidIndex;
    if (profilingData.Write(fileWriter).Failed())
    {
      WLog::Error("Failed to write profiling capture: {0}.", sCapturePath);
      return W_FAILURE;
    }

    WLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  }
  else
  {
    WLog::Error("Could not write profiling capture to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    return W_FAILURE;
  }
  return W_SUCCESS;
}

WResult WProfilingUtils::MergeProfilingCaptures(WStringView sCapturePath1, WStringView sCapturePath2, WStringView sMergedCapturePath)
{
  WString sFirstProfilingJson;
  {
    WFileReader reader;
    if (reader.Open(sCapturePath1).Failed())
    {
      WLog::Error("Failed to read first profiling capture to be merged: {}.", sCapturePath1);
      return W_FAILURE;
    }
    sFirstProfilingJson.ReadAll(reader);
  }
  WString sSecondProfilingJson;
  {
    WFileReader reader;
    if (reader.Open(sCapturePath2).Failed())
    {
      WLog::Error("Failed to read second profiling capture to be merged: {}.", sCapturePath2);
      return W_FAILURE;
    }
    sSecondProfilingJson.ReadAll(reader);
  }

  WStringBuilder sMergedProfilingJson;
  {
    // Just glue the array together
    sMergedProfilingJson.Reserve(sFirstProfilingJson.GetElementCount() + 1 + sSecondProfilingJson.GetElementCount());
    const char* szEndArray = sFirstProfilingJson.FindLastSubString("]");
    sMergedProfilingJson.Append(WStringView(sFirstProfilingJson.GetData(), static_cast<WUInt32>(szEndArray - sFirstProfilingJson.GetData())));
    sMergedProfilingJson.Append(",");
    const char* szStartArray = sSecondProfilingJson.FindSubString("[") + 1;
    sMergedProfilingJson.Append(WStringView(szStartArray, static_cast<WUInt32>(sSecondProfilingJson.GetElementCount() - (szStartArray - sSecondProfilingJson.GetData()))));
  }

  WFileWriter fileWriter;
  if (fileWriter.Open(sMergedCapturePath).Failed() || fileWriter.WriteBytes(sMergedProfilingJson.GetData(), sMergedProfilingJson.GetElementCount()).Failed())
  {
    WLog::Error("Failed to write merged profiling capture: {}.", sMergedCapturePath);
    return W_FAILURE;
  }
  WLog::Info("Merged profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
  return W_SUCCESS;
}
