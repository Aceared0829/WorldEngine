#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <FoundationTest/IO/JSONTestHelpers.h>


W_CREATE_SIMPLE_TEST(IO, StandardJSONWriter)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Object")
  {
    StreamComparer sc("\"TestObject\" : {\n\
  \n\
}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject("TestObject");
    js.EndObject();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Anonymous Object")
  {
    StreamComparer sc("{\n\
  \n\
}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.EndObject();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableBool")
  {
    StreamComparer sc("\"var1\" : true,\n\"var2\" : false");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableBool("var1", true);
    js.AddVariableBool("var2", false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt32("var1", 23);
    js.AddVariableInt32("var2", -42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableUInt32")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt32("var1", 23);
    js.AddVariableUInt32("var2", 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : -42");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableInt64("var1", 23);
    js.AddVariableInt64("var2", -42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableUInt64")
  {
    StreamComparer sc("\"var1\" : 23,\n\"var2\" : 42");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUInt64("var1", 23);
    js.AddVariableUInt64("var2", 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableFloat")
  {
    StreamComparer sc("\"var1\" : -65.5,\n\"var2\" : 2621.25");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableFloat("var1", -65.5f);
    js.AddVariableFloat("var2", 2621.25f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableDouble")
  {
    StreamComparer sc("\"var1\" : -65.125,\n\"var2\" : 2621.0625");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableDouble("var1", -65.125f);
    js.AddVariableDouble("var2", 2621.0625f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableString")
  {
    StreamComparer sc("\"var1\" : \"bla\",\n\"var2\" : \"blub\",\n\"special\" : \"I\\\\m\\t\\\"s\\bec/al\\\" \\f\\n//\\\\\\r\"");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableString("var1", "bla");
    js.AddVariableString("var2", "blub");

    js.AddVariableString("special", "I\\m\t\"s\bec/al\" \f\n//\\\r");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableNULL")
  {
    StreamComparer sc("\"var1\" : null");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableNULL("var1");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableTime")
  {
    StreamComparer sc("\"var1\" : 0.5,\n\"var2\" : 2.25");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableTime("var1", WTime::MakeFromSeconds(0.5));
    js.AddVariableTime("var2", WTime::MakeFromSeconds(2.25));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableUuid")
  {
    WUuid guid;
    WUInt64 val[2];
    val[0] = 0x1122334455667788;
    val[1] = 0x99AABBCCDDEEFF00;
    WMemoryUtils::Copy(reinterpret_cast<WUInt64*>(&guid), val, 2);

    StreamComparer sc("\"uuid_var\" : \"{ 55667788-3344-1122-00ff-eeddccbbaa99 }\"");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableUuid("uuid_var", guid);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableAngle")
  {
    StreamComparer sc("\"var1\" : 90,\n\"var2\" : 180");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    // vs2019 is so imprecise, that the degree->radian conversion introduces differences in the final output
    js.AddVariableAngle("var1", WAngle::MakeFromRadian(1.5707963267f));
    js.AddVariableAngle("var2", WAngle::MakeFromRadian(1.0f * WMath::Pi<float>()));
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableColor")
  {
    StreamComparer sc("\"var1\" : {\n  \"r\" : 1,\n  \"g\" : 2,\n  \"b\" : 3,\n  \"a\" : 4\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableColor("var1", WColor(1, 2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableColorGamma")
  {
    StreamComparer sc("\"var1\" : {\n  \"r\" : 1,\n  \"g\" : 2,\n  \"b\" : 3,\n  \"a\" : 4\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableColorGamma("var1", WColorGammaUB(1, 2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec2")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2("var1", WVec2(1, 2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec3")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2,\n  \"z\" : 3\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3("var1", WVec3(1, 2, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec4")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2,\n  \"z\" : 3,\n  \"w\" : 4\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4("var1", WVec4(1, 2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec2I32")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec2I32("var1", WVec2I32(1, 2));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec3I32")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2,\n  \"z\" : 3\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec3I32("var1", WVec3I32(1, 2, 3));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVec4I32")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2,\n  \"z\" : 3,\n  \"w\" : 4\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVec4I32("var1", WVec4I32(1, 2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableDataBuffer")
  {
    StreamComparer sc("\"var1\" : \"ff00da\"");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    WDataBuffer db;
    db.PushBack(0xFF);
    db.PushBack(0x00);
    db.PushBack(0xDA);
    js.AddVariableDataBuffer("var1", db);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableQuat")
  {
    StreamComparer sc("\"var1\" : {\n  \"x\" : 1,\n  \"y\" : 2,\n  \"z\" : 3,\n  \"w\" : 4\n}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableQuat("var1", WQuat(1, 2, 3, 4));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableMat3")
  {
    StreamComparer sc("\"var1\" : [ [ 1, 2, 3 ], [ 4, 5, 6 ], [ 7, 8, 9 ] ]");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat3("var1", WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableMat4")
  {
    StreamComparer sc("\"var1\" : [ [ 1, 2, 3, 4 ], [ 5, 6, 7, 8 ], [ 9, 10, 11, 12 ], [ 13, 14, 15, 16 ] ]");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableMat4("var1", WMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableVariant")
  {
    StreamComparer sc("\
\"var1\" : 23,\n\
\"var2\" : 42.5,\n\
\"var3\" : 21.25,\n\
\"var4\" : true,\n\
\"var5\" : \"pups\"");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableVariant("var1", WVariant(23));
    js.AddVariableVariant("var2", WVariant(42.5f));
    js.AddVariableVariant("var3", WVariant(21.25));
    js.AddVariableVariant("var4", WVariant(true));
    js.AddVariableVariant("var5", WVariant("pups"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Control character escaping, exact output")
  {
    // Checked as text rather than by parsing, because what matters is the exact escape sequence, and
    // because a string with an embedded zero does not survive being read back into an WString.
    StreamComparer sc("\
\"var1\" : \"a\\u0007b\",\n\
\"var2\" : \"a\\u0000b\",\n\
\"var3\" : \"\\\\\\u0001\"");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.AddVariableString("var1", "a\x07"
                                 "b");
    js.AddVariableString("var2", WStringView("a\0b", 3));

    // The backslash is escaped once, by the pass that runs before this one - the \uXXXX sequence must
    // not be fed through it again.
    js.AddVariableString("var3", "\\\x01");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "AddVariableRawJson")
  {
    StreamComparer sc("\
{\n\
  \"schema\" : {\"type\":\"object\"},\n\
  \"after\" : 23,\n\
  \"array\" : [ 1, [2,3], \"four\" ]\n\
}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();

    // spliced in unchanged: no quotes, no escaping, and the writer's own whitespace mode does not
    // reach inside it
    js.AddVariableRawJson("schema", "{\"type\":\"object\"}");

    // the raw value counts as a value, so the following member is separated by a comma as usual
    js.AddVariableInt32("after", 23);

    js.BeginArray("array");
    js.WriteInt32(1);
    js.WriteRawJson("[2,3]");
    js.WriteString("four");
    js.EndArray();

    js.EndObject();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Arrays")
  {
    StreamComparer sc("\
{\n\
  \"EmptyArray\" : [  ],\n\
  \"NamedArray\" : [ 13 ],\n\
  \"NamedArray2\" : [ 1337, -4996 ],\n\
  \"Nested\" : [ null, [ 1, 2, 3 ], [ 4, 5, 6 ], [  ], \"That was an empty array\" ]\n\
}");

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();
    js.BeginArray("EmptyArray");
    js.EndArray();

    js.BeginArray("NamedArray");
    js.WriteInt32(13);
    js.EndArray();

    js.BeginArray("NamedArray2");
    js.WriteInt32(1337);
    js.WriteInt32(-4996);
    js.EndArray();

    js.BeginVariable("Nested");
    js.BeginArray();
    js.WriteNULL();

    js.BeginArray();
    js.WriteInt32(1);
    js.WriteInt32(2);
    js.WriteInt32(3);
    js.EndArray();

    js.BeginArray();
    js.WriteInt32(4);
    js.WriteInt32(5);
    js.WriteInt32(6);
    js.EndArray();

    js.BeginArray();
    js.EndArray();

    js.WriteString("That was an empty array");
    js.EndArray();
    js.EndVariable();

    js.EndObject();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Complex Objects")
  {
    WStringUtf8 sExp(L"\
{\n\
  \"String\" : \"testvälue\",\n\
  \"double\" : 43.56,\n\
  \"float\" : 64.720001,\n\
  \"bööl\" : true,\n\
  \"int\" : 23,\n\
  \"myarray\" : [ 1, 2.2, 3.3, false, \"ende\" ],\n\
  \"object\" : {\n\
    \"variable in object\" : \"bla/*asdf*/ //tuff\",\n\
    \"Subobject\" : {\n\
      \"variable in subobject\" : \"bla\\\\\",\n\
      \"array in sub\" : [ {\n\
          \"obj var\" : 234\n\
        },\n\
        {\n\
          \"obj var 2\" : -235\n\
        }, true, 4, false ]\n\
    }\n\
  },\n\
  \"test\" : \"text\"\n\
}");

    StreamComparer sc(sExp.GetData());

    WStandardJSONWriter js;
    js.SetOutputStream(&sc);

    js.BeginObject();

    js.AddVariableString("String", WStringUtf8(L"testvälue").GetData()); // Unicode / Utf-8 test (in string)
    js.AddVariableDouble("double", 43.56);
    js.AddVariableFloat("float", 64.72f);
    js.AddVariableBool(WStringUtf8(L"bööl").GetData(), true);            // Unicode / Utf-8 test (identifier)
    js.AddVariableInt32("int", 23);

    js.BeginArray("myarray");
    js.WriteInt32(1);
    js.WriteFloat(2.2f);
    js.WriteDouble(3.3);
    js.WriteBool(false);
    js.WriteString("ende");
    js.EndArray();

    js.BeginObject("object");
    js.AddVariableString("variable in object", "bla/*asdf*/ //tuff"); // 'comment' in string
    js.BeginObject("Subobject");
    js.AddVariableString("variable in subobject", "bla\\");           // character to be escaped

    js.BeginArray("array in sub");
    js.BeginObject();
    js.AddVariableUInt64("obj var", 234);
    js.EndObject();
    js.BeginObject();
    js.AddVariableInt64("obj var 2", -235);
    js.EndObject();
    js.WriteBool(true);
    js.WriteInt32(4);
    js.WriteBool(false);
    js.EndArray();
    js.EndObject();
    js.EndObject();

    js.AddVariableString("test", "text");

    js.EndObject();
  }
}

W_CREATE_SIMPLE_TEST(IO, StandardJSONWriter_EarlyOut)
{
  // ~WStandardJSONWriter asserts that everything that was begun was also ended. These are the two
  // supported ways to stop writing part way through, which error paths need - without them the only
  // options are matching every Begin with an End on every path, or a dead process.
  //
  // Verified by parsing the result rather than by comparing text, because what matters is that the
  // output is valid JSON with the expected content, not how it is formatted.
  auto WriteAndParse = [](WDelegate<void(WStandardJSONWriter&)> build, WVariantDictionary& out_result) -> WResult
  {
    WDefaultMemoryStreamStorage storage;

    {
      WMemoryStreamWriter writer(&storage);
      WStandardJSONWriter js;
      js.SetOutputStream(&writer);
      build(js);
    } // the writer is destroyed here - that is what would assert without EndAll()/Abandon()

    WMemoryStreamReader reader(&storage);
    WJSONReader parser;
    W_SUCCEED_OR_RETURN(parser.Parse(reader));

    out_result = parser.GetTopLevelObject();
    return W_SUCCESS;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "EndAll closes nested objects")
  {
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        js.BeginObject();
        js.BeginObject("outer");
        js.BeginObject("inner");
        js.AddVariableInt32("var", 1);
        js.EndAll(); // three objects still open
      },
      result)
                   .Succeeded());

    WVariant outer;
    W_TEST_BOOL(result.TryGetValue("outer", outer));
    W_TEST_BOOL(outer.IsA<WVariantDictionary>());

    WVariant inner;
    W_TEST_BOOL(outer.Get<WVariantDictionary>().TryGetValue("inner", inner));
    W_TEST_BOOL(inner.IsA<WVariantDictionary>());

    WVariant var;
    W_TEST_BOOL(inner.Get<WVariantDictionary>().TryGetValue("var", var));
    W_TEST_INT(var.ConvertTo<WInt32>(), 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndAll closes an open array")
  {
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        js.BeginObject();
        js.BeginArray("arr");
        js.WriteInt32(1);
        js.WriteInt32(2);
        js.EndAll(); }, result)
                   .Succeeded());

    WVariant arr;
    W_TEST_BOOL(result.TryGetValue("arr", arr));
    W_TEST_BOOL(arr.IsA<WVariantArray>());
    W_TEST_INT(arr.Get<WVariantArray>().GetCount(), 2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndAll writes null for a variable that has no value")
  {
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        js.BeginObject();
        js.BeginVariable("var"); // nothing written for it
        js.EndAll(); }, result)
                   .Succeeded());

    // The member has to exist, because an object member without any value is not representable.
    W_TEST_BOOL(result.Contains("var"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "EndAll on a balanced stream changes nothing")
  {
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        js.BeginObject();
        js.AddVariableInt32("var", 42);
        js.EndObject();
        js.EndAll(); }, result)
                   .Succeeded());

    WVariant var;
    W_TEST_BOOL(result.TryGetValue("var", var));
    W_TEST_INT(var.ConvertTo<WInt32>(), 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Abandon suppresses the destructor check")
  {
    // The output is deliberately left incomplete and must not be used. The point is that destroying
    // the writer with containers still open does not assert.
    WDefaultMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);

    WStandardJSONWriter js;
    js.SetOutputStream(&writer);

    js.BeginObject();
    js.BeginArray("arr");
    js.WriteInt32(1);

    js.Abandon();

    W_TEST_BOOL(js.HadWriteError());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Control characters are escaped as \\uXXXX")
  {
    // A raw control character in a JSON string is invalid and parsers reject the whole document. Log
    // text and user-entered strings contain them, so the writer has to escape them rather than hope.
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        js.BeginObject();
        js.AddVariableString("bell", "a\x07"
                                     "b");

        // The ones with a named escape keep it, and a backslash the caller wrote stays a single
        // backslash rather than being escaped twice by the \uXXXX pass.
        js.AddVariableString("named", "tab\there\nnewline");
        js.AddVariableString("backslash", "a\\b\x01");
        js.EndObject(); }, result)
                   .Succeeded());

    WVariant value;

    W_TEST_BOOL(result.TryGetValue("bell", value));
    W_TEST_STRING(value.ConvertTo<WString>(), "a\x07"
                                                "b");

    W_TEST_BOOL(result.TryGetValue("named", value));
    W_TEST_STRING(value.ConvertTo<WString>(), "tab\there\nnewline");

    W_TEST_BOOL(result.TryGetValue("backslash", value));
    W_TEST_STRING(value.ConvertTo<WString>(), "a\\b\x01");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WriteVariant covers standard types")
  {
    WVariantDictionary result;
    W_TEST_BOOL(WriteAndParse([](WStandardJSONWriter& js)
      {
        WHashedString sHashed;
        sHashed.Assign("text");

        js.BeginObject();
        js.AddVariableVariant("hashed", WVariant(sHashed));
        js.AddVariableVariant("tempHashed", WVariant(WTempHashedString("text")));
        js.AddVariableVariant("vec2u", WVariant(WVec2U32(1, 2)));
        js.AddVariableVariant("vec3u", WVariant(WVec3U32(1, 2, 3)));
        js.AddVariableVariant("vec4u", WVariant(WVec4U32(1, 2, 3, 4)));
        js.AddVariableVariant("transform", WVariant(WTransform(WVec3(1, 2, 3))));
        js.EndObject(); }, result)
                   .Succeeded());

    WVariant value;

    W_TEST_BOOL(result.TryGetValue("hashed", value));
    W_TEST_STRING(value.ConvertTo<WString>(), "text");

    // Only the hash survives - an WTempHashedString does not keep the text it was built from. Compared
    // as a double, because that is what the value came back through: WJSONReader parses every number
    // into a double, so a 64 bit hash does not survive the round trip exactly. The writer is not what
    // loses it, and writing it as a string instead would make it indistinguishable from a real one.
    W_TEST_BOOL(result.TryGetValue("tempHashed", value));
    W_TEST_DOUBLE(value.ConvertTo<double>(), static_cast<double>(WTempHashedString("text").GetHash()), 4096.0);

    W_TEST_BOOL(result.TryGetValue("vec3u", value));
    W_TEST_BOOL(value.IsA<WVariantDictionary>());
    W_TEST_INT(value.Get<WVariantDictionary>().GetValue("z")->ConvertTo<WUInt32>(), 3);

    W_TEST_BOOL(result.TryGetValue("vec4u", value));
    W_TEST_BOOL(value.IsA<WVariantDictionary>());
    W_TEST_INT(value.Get<WVariantDictionary>().GetValue("w")->ConvertTo<WUInt32>(), 4);

    W_TEST_BOOL(result.TryGetValue("vec2u", value));
    W_TEST_BOOL(value.IsA<WVariantDictionary>());
    W_TEST_INT(value.Get<WVariantDictionary>().GetValue("y")->ConvertTo<WUInt32>(), 2);

    W_TEST_BOOL(result.TryGetValue("transform", value));
    W_TEST_BOOL(value.IsA<WVariantDictionary>());

    const WVariantDictionary& transform = value.Get<WVariantDictionary>();
    W_TEST_BOOL(transform.Contains("position"));
    W_TEST_BOOL(transform.Contains("rotation"));
    W_TEST_BOOL(transform.Contains("scale"));
    W_TEST_INT(transform.GetValue("position")->Get<WVariantDictionary>().GetValue("x")->ConvertTo<WInt32>(), 1);
  }
}
