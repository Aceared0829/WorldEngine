#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Tracks/EventTrack.h>

W_CREATE_SIMPLE_TEST_GROUP(Tracks);

W_CREATE_SIMPLE_TEST(Tracks, EventTrack)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Empty")
  {
    WEventTrack et;
    WTempHybridArray<WHashedString, 8> result;

    W_TEST_BOOL(et.IsEmpty());
    et.Sample(WTime::MakeZero(), WTime::MakeFromSeconds(1.0), result);

    W_TEST_BOOL(result.IsEmpty());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sample")
  {
    WEventTrack et;
    WTempHybridArray<WHashedString, 8> result;

    et.AddControlPoint(WTime::MakeFromSeconds(3.0), "Event3");
    et.AddControlPoint(WTime::MakeFromSeconds(0.0), "Event0");
    et.AddControlPoint(WTime::MakeFromSeconds(4.0), "Event4");
    et.AddControlPoint(WTime::MakeFromSeconds(1.0), "Event1");
    et.AddControlPoint(WTime::MakeFromSeconds(2.0), "Event2");

    W_TEST_BOOL(!et.IsEmpty());

    // sampling an empty range should yield no results, even if sampling an exact time where an event is
    result.Clear();
    {
      {
        et.Sample(WTime::MakeFromSeconds(0.0), WTime::MakeFromSeconds(0.0), result);
        W_TEST_INT(result.GetCount(), 0);
      }

      {
        result.Clear();
        et.Sample(WTime::MakeFromSeconds(1.0), WTime::MakeFromSeconds(1.0), result);
        W_TEST_INT(result.GetCount(), 0);
      }

      {
        result.Clear();
        et.Sample(WTime::MakeFromSeconds(4.0), WTime::MakeFromSeconds(4.0), result);
        W_TEST_INT(result.GetCount(), 0);
      }
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(0.0), WTime::MakeFromSeconds(1.0), result);
      W_TEST_INT(result.GetCount(), 1);
      W_TEST_STRING(result[0].GetString(), "Event0");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(0.0), WTime::MakeFromSeconds(2.0), result);
      W_TEST_INT(result.GetCount(), 2);
      W_TEST_STRING(result[0].GetString(), "Event0");
      W_TEST_STRING(result[1].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(0.0), WTime::MakeFromSeconds(4.0), result);
      W_TEST_INT(result.GetCount(), 4);
      W_TEST_STRING(result[0].GetString(), "Event0");
      W_TEST_STRING(result[1].GetString(), "Event1");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event3");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(0.0), WTime::MakeFromSeconds(10.0), result);
      W_TEST_INT(result.GetCount(), 5);
      W_TEST_STRING(result[0].GetString(), "Event0");
      W_TEST_STRING(result[1].GetString(), "Event1");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event3");
      W_TEST_STRING(result[4].GetString(), "Event4");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(-0.1), WTime::MakeFromSeconds(10.0), result);
      W_TEST_INT(result.GetCount(), 5);
      W_TEST_STRING(result[0].GetString(), "Event0");
      W_TEST_STRING(result[1].GetString(), "Event1");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event3");
      W_TEST_STRING(result[4].GetString(), "Event4");
    }

    et.Clear();
    W_TEST_BOOL(et.IsEmpty());
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Reverse Sample")
  {
    WEventTrack et;
    WTempHybridArray<WHashedString, 8> result;

    et.AddControlPoint(WTime::MakeFromSeconds(3.0), "Event3");
    et.AddControlPoint(WTime::MakeFromSeconds(0.0), "Event0");
    et.AddControlPoint(WTime::MakeFromSeconds(4.0), "Event4");
    et.AddControlPoint(WTime::MakeFromSeconds(1.0), "Event1");
    et.AddControlPoint(WTime::MakeFromSeconds(2.0), "Event2");

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(2.0), WTime::MakeFromSeconds(0.0), result);
      W_TEST_INT(result.GetCount(), 2);
      W_TEST_STRING(result[0].GetString(), "Event2");
      W_TEST_STRING(result[1].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(4.0), WTime::MakeFromSeconds(0.0), result);
      W_TEST_INT(result.GetCount(), 4);
      W_TEST_STRING(result[0].GetString(), "Event4");
      W_TEST_STRING(result[1].GetString(), "Event3");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(10.0), WTime::MakeFromSeconds(0.0), result);
      W_TEST_INT(result.GetCount(), 4);
      W_TEST_STRING(result[0].GetString(), "Event4");
      W_TEST_STRING(result[1].GetString(), "Event3");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event1");
    }

    {
      result.Clear();
      et.Sample(WTime::MakeFromSeconds(10.0), WTime::MakeFromSeconds(-0.1), result);
      W_TEST_INT(result.GetCount(), 5);
      W_TEST_STRING(result[0].GetString(), "Event4");
      W_TEST_STRING(result[1].GetString(), "Event3");
      W_TEST_STRING(result[2].GetString(), "Event2");
      W_TEST_STRING(result[3].GetString(), "Event1");
      W_TEST_STRING(result[4].GetString(), "Event0");
    }
  }
}
