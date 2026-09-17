#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetInfoFile.h>

W_CREATE_SIMPLE_TEST(Utility, AssetInfoFile)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "SetValue / GetValue")
  {
    WAssetInfoFile info;
    W_TEST_BOOL(info.IsEmpty());

    info.SetValue(WAssetInfoFile::Keys::NumTriangles, 1234u);
    W_TEST_BOOL(!info.IsEmpty());
    W_TEST_INT(info.GetValue(WAssetInfoFile::Keys::NumTriangles).Get<WUInt32>(), 1234);

    // overwriting replaces the value instead of adding a second entry
    info.SetValue(WAssetInfoFile::Keys::NumTriangles, 42u);
    W_TEST_INT(info.GetValue(WAssetInfoFile::Keys::NumTriangles).Get<WUInt32>(), 42);
    W_TEST_INT(info.GetValues().GetCount(), 1);

    // an unknown key yields an invalid variant, rather than asserting
    W_TEST_BOOL(!info.GetValue("DoesNotExist").IsValid());

    // an invalid value removes the key again
    info.SetValue(WAssetInfoFile::Keys::NumTriangles, WVariant());
    W_TEST_BOOL(info.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Write / Read")
  {
    WAssetInfoFile info;
    info.SetValue(WAssetInfoFile::Keys::NumVertices, 100u);
    info.SetValue(WAssetInfoFile::Keys::NumTriangles, 50u);
    // not exactly representable, to cover the writer's readable float mode
    info.SetValue(WAssetInfoFile::Keys::BoundsCenter, WVec3(0.477654f, -1.2345678f, 3));
    info.SetValue(WAssetInfoFile::Keys::BoundsHalfExtents, WVec3(4, 5, 6));
    info.SetValue(WAssetInfoFile::Keys::Format, WString("BC7"));
    info.SetValue(WAssetInfoFile::Keys::CollisionMeshType, WString("ConvexHull"));

    WAssetFileHeader header;
    header.SetFileHashAndVersion(0x1234567890ABCDEFull, 7);

    WDefaultMemoryStreamStorage storage;
    {
      WMemoryStreamWriter writer(&storage);
      W_TEST_BOOL(info.Write(writer, header).Succeeded());
    }

    WAssetInfoFile read;
    WAssetFileHeader readHeader;
    {
      WMemoryStreamReader reader(&storage);
      W_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }

    W_TEST_BOOL(readHeader.IsFileUpToDate(0x1234567890ABCDEFull, 7));
    W_TEST_INT(read.GetValues().GetCount(), 6);
    W_TEST_INT(read.GetValue(WAssetInfoFile::Keys::NumVertices).Get<WUInt32>(), 100);
    W_TEST_INT(read.GetValue(WAssetInfoFile::Keys::NumTriangles).Get<WUInt32>(), 50);
    W_TEST_VEC3(read.GetValue(WAssetInfoFile::Keys::BoundsCenter).Get<WVec3>(), WVec3(0.477654f, -1.2345678f, 3), 0.001f);
    W_TEST_VEC3(read.GetValue(WAssetInfoFile::Keys::BoundsHalfExtents).Get<WVec3>(), WVec3(4, 5, 6), 0.0f);
    W_TEST_STRING(read.GetValue(WAssetInfoFile::Keys::Format).Get<WString>(), "BC7");
    W_TEST_STRING(read.GetValue(WAssetInfoFile::Keys::CollisionMeshType).Get<WString>(), "ConvexHull");

    // reading replaces previous content rather than merging into it
    {
      WMemoryStreamReader reader(&storage);
      W_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }
    W_TEST_INT(read.GetValues().GetCount(), 6);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "String array round trip")
  {
    WVariantArray clips;
    clips.PushBack(WVariant(WString("Idle")));
    clips.PushBack(WVariant(WString("Walk")));
    clips.PushBack(WVariant(WString("Run")));

    WAssetInfoFile info;
    info.SetValue(WAssetInfoFile::Keys::AvailableClips, WVariant(clips));

    WAssetFileHeader header;
    header.SetFileHashAndVersion(1, 1);

    WDefaultMemoryStreamStorage storage;
    {
      WMemoryStreamWriter writer(&storage);
      W_TEST_BOOL(info.Write(writer, header).Succeeded());
    }

    WAssetInfoFile read;
    WAssetFileHeader readHeader;
    {
      WMemoryStreamReader reader(&storage);
      W_TEST_BOOL(read.Read(reader, readHeader).Succeeded());
    }

    const WVariant value = read.GetValue(WAssetInfoFile::Keys::AvailableClips);
    W_TEST_BOOL(value.IsA<WVariantArray>());

    const WVariantArray& readClips = value.Get<WVariantArray>();
    W_TEST_INT(readClips.GetCount(), 3);
    W_TEST_STRING(readClips[0].ConvertTo<WString>(), "Idle");
    W_TEST_STRING(readClips[1].ConvertTo<WString>(), "Walk");
    W_TEST_STRING(readClips[2].ConvertTo<WString>(), "Run");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AppendToDisplayString")
  {
    // nothing is appended for an empty set, not even a separator
    {
      WAssetInfoFile info;
      WStringBuilder s("Existing");
      info.AppendToDisplayString(s);
      W_TEST_STRING(s, "Existing");
    }

    // center and half extents collapse into a size and a range, rather than being listed raw
    {
      WAssetInfoFile info;
      info.SetValue(WAssetInfoFile::Keys::NumTriangles, 252u);
      info.SetValue(WAssetInfoFile::Keys::BoundsCenter, WVec3(0, 0, 0.5f));
      info.SetValue(WAssetInfoFile::Keys::BoundsHalfExtents, WVec3(0.25f, 0.25f, 0.5f));
      info.SetValue(WAssetInfoFile::Keys::BoundsRadius, 0.75f);

      WStringBuilder s;
      info.AppendToDisplayString(s, "\n");

      W_TEST_BOOL(s.FindSubString("Size: 0.500 x 0.500 x 1.000") != nullptr);
      W_TEST_BOOL(s.FindSubString("Bounds: -0.250 -0.250 0.000 to 0.250 0.250 1.000") != nullptr);
      W_TEST_BOOL(s.FindSubString("Triangles: 252") != nullptr);
      W_TEST_BOOL(s.FindSubString("Radius: 0.75") != nullptr);
      // the keys folded into the lines above must not also appear on their own
      W_TEST_BOOL(s.FindSubString("Center:") == nullptr);
      W_TEST_BOOL(s.FindSubString("Extents:") == nullptr);
    }

    // the bounds range needs both halves, so a center on its own prints nothing
    {
      WAssetInfoFile info;
      info.SetValue(WAssetInfoFile::Keys::BoundsCenter, WVec3(1, 2, 3));

      WStringBuilder s;
      info.AppendToDisplayString(s);
      W_TEST_BOOL(s.IsEmpty());
    }

    // width and height collapse into a single resolution line
    {
      WAssetInfoFile info;
      info.SetValue(WAssetInfoFile::Keys::ImageWidth, 512u);
      info.SetValue(WAssetInfoFile::Keys::ImageHeight, 256u);
      info.SetValue(WAssetInfoFile::Keys::Format, WString("BC7"));

      WStringBuilder s;
      info.AppendToDisplayString(s);

      W_TEST_BOOL(s.FindSubString("Resolution: 512 x 256") != nullptr);
      W_TEST_BOOL(s.FindSubString("Format: BC7") != nullptr);
      W_TEST_BOOL(s.FindSubString("Width:") == nullptr);
    }

    // a key this code does not know about is still shown, under its raw name
    {
      WAssetInfoFile info;
      info.SetValue("SomethingNew", 42u);

      WStringBuilder s;
      info.AppendToDisplayString(s);
      W_TEST_BOOL(s.FindSubString("SomethingNew: 42") != nullptr);
    }

    // an array is summarized by its element count, not by listing the elements
    {
      WVariantArray clips;
      clips.PushBack(WVariant(WString("Idle")));
      clips.PushBack(WVariant(WString("Walk")));

      WAssetInfoFile info;
      info.SetValue(WAssetInfoFile::Keys::AvailableClips, WVariant(clips));

      WStringBuilder s;
      info.AppendToDisplayString(s);

      W_TEST_BOOL(s.FindSubString("Clips: 2") != nullptr);
      W_TEST_BOOL(s.FindSubString("Idle") == nullptr);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AppendValuesToDisplayString")
  {
    WAssetInfoFile info;
    info.SetValue(WAssetInfoFile::Keys::NumVertices, 4460u);
    info.SetValue(WAssetInfoFile::Keys::NumTriangles, 5238u);
    info.SetValue(WAssetInfoFile::Keys::NumSubMeshes, 1u);
    info.SetValue(WAssetInfoFile::Keys::BoundsCenter, WVec3(0, 0, 0.5f));
    info.SetValue(WAssetInfoFile::Keys::BoundsHalfExtents, WVec3(0.25f, 0.25f, 0.5f));

    // only the requested keys appear, so that a caller can keep a tooltip short
    {
      const WStringView keys[] = {WAssetInfoFile::Keys::NumTriangles, WAssetInfoFile::Keys::BoundsHalfExtents};

      WStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      W_TEST_BOOL(s.FindSubString("Triangles: 5238") != nullptr);
      W_TEST_BOOL(s.FindSubString("Size: 0.500 x 0.500 x 1.000") != nullptr);
      W_TEST_BOOL(s.FindSubString("Vertices:") == nullptr);
      W_TEST_BOOL(s.FindSubString("Meshes:") == nullptr);
      // asking for the size must not also print the full coordinate range
      W_TEST_BOOL(s.FindSubString("Bounds:") == nullptr);
    }

    // the requested order is kept, rather than the order inside the file
    {
      const WStringView keys[] = {WAssetInfoFile::Keys::NumSubMeshes, WAssetInfoFile::Keys::NumVertices};

      WStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      const char* szSub = s.FindSubString("Meshes:");
      const char* szVert = s.FindSubString("Vertices:");
      W_TEST_BOOL(szSub != nullptr && szVert != nullptr && szSub < szVert);
    }

    // a key with no value is skipped instead of printing an empty line
    {
      const WStringView keys[] = {WAssetInfoFile::Keys::Format, WAssetInfoFile::Keys::NumTriangles};

      WStringBuilder s;
      info.AppendValuesToDisplayString(s, keys);

      W_TEST_BOOL(s.FindSubString("Format:") == nullptr);
      W_TEST_STRING(s, "\nTriangles: 5238");
    }

    // asking for the absorbed half of a combined pair yields nothing on its own
    {
      WAssetInfoFile tex;
      tex.SetValue(WAssetInfoFile::Keys::ImageWidth, 512u);
      tex.SetValue(WAssetInfoFile::Keys::ImageHeight, 256u);

      WStringBuilder sHeight;
      const WStringView keysHeight[] = {WAssetInfoFile::Keys::ImageHeight};
      W_TEST_BOOL(!tex.AppendValueToDisplayString(sHeight, WAssetInfoFile::Keys::ImageHeight));
      tex.AppendValuesToDisplayString(sHeight, keysHeight);
      W_TEST_BOOL(sHeight.IsEmpty());

      // while the other half prints the whole resolution
      WStringBuilder sWidth;
      W_TEST_BOOL(tex.AppendValueToDisplayString(sWidth, WAssetInfoFile::Keys::ImageWidth));
      W_TEST_BOOL(sWidth.FindSubString("Resolution: 512 x 256") != nullptr);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetInfoFilePathForOutput")
  {
    W_TEST_STRING(WAssetInfoFile::GetInfoFilePathForOutput("C:/Foo/Bar.WMesh"), "C:/Foo/Bar.WMesh.WAssetInfo");
  }
}
