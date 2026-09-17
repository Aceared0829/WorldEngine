
void OutputToConsole_Platform(WUInt8 uiColor, WTestOutput::Enum type, WInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
}
