#include <android/log.h>

void OutputToConsole_Platform(WUInt8 uiColor, WTestOutput::Enum type, WInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
  __android_log_print(ANDROID_LOG_DEBUG, "WorldEngine", "%*s%s\n", iIndentation, "", szMsg);
}
