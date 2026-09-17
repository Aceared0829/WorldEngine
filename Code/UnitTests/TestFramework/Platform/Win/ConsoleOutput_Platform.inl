#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

void OutputToConsole_Platform(WUInt8 uiColor, WTestOutput::Enum type, WInt32 iIndentation, const char* szMsg)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (DWORD)uiColor);
  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);

  char sz[4096];
  WStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(WStringWChar(sz).GetData());
}
