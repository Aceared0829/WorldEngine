#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/Progress.h>

W_CREATE_SIMPLE_TEST(Utility, Progress)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Simple progress")
  {
    WProgress progress;
    {
      WProgressRange progressRange = WProgressRange("TestProgress", 4, false, &progress);

      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(progress.GetMainDisplayText() == "TestProgress");

      progressRange.BeginNextStep("Step1");
      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(progress.GetStepDisplayText() == "Step1");

      progressRange.BeginNextStep("Step2");
      W_TEST_FLOAT(progress.GetCompletion(), 0.25f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(progress.GetStepDisplayText() == "Step2");

      progressRange.BeginNextStep("Step3");
      W_TEST_FLOAT(progress.GetCompletion(), 0.5f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(progress.GetStepDisplayText() == "Step3");

      progressRange.BeginNextStep("Step4");
      W_TEST_FLOAT(progress.GetCompletion(), 0.75f, WMath::DefaultEpsilon<float>());
      W_TEST_BOOL(progress.GetStepDisplayText() == "Step4");
    }

    W_TEST_FLOAT(progress.GetCompletion(), 1.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Weighted progress")
  {
    WProgress progress;
    {
      WProgressRange progressRange = WProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0+1", 2);
      W_TEST_FLOAT(progress.GetCompletion(), 0.2f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2"); // this step should have twice the weight as the other steps.
      W_TEST_FLOAT(progress.GetCompletion(), 0.4f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      W_TEST_FLOAT(progress.GetCompletion(), 0.8f, WMath::DefaultEpsilon<float>());
    }

    W_TEST_FLOAT(progress.GetCompletion(), 1.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Nested progress")
  {
    WProgress progress;
    {
      WProgressRange progressRange = WProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      W_TEST_FLOAT(progress.GetCompletion(), 0.2f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      W_TEST_FLOAT(progress.GetCompletion(), 0.4f, WMath::DefaultEpsilon<float>());

      {
        WProgressRange nestedRange = WProgressRange("Nested", 5, false, &progress);
        nestedRange.SetStepWeighting(1, 4.0f);

        W_TEST_FLOAT(progress.GetCompletion(), 0.4f, WMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep0");
        W_TEST_FLOAT(progress.GetCompletion(), 0.4f, WMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep1");
        W_TEST_FLOAT(progress.GetCompletion(), 0.45f, WMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep2");
        W_TEST_FLOAT(progress.GetCompletion(), 0.65f, WMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep3");
        W_TEST_FLOAT(progress.GetCompletion(), 0.7f, WMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep4");
        W_TEST_FLOAT(progress.GetCompletion(), 0.75f, WMath::DefaultEpsilon<float>());
      }
      W_TEST_FLOAT(progress.GetCompletion(), 0.8f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      W_TEST_FLOAT(progress.GetCompletion(), 0.8f, WMath::DefaultEpsilon<float>());
    }

    W_TEST_FLOAT(progress.GetCompletion(), 1.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Nested progress with manual completion")
  {
    WProgress progress;
    {
      WProgressRange progressRange = WProgressRange("TestProgress", 3, false, &progress);
      progressRange.SetStepWeighting(1, 2.0f);

      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      W_TEST_FLOAT(progress.GetCompletion(), 0.0f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      W_TEST_FLOAT(progress.GetCompletion(), 0.25f, WMath::DefaultEpsilon<float>());

      {
        WProgressRange nestedRange = WProgressRange("Nested", false, &progress);

        W_TEST_FLOAT(progress.GetCompletion(), 0.25f, WMath::DefaultEpsilon<float>());

        nestedRange.SetCompletion(0.5);
        W_TEST_FLOAT(progress.GetCompletion(), 0.5f, WMath::DefaultEpsilon<float>());
      }
      W_TEST_FLOAT(progress.GetCompletion(), 0.75f, WMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      W_TEST_FLOAT(progress.GetCompletion(), 0.75f, WMath::DefaultEpsilon<float>());
    }

    W_TEST_FLOAT(progress.GetCompletion(), 1.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Progress Events")
  {
    WUInt32 uiNumProgressUpdatedEvents = 0;

    WProgress progress;
    progress.m_Events.AddEventHandler([&](const WProgressEvent& e)
      {
      if (e.m_Type == WProgressEvent::Type::ProgressChanged)
      {
        ++uiNumProgressUpdatedEvents;
        W_TEST_FLOAT(e.m_pProgressbar->GetCompletion(), uiNumProgressUpdatedEvents * 0.25f, WMath::DefaultEpsilon<float>());
      } });

    {
      WProgressRange progressRange = WProgressRange("TestProgress", 4, false, &progress);

      progressRange.BeginNextStep("Step1");
      progressRange.BeginNextStep("Step2");
      progressRange.BeginNextStep("Step3");
      progressRange.BeginNextStep("Step4");
    }

    W_TEST_FLOAT(progress.GetCompletion(), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_INT(uiNumProgressUpdatedEvents, 4);
  }
}
