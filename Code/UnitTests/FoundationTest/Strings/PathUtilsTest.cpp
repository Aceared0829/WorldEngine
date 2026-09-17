#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Strings/String.h>

W_CREATE_SIMPLE_TEST(Strings, PathUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "IsPathSeparator")
  {
    for (int i = 0; i < 0xFFFF; ++i)
    {
      if (i == '/')
      {
        W_TEST_BOOL(WPathUtils::IsPathSeparator(i));
      }
      else if (i == '\\')
      {
        W_TEST_BOOL(WPathUtils::IsPathSeparator(i));
      }
      else
      {
        W_TEST_BOOL(!WPathUtils::IsPathSeparator(i));
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindPreviousSeparator")
  {
    const char* szPath = "This/Is\\My//Path.dot\\file.extension";

    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 35) == szPath + 20);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 20) == szPath + 11);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 11) == szPath + 10);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 10) == szPath + 7);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 7) == szPath + 4);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath + 4) == nullptr);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(szPath, szPath) == nullptr);
    W_TEST_BOOL(WPathUtils::FindPreviousSeparator(nullptr, nullptr) == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileExtension")
  {
    W_TEST_BOOL(WPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    W_TEST_BOOL(WPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("") == "");

    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar.txt") == "txt");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar.") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar.txt/bar.cc") == "cc");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar.txt/bar.") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/bar.txt/bar") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/.") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/..") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/.hidden") == "");
    W_TEST_BOOL(WPathUtils::GetFileExtension("/foo/..bar") == "");

    W_TEST_BOOL(WPathUtils::GetFileExtension("foo.bar.baz.tar") == "tar");
    W_TEST_BOOL(WPathUtils::GetFileExtension("foo.bar.baz") == "baz");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasAnyExtension")
  {
    W_TEST_BOOL(WPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension(""));

    W_TEST_BOOL(WPathUtils::HasAnyExtension("/foo/bar.txt"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/bar."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/bar"));
    W_TEST_BOOL(WPathUtils::HasAnyExtension("/foo/bar.txt/bar.cc"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/bar.txt/bar."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/bar.txt/bar"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension(".."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/.."));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/.hidden"));
    W_TEST_BOOL(!WPathUtils::HasAnyExtension("/foo/..bar"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "HasExtension")
  {
    W_TEST_BOOL(WPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    W_TEST_BOOL(WPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    W_TEST_BOOL(!WPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    W_TEST_BOOL(!WPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    W_TEST_BOOL(!WPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    W_TEST_BOOL(!WPathUtils::HasExtension("", "ext"));

    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar.txt", "txt"));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar.", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar.txt/bar.cc", "cc"));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar.txt/bar.", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/bar.txt/bar", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/.", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/..", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/.hidden", ""));
    W_TEST_BOOL(WPathUtils::HasExtension("/foo/..bar", ""));
    W_TEST_BOOL(!WPathUtils::HasExtension(".file", ".file"));
    W_TEST_BOOL(!WPathUtils::HasExtension(".file", "file"));
    W_TEST_BOOL(!WPathUtils::HasExtension("folder/.file", ".file"));
    W_TEST_BOOL(!WPathUtils::HasExtension("folder/.file", "file"));

    W_TEST_BOOL(WPathUtils::HasExtension("foo.bar.baz.tar", "tar"));
    W_TEST_BOOL(WPathUtils::HasExtension("foo.bar.baz", "baz"));

    W_TEST_BOOL(WPathUtils::HasExtension("file.txt", "txt"));
    W_TEST_BOOL(WPathUtils::HasExtension("file.txt", ".txt"));
    W_TEST_BOOL(WPathUtils::HasExtension("file.a.b", ".b"));
    W_TEST_BOOL(WPathUtils::HasExtension("file.a.b", "a.b"));
    W_TEST_BOOL(WPathUtils::HasExtension("file.a.b", ".a.b"));
    W_TEST_BOOL(!WPathUtils::HasExtension("file.a.b", "file.a.b"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileNameAndExtension")
  {
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file.extension") == "file.extension");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\.extension") == ".extension");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file") == "file");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("\\file") == "file");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("") == "");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("/") == "");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\") == "");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("file") == "file");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("file.ext") == "file.ext");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension(".stupidfile") == ".stupidfile");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("folder/.") == ".");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("folder/..") == "..");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension(".") == ".");
    W_TEST_BOOL(WPathUtils::GetFileNameAndExtension("..") == "..");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileName")
  {
    W_TEST_BOOL(WPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    W_TEST_BOOL(WPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    W_TEST_BOOL(WPathUtils::GetFileName("\\file") == "file");
    W_TEST_BOOL(WPathUtils::GetFileName("") == "");
    W_TEST_BOOL(WPathUtils::GetFileName("/") == "");
    W_TEST_BOOL(WPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    W_TEST_BOOL(WPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == ".stupidfile");
    W_TEST_BOOL(WPathUtils::GetFileName(".stupidfile") == ".stupidfile");

    W_TEST_BOOL(WPathUtils::GetFileName("File.ext") == "File");
    W_TEST_BOOL(WPathUtils::GetFileName("File.") == "File.");
    W_TEST_BOOL(WPathUtils::GetFileName("File.ext.") == "File.ext.");

    W_TEST_BOOL(WPathUtils::GetFileName("folder/.") == ".");
    W_TEST_BOOL(WPathUtils::GetFileName("folder/..") == "..");
    W_TEST_BOOL(WPathUtils::GetFileName(".") == ".");
    W_TEST_BOOL(WPathUtils::GetFileName("..") == "..");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetFileDirectory")
  {
    W_TEST_BOOL(WPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file.extension") == "This/Is\\My//Path.dot\\");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\.extension") == "This/Is\\My//Path.dot\\");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file") == "This/Is\\My//Path.dot\\");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("\\file") == "\\");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("") == "");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("/") == "/");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\") == "This/Is\\My//Path.dot\\");
    W_TEST_BOOL(WPathUtils::GetFileDirectory("This") == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsAbsolutePath")
  {
#if W_ENABLED(W_PLATFORM_WINDOWS)
    W_TEST_BOOL(WPathUtils::IsAbsolutePath("C:\\temp.stuff"));
    W_TEST_BOOL(WPathUtils::IsAbsolutePath("C:/temp.stuff"));
    W_TEST_BOOL(WPathUtils::IsAbsolutePath("\\\\myserver\\temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("\\myserver\\temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("/temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("\\temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("..\\temp.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath(".\\temp.stuff"));
#else
    W_TEST_BOOL(WPathUtils::IsAbsolutePath("/usr/local/.stuff"));
    W_TEST_BOOL(WPathUtils::IsAbsolutePath("/file.test"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("./file.stuff"));
    W_TEST_BOOL(!WPathUtils::IsAbsolutePath("file.stuff"));
#endif
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetRootedPathParts")
  {
    WStringView root, relPath;
    WPathUtils::GetRootedPathParts(":MyRoot\\folder\\file.txt", root, relPath);
    W_TEST_BOOL(WPathUtils::GetRootedPathRootName(":MyRoot\\folder\\file.txt") == root);
    W_TEST_BOOL(root == "MyRoot");
    W_TEST_BOOL(relPath == "folder\\file.txt");

    WPathUtils::GetRootedPathParts("folder\\file2.txt", root, relPath);
    W_TEST_BOOL(root.IsEmpty());
    W_TEST_BOOL(relPath == "folder\\file2.txt");

    WPathUtils::GetRootedPathParts(":root", root, relPath);
    W_TEST_BOOL(root == "root");
    W_TEST_BOOL(relPath == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NormalizeWindowsDriveLetter")
  {
    auto Normalized = [](const char* szPath) -> WStringBuilder
    {
      WStringBuilder s = szPath;
      WPathUtils::NormalizeWindowsDriveLetter(s);
      return s;
    };

    W_TEST_STRING(Normalized("c:/DataDir/File.txt"), "C:/DataDir/File.txt");
    W_TEST_STRING(Normalized("C:/DataDir/File.txt"), "C:/DataDir/File.txt");
    W_TEST_STRING(Normalized("z:"), "Z:");
    W_TEST_STRING(Normalized("c:"), "C:");

    // the rest of the path is not touched
    W_TEST_STRING(Normalized("c:/dataDIR/fILE.txt"), "C:/dataDIR/fILE.txt");

    // nothing that looks like a drive letter
    W_TEST_STRING(Normalized(""), "");
    W_TEST_STRING(Normalized("c"), "c");
    W_TEST_STRING(Normalized("/usr/local"), "/usr/local");
    W_TEST_STRING(Normalized("relative/path.txt"), "relative/path.txt");
    W_TEST_STRING(Normalized("1:/foo"), "1:/foo");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsSubPath")
  {
    W_TEST_BOOL(WPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:/DataDir/", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:/DataDir", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:/DataDir/", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath("C:/DataDir", "C:/DataDir2"));

    W_TEST_BOOL(WPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/"));
    W_TEST_BOOL(WPathUtils::IsSubPath("C:\\DataDir\\", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir2"));

    W_TEST_BOOL(!WPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(!WPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir"));
    W_TEST_BOOL(!WPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir2"));

    W_TEST_BOOL(!WPathUtils::IsSubPath("C:/DataDir/SomeFolder", "C:/DataDir"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsSubPath_NoCase")
  {
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir2"));

    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDir\\", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir2"));

    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir"));
    W_TEST_BOOL(WPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/"));
    W_TEST_BOOL(!WPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir2"));

    W_TEST_BOOL(!WPathUtils::IsSubPath_NoCase("C:/DataDir/SomeFolder", "C:/DataDir"));
  }
}
