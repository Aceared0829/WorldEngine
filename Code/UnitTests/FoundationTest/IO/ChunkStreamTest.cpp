#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>

W_CREATE_SIMPLE_TEST(IO, ChunkStream)
{
  WDefaultMemoryStreamStorage StreamStorage;

  WMemoryStreamWriter MemoryWriter(&StreamStorage);
  WMemoryStreamReader MemoryReader(&StreamStorage);

  W_TEST_BLOCK(WTestBlock::Enabled, "Write Format")
  {
    WChunkStreamWriter writer(MemoryWriter);

    writer.BeginStream(1);

    {
      writer.BeginChunk("Chunk1", 1);

      writer << (WUInt32)4;
      writer << (float)5.6f;
      writer << (double)7.8;
      writer << "nine";
      writer << WVec3(10, 11.2f, 13.4f);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk2", 2);

      writer << "chunk 2 content";

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk3", 3);

      writer << "chunk 3 content";

      writer.EndChunk();
    }

    writer.EndStream();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Read Format")
  {
    WChunkStreamReader reader(MemoryReader);

    reader.BeginStream();

    // Chunk 1
    {
      W_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      W_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 36);

      WUInt32 i;
      float f;
      double d;
      WString s;

      reader >> i;
      reader >> f;
      reader >> d;
      reader >> s;

      W_TEST_INT(i, 4);
      W_TEST_FLOAT(f, 5.6f, 0);
      W_TEST_DOUBLE(d, 7.8, 0);
      W_TEST_STRING(s.GetData(), "nine");

      W_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 12);
      reader.NextChunk();
    }

    // Chunk 2
    {
      W_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      W_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk2");
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 2);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      WString s;

      reader >> s;

      W_TEST_STRING(s.GetData(), "chunk 2 content");

      W_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    // Chunk 3
    {
      W_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      W_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk3");
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 3);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      W_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      WString s;

      reader >> s;

      W_TEST_STRING(s.GetData(), "chunk 3 content");

      W_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    W_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    reader.SetEndChunkFileMode(WChunkStreamReader::EndChunkFileMode::SkipToEnd);
    reader.EndStream();

    WUInt8 Temp[1024];
    W_TEST_INT(MemoryReader.ReadBytes(Temp, 1024), 0); // nothing left to read
  }
}
