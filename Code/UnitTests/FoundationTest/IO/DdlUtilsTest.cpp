#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

static WVariant CreateVariant(WVariant::Type::Enum t, const void* pData);

W_CREATE_SIMPLE_TEST(IO, DdlUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToColor")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WColor c1, c2, c3, c4, c5, c6, c7, c8, c0;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("t0"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c1"), c1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c2"), c2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c3"), c3).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c4"), c4).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c5"), c5).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c6"), c6).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c7"), c7).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c8"), c8).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c9"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c10"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c11"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c12"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColor(doc.FindElement("c13"), c0).Failed());

    W_TEST_BOOL(c1 == WColor(1, 0, 0.5f, 1.0f));
    W_TEST_BOOL(c2 == WColor(2, 1, 1.5f, 0.1f));
    W_TEST_BOOL(c3 == WColorGammaUB(128, 2, 32));
    W_TEST_BOOL(c4 == WColorGammaUB(128, 0, 32, 64));
    W_TEST_BOOL(c5 == WColor(1, 0, 0.5f, 1.0f));
    W_TEST_BOOL(c6 == WColor(2, 1, 1.5f, 0.1f));
    W_TEST_BOOL(c7 == WColorGammaUB(128, 2, 32));
    W_TEST_BOOL(c8 == WColorGammaUB(128, 0, 32, 64));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToColorGamma")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WColorGammaUB c1, c2, c3, c4, c5, c6, c7, c8, c0;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("t0"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c1"), c1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c2"), c2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c3"), c3).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c4"), c4).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c5"), c5).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c6"), c6).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c7"), c7).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c8"), c8).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c9"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c10"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c11"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c12"), c0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c13"), c0).Failed());

    W_TEST_BOOL(c1 == WColorGammaUB(WColor(1, 0, 0.5f, 1.0f)));
    W_TEST_BOOL(c2 == WColorGammaUB(WColor(2, 1, 1.5f, 0.1f)));
    W_TEST_BOOL(c3 == WColorGammaUB(128, 2, 32));
    W_TEST_BOOL(c4 == WColorGammaUB(128, 0, 32, 64));
    W_TEST_BOOL(c5 == WColorGammaUB(WColor(1, 0, 0.5f, 1.0f)));
    W_TEST_BOOL(c6 == WColorGammaUB(WColor(2, 1, 1.5f, 0.1f)));
    W_TEST_BOOL(c7 == WColorGammaUB(128, 2, 32));
    W_TEST_BOOL(c8 == WColorGammaUB(128, 0, 32, 64));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToTime")
  {
    const char* szTestData = "\
Time $t1 { float { 0.1 } }\
Time $t2 { double { 0.2 } }\
float $t3 { 0.3 }\
double $t4 { 0.4 }\
Time $t5 { double { 0.2, 2 } }\
Time $t6 { int8 { 0, 2 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WTime t1, t2, t3, t4, t0;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t0"), t0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t1"), t1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t2"), t2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t3"), t3).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t4"), t4).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t5"), t0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTime(doc.FindElement("t6"), t0).Failed());

    W_TEST_FLOAT(t1.GetSeconds(), 0.1, 0.0001f);
    W_TEST_FLOAT(t2.GetSeconds(), 0.2, 0.0001f);
    W_TEST_FLOAT(t3.GetSeconds(), 0.3, 0.0001f);
    W_TEST_FLOAT(t4.GetSeconds(), 0.4, 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToVec2")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2 } }\
float $v2 { 0.3, 3 }\
Vector $v3 { float { 0.1 } }\
Vector $v4 { float { 0.1, 2.2, 3.33 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WVec2 v0, v1, v2;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec2(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec2(doc.FindElement("v1"), v1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec2(doc.FindElement("v2"), v2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec2(doc.FindElement("v3"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec2(doc.FindElement("v4"), v0).Failed());

    W_TEST_VEC2(v1, WVec2(0.1f, 2.0f), 0.0001f);
    W_TEST_VEC2(v2, WVec2(0.3f, 3.0f), 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToVec3")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2 } }\
float $v2 { 0.3, 3,0}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33,44 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WVec3 v0, v1, v2;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec3(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec3(doc.FindElement("v1"), v1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec3(doc.FindElement("v2"), v2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec3(doc.FindElement("v3"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec3(doc.FindElement("v4"), v0).Failed());

    W_TEST_VEC3(v1, WVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    W_TEST_VEC3(v2, WVec3(0.3f, 3.0f, 0.0f), 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToVec4")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WVec4 v0, v1, v2;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec4(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec4(doc.FindElement("v1"), v1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec4(doc.FindElement("v2"), v2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec4(doc.FindElement("v3"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVec4(doc.FindElement("v4"), v0).Failed());

    W_TEST_VEC4(v1, WVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    W_TEST_VEC4(v2, WVec4(0.3f, 3.0f, 0.0f, 12.0f), 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToMat3")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WMat3 v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToMat3(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToMat3(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_BOOL(v1.IsEqual(WMat3::MakeFromValues(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToMat4")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WMat4 v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToMat4(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToMat4(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_BOOL(v1.IsEqual(WMat4T::MakeFromValues(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToTransform")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WTransform v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToTransform(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTransform(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_VEC3(v1.m_vPosition, WVec3(1, 2, 3), 0.0001f);
    W_TEST_BOOL(v1.m_qRotation == WQuat(4, 5, 6, 7));
    W_TEST_VEC3(v1.m_vScale, WVec3(8, 9, 10), 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToQuat")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WQuat v0, v1, v2;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToQuat(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToQuat(doc.FindElement("v1"), v1).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToQuat(doc.FindElement("v2"), v2).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToQuat(doc.FindElement("v3"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToQuat(doc.FindElement("v4"), v0).Failed());

    W_TEST_BOOL(v1 == WQuat(0.1f, 2.0f, 3.2f, 44.5f));
    W_TEST_BOOL(v2 == WQuat(0.3f, 3.0f, 0.0f, 12.0f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToUuid")
  {
    const char* szTestData = "\
Data $v1 { unsigned_int64 { 12345678910, 10987654321 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WUuid v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToUuid(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToUuid(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_BOOL(v1 == WUuid(12345678910, 10987654321));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToAngle")
  {
    const char* szTestData = "\
Data $v1 { float { 45.23 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WAngle v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToAngle(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToAngle(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_FLOAT(v1.GetRadian(), 45.23f, 0.0001f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToHashedString")
  {
    const char* szTestData = "\
Data $v1 { string { \"Hello World\" } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WHashedString v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToHashedString(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToHashedString(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_STRING(v1.GetView(), "Hello World");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToTempHashedString")
  {
    const char* szTestData = "\
Data $v1 { uint64 { 2720389094277464445 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WTempHashedString v0, v1;

    W_TEST_BOOL(WOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v0"), v0).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v1"), v1).Succeeded());

    W_TEST_BOOL(v1 == WTempHashedString("GHIJK"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WOpenDdlUtils::ConvertToVariant")
  {
    const char* szTestData = "\
Color $v1 { float { 1, 0, 0.5 } }\
ColorGamma $v2 { unsigned_int8 { 128, 0, 32, 64 } }\
Time $v3 { float { 0.1 } }\
Vec2 $v4 { float { 0.1, 2 } }\
Vec3 $v5 { float { 0.1, 2, 3.2 } }\
Vec4 $v6 { float { 0.1, 2, 3.2, 44.5 } }\
Mat3 $v7 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
Mat4 $v8 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
Transform $v9 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
Quat $v10 { float { 0.1, 2, 3.2, 44.5 } }\
Uuid $v11 { unsigned_int64 { 12345678910, 10987654321 } }\
Angle $v12 { float { 45.23 } }\
HashedString $v13 { string { \"Soo much string\" } }\
TempHashedString $v14 { uint64 { 2720389094277464445 } }\
";

    StringStream stream(szTestData);
    WOpenDdlReader doc;
    W_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    WVariant v[15];

    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v0"), v[0]).Failed());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v1"), v[1]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v2"), v[2]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v3"), v[3]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v4"), v[4]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v5"), v[5]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v6"), v[6]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v7"), v[7]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v8"), v[8]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v9"), v[9]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v10"), v[10]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v11"), v[11]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v12"), v[12]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v13"), v[13]).Succeeded());
    W_TEST_BOOL(WOpenDdlUtils::ConvertToVariant(doc.FindElement("v14"), v[14]).Succeeded());

    W_TEST_BOOL(v[1].IsA<WColor>());
    W_TEST_BOOL(v[2].IsA<WColorGammaUB>());
    W_TEST_BOOL(v[3].IsA<WTime>());
    W_TEST_BOOL(v[4].IsA<WVec2>());
    W_TEST_BOOL(v[5].IsA<WVec3>());
    W_TEST_BOOL(v[6].IsA<WVec4>());
    W_TEST_BOOL(v[7].IsA<WMat3>());
    W_TEST_BOOL(v[8].IsA<WMat4>());
    W_TEST_BOOL(v[9].IsA<WTransform>());
    W_TEST_BOOL(v[10].IsA<WQuat>());
    W_TEST_BOOL(v[11].IsA<WUuid>());
    W_TEST_BOOL(v[12].IsA<WAngle>());
    W_TEST_BOOL(v[13].IsA<WHashedString>());
    W_TEST_BOOL(v[14].IsA<WTempHashedString>());

    W_TEST_BOOL(v[1].Get<WColor>() == WColor(1, 0, 0.5));
    W_TEST_BOOL(v[2].Get<WColorGammaUB>() == WColorGammaUB(128, 0, 32, 64));
    W_TEST_FLOAT(v[3].Get<WTime>().GetSeconds(), 0.1, 0.0001f);
    W_TEST_VEC2(v[4].Get<WVec2>(), WVec2(0.1f, 2.0f), 0.0001f);
    W_TEST_VEC3(v[5].Get<WVec3>(), WVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    W_TEST_VEC4(v[6].Get<WVec4>(), WVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    W_TEST_BOOL(v[7].Get<WMat3>().IsEqual(WMat3::MakeFromValues(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    W_TEST_BOOL(v[8].Get<WMat4>().IsEqual(WMat4::MakeFromValues(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    W_TEST_BOOL(v[9].Get<WTransform>().m_qRotation == WQuat(4, 5, 6, 7));
    W_TEST_VEC3(v[9].Get<WTransform>().m_vPosition, WVec3(1, 2, 3), 0.0001f);
    W_TEST_VEC3(v[9].Get<WTransform>().m_vScale, WVec3(8, 9, 10), 0.0001f);
    W_TEST_BOOL(v[10].Get<WQuat>() == WQuat(0.1f, 2.0f, 3.2f, 44.5f));
    W_TEST_BOOL(v[11].Get<WUuid>() == WUuid(12345678910, 10987654321));
    W_TEST_FLOAT(v[12].Get<WAngle>().GetRadian(), 45.23f, 0.0001f);
    W_TEST_STRING(v[13].Get<WHashedString>().GetView(), "Soo much string");
    W_TEST_BOOL(v[14].Get<WTempHashedString>() == WTempHashedString("GHIJK"));


    /// \test Test primitive types in WVariant
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreColor")
  {
    StreamComparer sc("Color $v1{float{1,2,3,4}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreColor(js, WColor(1, 2, 3, 4), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreColorGamma")
  {
    StreamComparer sc("ColorGamma $v1{uint8{1,2,3,4}}\n");

    WOpenDdlWriter js;
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreColorGamma(js, WColorGammaUB(1, 2, 3, 4), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreTime")
  {
    StreamComparer sc("Time $v1{double{2.3}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreTime(js, WTime::MakeFromSeconds(2.3), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreVec2")
  {
    StreamComparer sc("Vec2 $v1{float{1,2}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreVec2(js, WVec2(1, 2), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreVec3")
  {
    StreamComparer sc("Vec3 $v1{float{1,2,3}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreVec3(js, WVec3(1, 2, 3), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreVec4")
  {
    StreamComparer sc("Vec4 $v1{float{1,2,3,4}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreVec4(js, WVec4(1, 2, 3, 4), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreMat3")
  {
    StreamComparer sc("Mat3 $v1{float{1,4,7,2,5,8,3,6,9}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreMat3(js, WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreMat4")
  {
    StreamComparer sc("Mat4 $v1{float{1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreMat4(js, WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), "v1", true);
  }

  // W_TEST_BLOCK(WTestBlock::Enabled, "StoreTransform")
  //{
  //  StreamComparer sc("Transform $v1{float{1,4,7,2,5,8,3,6,9,10}}\n");

  //  WOpenDdlWriter js;
  //  js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
  //  js.SetOutputStream(&sc);

  //  WOpenDdlUtils::StoreTransform(js, WTransform(WVec3(10, 20, 30), WMat3(1, 2, 3, 4, 5, 6, 7, 8, 9)), "v1", true);
  //}

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreQuat")
  {
    StreamComparer sc("Quat $v1{float{1,2,3,4}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreQuat(js, WQuat(1, 2, 3, 4), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreUuid")
  {
    StreamComparer sc("Uuid $v1{u4{12345678910,10987654321}}\n");

    WOpenDdlWriter js;
    js.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Shortest);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreUuid(js, WUuid(12345678910, 10987654321), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreAngle")
  {
    StreamComparer sc("Angle $v1{float{2.3}}\n");

    WOpenDdlWriter js;
    js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreAngle(js, WAngle::MakeFromRadian(2.3f), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreHashedString")
  {
    StreamComparer sc("HashedString $v1{string{\"ABCDE\"}}\n");

    WOpenDdlWriter js;
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreHashedString(js, WMakeHashedString("ABCDE"), "v1", true);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "StoreTempHashedString")
  {
    StreamComparer sc("TempHashedString $v1{uint64{2720389094277464445}}\n");

    WOpenDdlWriter js;
    js.SetOutputStream(&sc);

    WOpenDdlUtils::StoreTempHashedString(js, WTempHashedString("GHIJK"), "v1", true);
  }

  // this test also covers all the types that Variant supports
  W_TEST_BLOCK(WTestBlock::Enabled, "StoreVariant")
  {
    alignas(alignof(float)) WUInt8 rawData[sizeof(float) * 16]; // enough for mat4

    for (WUInt8 i = 0; i < W_ARRAY_SIZE(rawData); ++i)
    {
      rawData[i] = i + 33;
    }

    rawData[W_ARRAY_SIZE(rawData) - 1] = 0; // string terminator

    for (WUInt32 t = WVariant::Type::FirstStandardType + 1; t < WVariant::Type::LastStandardType; ++t)
    {
      const WVariant var = CreateVariant((WVariant::Type::Enum)t, rawData);

      WDefaultMemoryStreamStorage storage;
      WMemoryStreamWriter writer(&storage);
      WMemoryStreamReader reader(&storage);

      WOpenDdlWriter js;
      js.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Exact);
      js.SetOutputStream(&writer);

      WOpenDdlUtils::StoreVariant(js, var, "bla");

      WOpenDdlReader doc;
      W_TEST_BOOL(doc.ParseDocument(reader).Succeeded());

      const auto pVarElem = doc.GetRootElement()->FindChild("bla");

      WVariant result;
      WOpenDdlUtils::ConvertToVariant(pVarElem, result).IgnoreResult();

      W_TEST_BOOL(var == result);
    }
  }
}

static WVariant CreateVariant(WVariant::Type::Enum t, const void* pData)
{
  switch (t)
  {
    case WVariant::Type::Bool:
      return WVariant(*(WInt8*)pData != 0);
    case WVariant::Type::Int8:
      return WVariant(*((WInt8*)pData));
    case WVariant::Type::UInt8:
      return WVariant(*((WUInt8*)pData));
    case WVariant::Type::Int16:
      return WVariant(*((WInt16*)pData));
    case WVariant::Type::UInt16:
      return WVariant(*((WUInt16*)pData));
    case WVariant::Type::Int32:
      return WVariant(*((WInt32*)pData));
    case WVariant::Type::UInt32:
      return WVariant(*((WUInt32*)pData));
    case WVariant::Type::Int64:
      return WVariant(*((WInt64*)pData));
    case WVariant::Type::UInt64:
      return WVariant(*((WUInt64*)pData));
    case WVariant::Type::Float:
      return WVariant(*((float*)pData));
    case WVariant::Type::Double:
      return WVariant(*((double*)pData));
    case WVariant::Type::Color:
      return WVariant(*((WColor*)pData));
    case WVariant::Type::Vector2:
      return WVariant(*((WVec2*)pData));
    case WVariant::Type::Vector3:
      return WVariant(*((WVec3*)pData));
    case WVariant::Type::Vector4:
      return WVariant(*((WVec4*)pData));
    case WVariant::Type::Vector2I:
      return WVariant(*((WVec2I32*)pData));
    case WVariant::Type::Vector3I:
      return WVariant(*((WVec3I32*)pData));
    case WVariant::Type::Vector4I:
      return WVariant(*((WVec4I32*)pData));
    case WVariant::Type::Vector2U:
      return WVariant(*((WVec2U32*)pData));
    case WVariant::Type::Vector3U:
      return WVariant(*((WVec3U32*)pData));
    case WVariant::Type::Vector4U:
      return WVariant(*((WVec4U32*)pData));
    case WVariant::Type::Quaternion:
      return WVariant(*((WQuat*)pData));
    case WVariant::Type::Matrix3:
      return WVariant(*((WMat3*)pData));
    case WVariant::Type::Matrix4:
      return WVariant(*((WMat4*)pData));
    case WVariant::Type::Transform:
      return WVariant(*((WTransform*)pData));
    case WVariant::Type::String:
    case WVariant::Type::StringView: // string views are stored as full strings as well
      return WVariant((const char*)pData);
    case WVariant::Type::DataBuffer:
    {
      WDataBuffer db;
      db.SetCountUninitialized(sizeof(float) * 16);
      for (WUInt32 i = 0; i < db.GetCount(); ++i)
        db[i] = ((WUInt8*)pData)[i];

      return WVariant(db);
    }
    case WVariant::Type::Time:
      return WVariant(*((WTime*)pData));
    case WVariant::Type::Uuid:
      return WVariant(*((WUuid*)pData));
    case WVariant::Type::Angle:
      return WVariant(*((WAngle*)pData));
    case WVariant::Type::ColorGamma:
      return WVariant(*((WColorGammaUB*)pData));
    case WVariant::Type::HashedString:
    {
      WHashedString s;
      s.Assign((const char*)pData);
      return WVariant(s);
    }
    case WVariant::Type::TempHashedString:
      return WVariant(WTempHashedString((const char*)pData));

    default:
      W_REPORT_FAILURE("Unknown type");
  }

  return WVariant();
}
