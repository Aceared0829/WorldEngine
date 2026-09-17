#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>

W_CREATE_SIMPLE_TEST(Utility, CommandLineUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "GetParameterCount / GetParameter")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    W_TEST_INT(CmdLn.GetParameterCount(), 9);
    W_TEST_STRING(CmdLn.GetParameter(0), "bla/blub/myprogram.exe");
    W_TEST_STRING(CmdLn.GetParameter(1), "-Test1");
    W_TEST_STRING(CmdLn.GetParameter(2), "true");
    W_TEST_STRING(CmdLn.GetParameter(3), "-Test2");
    W_TEST_STRING(CmdLn.GetParameter(4), "off");
    W_TEST_STRING(CmdLn.GetParameter(5), "-Test3");
    W_TEST_STRING(CmdLn.GetParameter(6), "-Test4");
    W_TEST_STRING(CmdLn.GetParameter(7), "on");
    W_TEST_STRING(CmdLn.GetParameter(8), "-Test5");
    CmdLn.InjectCustomArgument("-duh");
    W_TEST_INT(CmdLn.GetParameterCount(), 10);
    W_TEST_STRING(CmdLn.GetParameter(9), "-duh");
    CmdLn.InjectCustomArgument("I need my Space");
    W_TEST_INT(CmdLn.GetParameterCount(), 11);
    W_TEST_STRING(CmdLn.GetParameter(10), "I need my Space");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetOptionIndex / GetStringOptionArguments  / GetStringOption")
  {
    const int argc = 15;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-opt1", "true", "false", "-opt2", "\"test2\"", "-opt3", "-opt4", "one", "two = three",
      "four", "   five  ", " six ", "-opt5", "-opt6"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    W_TEST_INT(CmdLn.GetOptionIndex("-opt1"), 1);
    W_TEST_INT(CmdLn.GetOptionIndex("-opt2"), 4);
    W_TEST_INT(CmdLn.GetOptionIndex("-opt3"), 6);
    W_TEST_INT(CmdLn.GetOptionIndex("-opt4"), 7);
    W_TEST_INT(CmdLn.GetOptionIndex("-opt5"), 13);
    W_TEST_INT(CmdLn.GetOptionIndex("-opt6"), 14);

    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt1"), 2);
    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt2"), 1);
    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt3"), 0);
    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt4"), 5);
    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt5"), 0);
    W_TEST_INT(CmdLn.GetStringOptionArguments("-opt6"), 0);

    W_TEST_STRING(CmdLn.GetStringOption("-opt1", 0), "true");
    W_TEST_STRING(CmdLn.GetStringOption("-opt1", 1), "false");
    W_TEST_STRING(CmdLn.GetStringOption("-opt1", 2, "end"), "end");

    // GetStringOption will trim " at the start and end to match the windows command line parsing behaviour on all platforms.
    W_TEST_STRING(CmdLn.GetStringOption("-opt2", 0), "test2");
    W_TEST_STRING(CmdLn.GetStringOption("-opt2", 1, "end"), "end");

    W_TEST_STRING(CmdLn.GetStringOption("-opt3", 0, "end"), "end");

    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 0), "one");
    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 1), "two = three");
    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 2), "four");
    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 3), "   five  ");
    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 4), " six ");
    W_TEST_STRING(CmdLn.GetStringOption("-opt4", 5, "end"), "end");

    W_TEST_STRING(CmdLn.GetStringOption("-opt5", 0), "");

    W_TEST_STRING(CmdLn.GetStringOption("-opt6", 0), "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetBoolOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    W_TEST_BOOL(CmdLn.GetBoolOption("-test1", true, true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-test1", false, true) == false);

    W_TEST_BOOL(CmdLn.GetBoolOption("-test2", true, true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-test2", false, true) == false);

    // case insensitive and wrong
    W_TEST_BOOL(CmdLn.GetBoolOption("-test1", true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-test1", false) == true);

    W_TEST_BOOL(CmdLn.GetBoolOption("-test2", true) == false);
    W_TEST_BOOL(CmdLn.GetBoolOption("-test2", false) == false);

    // case sensitive and correct
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test1", true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test1", false) == true);

    W_TEST_BOOL(CmdLn.GetBoolOption("-Test2", true) == false);
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test2", false) == false);

    W_TEST_BOOL(CmdLn.GetBoolOption("-Test3", true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test3", false) == true);

    W_TEST_BOOL(CmdLn.GetBoolOption("-Test4", true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test4", false) == true);

    W_TEST_BOOL(CmdLn.GetBoolOption("-Test5", true) == true);
    W_TEST_BOOL(CmdLn.GetBoolOption("-Test5", false) == true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetIntOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    W_TEST_INT(CmdLn.GetIntOption("-test1", 2, true), 2);
    W_TEST_INT(CmdLn.GetIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    W_TEST_INT(CmdLn.GetIntOption("-test1", 2), 23);
    W_TEST_INT(CmdLn.GetIntOption("-test2", 17), 42);

    // case sensitive and correct
    W_TEST_INT(CmdLn.GetIntOption("-Test1", 2), 23);
    W_TEST_INT(CmdLn.GetIntOption("-Test2", 3), 42);
    W_TEST_INT(CmdLn.GetIntOption("-Test3", 4), 4);
    W_TEST_INT(CmdLn.GetIntOption("-Test4", 5), -11);
    W_TEST_INT(CmdLn.GetIntOption("-Test5"), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetUIntOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    W_TEST_INT(CmdLn.GetUIntOption("-test1", 2, true), 2);
    W_TEST_INT(CmdLn.GetUIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    W_TEST_INT(CmdLn.GetUIntOption("-test1", 2), 23);
    W_TEST_INT(CmdLn.GetUIntOption("-test2", 17), 42);

    // case sensitive and correct
    W_TEST_INT(CmdLn.GetUIntOption("-Test1", 2), 23);
    W_TEST_INT(CmdLn.GetUIntOption("-Test2", 3), 42);
    W_TEST_INT(CmdLn.GetUIntOption("-Test3", 4), 4);
    W_TEST_INT(CmdLn.GetUIntOption("-Test4", 5), 5);
    W_TEST_INT(CmdLn.GetUIntOption("-Test5"), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFloatOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23.45", "-Test2", "42.3", "-Test3", "-Test4", "-11", "-Test5"};

    WCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3, true), 2.3, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8, true), 17.8, 0.0);

    // case insensitive and wrong
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3), 23.45, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8), 42.3, 0.0);

    // case sensitive and correct
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-Test1", 2.3), 23.45, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-Test2", 3.4), 42.3, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-Test3", 4.5), 4.5, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-Test4", 5.6), -11, 0.0);
    W_TEST_DOUBLE(CmdLn.GetFloatOption("-Test5"), 0, 0.0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "SplitCommandLineString")
  {
    // Simple unquoted arguments.
    {
      WDynamicArray<WString> args;
      WDynamicArray<const char*> argv;
      WCommandLineUtils::SplitCommandLineString("-run -noGui -all", false, args, argv);

      W_TEST_INT(args.GetCount(), 3);
      W_TEST_STRING(args[0], "-run");
      W_TEST_STRING(args[1], "-noGui");
      W_TEST_STRING(args[2], "-all");
      W_TEST_INT(argv.GetCount(), 3);
    }

    // Quoted argument with spaces.
    {
      WDynamicArray<WString> args;
      WDynamicArray<const char*> argv;
      WCommandLineUtils::SplitCommandLineString("-output \"C:/My Folder/file.txt\" -verbose", false, args, argv);

      W_TEST_INT(args.GetCount(), 3);
      W_TEST_STRING(args[0], "-output");
      W_TEST_STRING(args[1], "C:/My Folder/file.txt");
      W_TEST_STRING(args[2], "-verbose");
    }

    // Quoted argument without spaces (quotes should still be stripped).
    {
      WDynamicArray<WString> args;
      WDynamicArray<const char*> argv;
      WCommandLineUtils::SplitCommandLineString("-name \"hello\" -count 3", false, args, argv);

      W_TEST_INT(args.GetCount(), 4);
      W_TEST_STRING(args[0], "-name");
      W_TEST_STRING(args[1], "hello");
      W_TEST_STRING(args[2], "-count");
      W_TEST_STRING(args[3], "3");
    }

    // Multiple spaces between arguments.
    {
      WDynamicArray<WString> args;
      WDynamicArray<const char*> argv;
      WCommandLineUtils::SplitCommandLineString("  -a   -b  ", false, args, argv);

      W_TEST_INT(args.GetCount(), 2);
      W_TEST_STRING(args[0], "-a");
      W_TEST_STRING(args[1], "-b");
    }

    // Empty string.
    {
      WDynamicArray<WString> args;
      WDynamicArray<const char*> argv;
      WCommandLineUtils::SplitCommandLineString("", false, args, argv);

      W_TEST_INT(args.GetCount(), 0);
      W_TEST_INT(argv.GetCount(), 0);
    }
  }
}
