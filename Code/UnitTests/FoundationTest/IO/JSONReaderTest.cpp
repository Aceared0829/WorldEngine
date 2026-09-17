#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/JSONReader.h>

namespace JSONReaderTestDetail
{

  class StringStream : public WStreamReader
  {
  public:
    StringStream(const void* pData)
    {
      m_pData = pData;
      m_uiLength = WStringUtils::GetStringElementCount((const char*)pData);
    }

    virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead)
    {
      uiBytesToRead = WMath::Min(uiBytesToRead, m_uiLength);
      m_uiLength -= uiBytesToRead;

      if (uiBytesToRead > 0)
      {
        WMemoryUtils::Copy((WUInt8*)pReadBuffer, (WUInt8*)m_pData, (size_t)uiBytesToRead);
        m_pData = WMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
      }

      return uiBytesToRead;
    }

  private:
    const void* m_pData;
    WUInt64 m_uiLength;
  };

  void TraverseTree(const WVariant& var, WDeque<WString>& ref_compare)
  {
    if (ref_compare.IsEmpty())
      return;

    switch (var.GetType())
    {
      case WVariant::Type::VariantDictionary:
      {
        // WLog::Printf("Expect: %s - Is: %s\n", "<object>", Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), "<object>");
        ref_compare.PopFront();

        const WVariantDictionary& vd = var.Get<WVariantDictionary>();

        for (auto it = vd.GetIterator(); it.IsValid(); ++it)
        {
          if (ref_compare.IsEmpty())
            return;

          // WLog::Printf("Expect: %s - Is: %s\n", it.Key().GetData(), Compare.PeekFront().GetData());
          W_TEST_STRING(ref_compare.PeekFront().GetData(), it.Key().GetData());
          ref_compare.PopFront();

          TraverseTree(it.Value(), ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        W_TEST_STRING(ref_compare.PeekFront().GetData(), "</object>");
        // WLog::Printf("Expect: %s - Is: %s\n", "</object>", Compare.PeekFront().GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::VariantArray:
      {
        // WLog::Printf("Expect: %s - Is: %s\n", "<array>", Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), "<array>");
        ref_compare.PopFront();

        const WVariantArray& va = var.Get<WVariantArray>();

        for (WUInt32 i = 0; i < va.GetCount(); ++i)
        {
          TraverseTree(va[i], ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        // WLog::Printf("Expect: %s - Is: %s\n", "</array>", Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), "</array>");
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Invalid:
        // WLog::Printf("Expect: %s - Is: %s\n", "null", Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), "null");
        ref_compare.PopFront();
        break;

      case WVariant::Type::Bool:
        // WLog::Printf("Expect: %s - Is: %s\n", var.Get<bool>() ? "bool true" : "bool false", Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<bool>() ? "bool true" : "bool false");
        ref_compare.PopFront();
        break;

      case WVariant::Type::Int8:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("int8 {0}", var.Get<WInt8>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::UInt8:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("uint8 {0}", var.Get<WUInt8>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Int16:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("int16 {0}", var.Get<WInt16>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::UInt16:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("uint16 {0}", var.Get<WUInt16>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Int32:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("int32 {0}", var.Get<WInt32>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::UInt32:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("uint32 {0}", var.Get<WUInt32>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Int64:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("int64 {0}", var.Get<WInt64>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::UInt64:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("uint64 {0}", var.Get<WUInt64>());
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Float:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("float {0}", WArgF(var.Get<float>(), 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Double:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("double {0}", WArgF(var.Get<double>(), 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Time:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("time {0}", WArgF(var.Get<WTime>().GetSeconds(), 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Angle:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("angle {0}", WArgF(var.Get<WAngle>().GetDegree(), 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::String:
        // WLog::Printf("Expect: %s - Is: %s\n", var.Get<WString>().GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<WString>().GetData());
        ref_compare.PopFront();
        break;

      case WVariant::Type::StringView:
        // WLog::Printf("Expect: %s - Is: %s\n", var.Get<WString>().GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront(), var.Get<WStringView>());
        ref_compare.PopFront();
        break;

      case WVariant::Type::Vector2:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec2 ({0}, {1})", WArgF(var.Get<WVec2>().x, 4), WArgF(var.Get<WVec2>().y, 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Vector3:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec3 ({0}, {1}, {2})", WArgF(var.Get<WVec3>().x, 4), WArgF(var.Get<WVec3>().y, 4), WArgF(var.Get<WVec3>().z, 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Vector4:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec4 ({0}, {1}, {2}, {3})", WArgF(var.Get<WVec4>().x, 4), WArgF(var.Get<WVec4>().y, 4), WArgF(var.Get<WVec4>().z, 4), WArgF(var.Get<WVec4>().w, 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Vector2I:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec2i ({0}, {1})", var.Get<WVec2I32>().x, var.Get<WVec2I32>().y);
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Vector3I:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec3i ({0}, {1}, {2})", var.Get<WVec3I32>().x, var.Get<WVec3I32>().y, var.Get<WVec3I32>().z);
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Vector4I:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("vec4i ({0}, {1}, {2}, {3})", var.Get<WVec4I32>().x, var.Get<WVec4I32>().y, var.Get<WVec4I32>().z, var.Get<WVec4I32>().w);
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Color:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("color ({0}, {1}, {2}, {3})", WArgF(var.Get<WColor>().r, 4), WArgF(var.Get<WColor>().g, 4), WArgF(var.Get<WColor>().b, 4), WArgF(var.Get<WColor>().a, 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::ColorGamma:
      {
        WStringBuilder sTemp;
        const WColorGammaUB c = var.ConvertTo<WColorGammaUB>();

        sTemp.SetFormat("gamma ({0}, {1}, {2}, {3})", c.r, c.g, c.b, c.a);
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Quaternion:
      {
        WStringBuilder sTemp;
        sTemp.SetFormat("quat ({0}, {1}, {2}, {3})", WArgF(var.Get<WQuat>().x, 4), WArgF(var.Get<WQuat>().y, 4), WArgF(var.Get<WQuat>().z, 4), WArgF(var.Get<WQuat>().w, 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Matrix3:
      {
        WMat3 m = var.Get<WMat3>();

        WStringBuilder sTemp;
        sTemp.SetFormat("mat3 ({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8})", WArgF(m.m_fElementsCM[0], 4), WArgF(m.m_fElementsCM[1], 4), WArgF(m.m_fElementsCM[2], 4), WArgF(m.m_fElementsCM[3], 4), WArgF(m.m_fElementsCM[4], 4), WArgF(m.m_fElementsCM[5], 4), WArgF(m.m_fElementsCM[6], 4), WArgF(m.m_fElementsCM[7], 4), WArgF(m.m_fElementsCM[8], 4));
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Matrix4:
      {
        WMat4 m = var.Get<WMat4>();

        WStringBuilder sTemp;
        sTemp.SetPrintf("mat4 (%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)", m.m_fElementsCM[0], m.m_fElementsCM[1], m.m_fElementsCM[2], m.m_fElementsCM[3], m.m_fElementsCM[4], m.m_fElementsCM[5], m.m_fElementsCM[6], m.m_fElementsCM[7], m.m_fElementsCM[8], m.m_fElementsCM[9], m.m_fElementsCM[10], m.m_fElementsCM[11], m.m_fElementsCM[12], m.m_fElementsCM[13], m.m_fElementsCM[14], m.m_fElementsCM[15]);
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case WVariant::Type::Uuid:
      {
        WUuid uuid = var.Get<WUuid>();
        WStringBuilder sTemp;
        WConversionUtils::ToString(uuid, sTemp);
        sTemp.Prepend("uuid ");
        // WLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        W_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
} // namespace JSONReaderTestDetail

W_CREATE_SIMPLE_TEST(IO, JSONReader)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Test")
  {
    WStringUtf8 sTD(L"{\n\
\"myarray2\":[\"\",2.2],\n\
\"myarray\" : [1, 2.2, 3.3, false, \"ende\" ],\n\
\"String\"/**/ : \"testvälue\",\n\
\"double\"/***/ : 43.56,//comment\n\
\"float\" :/**//*a*/ 64/*comment*/.720001,\n\
\"bool\" : tr/*asdf*/ue,\n\
\"int\" : 23,\n\
\"MyNüll\" : nu/*asdf*/ll,\n\
\"object\" :\n\
/* totally \n weird \t stuff \n\n\n going on here // thats a line comment \n */ \
// more line comments \n\n\n\n\
{\n\
  \"variable in object\" : \"bla\\\\\\\"\\/\",\n\
    \"Subobject\" :\n\
  {\n\
    \"variable in subobject\" : \"blub\\r\\f\\n\\b\\t\",\n\
      \"array in sub\" : [\n\
    {\n\
      \"obj var\" : 234\n\
            /*stuff ] */ \
    },\n\
    {\n\
      \"obj var 2\" : -235\n//breakingcomment\n\
    }, true, 4, false ]\n\
  }\n\
},\n\
\"test\" : \"text\"\n\
}");
    const char* szTestData = sTD.GetData();

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // WVariantDictionary is an WHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    JSONReaderTestDetail::StringStream stream(szTestData);

    WJSONReader reader;
    W_TEST_BOOL(reader.Parse(stream).Succeeded());

    WDeque<WString> sCompare;
    sCompare.PushBack("<object>");
    sCompare.PushBack("int");
    sCompare.PushBack("double 23.0000");
    sCompare.PushBack("String");
    sCompare.PushBack(WStringUtf8(L"testvälue").GetData()); // unicode literal

    sCompare.PushBack("double");
    sCompare.PushBack("double 43.5600");

    sCompare.PushBack("myarray");
    sCompare.PushBack("<array>");
    sCompare.PushBack("double 1.0000");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("double 3.3000");
    sCompare.PushBack("bool false");
    sCompare.PushBack("ende");
    sCompare.PushBack("</array>");

    sCompare.PushBack("object");
    sCompare.PushBack("<object>");

    sCompare.PushBack("Subobject");
    sCompare.PushBack("<object>");

    sCompare.PushBack("array in sub");
    sCompare.PushBack("<array>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var");
    sCompare.PushBack("double 234.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var 2");
    sCompare.PushBack("double -235.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("bool true");
    sCompare.PushBack("double 4.0000");
    sCompare.PushBack("bool false");

    sCompare.PushBack("</array>");


    sCompare.PushBack("variable in subobject");
    sCompare.PushBack("blub\r\f\n\b\t"); // escaped special characters

    sCompare.PushBack("</object>");

    sCompare.PushBack("variable in object");
    sCompare.PushBack("bla\\\"/"); // escaped backslash, quotation mark, slash

    sCompare.PushBack("</object>");

    sCompare.PushBack("float");
    sCompare.PushBack("double 64.7200");

    sCompare.PushBack("myarray2");
    sCompare.PushBack("<array>");
    sCompare.PushBack("");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("</array>");

    sCompare.PushBack(WStringUtf8(L"MyNüll").GetData()); // unicode literal
    sCompare.PushBack("null");

    sCompare.PushBack("test");
    sCompare.PushBack("text");

    sCompare.PushBack("bool");
    sCompare.PushBack("bool true");

    sCompare.PushBack("</object>");

    if (W_TEST_BOOL(reader.GetTopLevelElementType() == WJSONReader::ElementType::Dictionary))
    {
      JSONReaderTestDetail::TraverseTree(reader.GetTopLevelObject(), sCompare);

      W_TEST_BOOL(sCompare.IsEmpty());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Array document")
  {
    const char* szTestData = "[\"a\",\"b\"]";

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // WVariantDictionary is an WHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    JSONReaderTestDetail::StringStream stream(szTestData);

    WJSONReader reader;
    W_TEST_BOOL(reader.Parse(stream).Succeeded());

    WDeque<WString> sCompare;
    sCompare.PushBack("<array>");
    sCompare.PushBack("a");
    sCompare.PushBack("b");
    sCompare.PushBack("</array>");

    if (W_TEST_BOOL(reader.GetTopLevelElementType() == WJSONReader::ElementType::Array))
    {
      JSONReaderTestDetail::TraverseTree(reader.GetTopLevelArray(), sCompare);

      W_TEST_BOOL(sCompare.IsEmpty());
    }
  }
}
