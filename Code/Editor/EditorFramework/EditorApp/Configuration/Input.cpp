#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void WQtEditorApp::GetKnownInputSlots(WDynamicArray<WString>& ref_slotList) const
{
  if (ref_slotList.IndexOf("") == WInvalidIndex)
    ref_slotList.PushBack("");

  WStringBuilder sFile;
  WDynamicArray<WStringView> Lines;

  WStringBuilder sSearchDir = WApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("InputSlots/*.txt");

  WFileSystemIterator it;
  for (it.StartSearch(sSearchDir, WFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    sFile = it.GetCurrentPath();
    sFile.AppendPath(it.GetStats().m_sName);

    WFileReader reader;
    if (reader.Open(sFile).Succeeded())
    {
      sFile.ReadAll(reader);

      Lines.Clear();
      sFile.Split(false, Lines, "\n", "\r");

      WString sSlot;
      for (WUInt32 s = 0; s < Lines.GetCount(); ++s)
      {
        sSlot = Lines[s];

        if (ref_slotList.IndexOf(sSlot) == WInvalidIndex)
          ref_slotList.PushBack(sSlot);
      }
    }
  }
}
