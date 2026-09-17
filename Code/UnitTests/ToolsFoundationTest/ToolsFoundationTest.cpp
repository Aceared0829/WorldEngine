#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

WInt32 WConstructionCounter::s_iConstructions = 0;
WInt32 WConstructionCounter::s_iDestructions = 0;
WInt32 WConstructionCounter::s_iConstructionsLast = 0;
WInt32 WConstructionCounter::s_iDestructionsLast = 0;

W_TESTFRAMEWORK_ENTRY_POINT("ToolsFoundationTest", "Tools Foundation Tests")
