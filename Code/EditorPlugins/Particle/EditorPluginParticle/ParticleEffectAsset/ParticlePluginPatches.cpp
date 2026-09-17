#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WParticleBehaviorFactory_SizeCurvePatch_1_2 : public WGraphPatch
{
public:
  WParticleBehaviorFactory_SizeCurvePatch_1_2()
    : WGraphPatch("WParticleBehaviorFactory_SizeCurve", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("SizeCurve", "SharedSizeCurve");
    pNode->RenameProperty("BaseSize", "SizeCurveOffset");
    pNode->RenameProperty("CurveScale", "SizeCurveScale");

    // Set the curve source to "Shared" for backward compatibility
    // In older versions, there was only the shared curve option
    pNode->AddProperty("ChangeSizeWith", (WInt32)1);
  }
};

WParticleBehaviorFactory_SizeCurvePatch_1_2 g_WParticleBehaviorFactory_SizeCurvePatch_1_2;

//////////////////////////////////////////////////////////////////////////

/// Migrates wind influence and rise speed from Velocity behavior to new Wind and Move behaviors
class WParticleBehaviorFactory_Velocity_1_2 : public WGraphPatch
{
public:
  WParticleBehaviorFactory_Velocity_1_2()
    : WGraphPatch("WParticleBehaviorFactory_Velocity", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Read the wind influence value from the old Velocity behavior
    auto* pWindInfluence = pNode->FindProperty("WindInfluence");
    const float fWindInfluence = (pWindInfluence && pWindInfluence->m_Value.IsFloatingPoint()) ? pWindInfluence->m_Value.ConvertTo<float>() : 0.0f;

    // Read the rise speed value from the old Velocity behavior
    auto* pRiseSpeed = pNode->FindProperty("RiseSpeed");
    const float fRiseSpeed = (pRiseSpeed && pRiseSpeed->m_Value.IsFloatingPoint()) ? pRiseSpeed->m_Value.ConvertTo<float>() : 0.0f;

    // Only create new behaviors if we have something to migrate
    if (fWindInfluence <= 0.0f && fRiseSpeed == 0.0f)
      return;

    // Find the parent system descriptor node
    WAbstractObjectNode* pSystemNode = nullptr;
    for (auto it = pGraph->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      WAbstractObjectNode* pCandidate = it.Value();
      if (pCandidate->GetType() == "WParticleSystemDescriptor")
      {
        // Check if this system contains our velocity behavior
        auto* pBehaviors = pCandidate->FindProperty("Behaviors");
        if (pBehaviors && pBehaviors->m_Value.IsA<WVariantArray>())
        {
          const auto& behaviors = pBehaviors->m_Value.Get<WVariantArray>();
          for (const auto& behaviorVar : behaviors)
          {
            if (behaviorVar.IsA<WUuid>() && behaviorVar.Get<WUuid>() == pNode->GetGuid())
            {
              pSystemNode = pCandidate;
              break;
            }
          }
        }
        if (pSystemNode)
          break;
      }
    }

    if (pSystemNode == nullptr)
      return;

    // Get the behaviors array to add new behaviors to
    auto* pBehaviors = pSystemNode->FindProperty("Behaviors");
    if (!pBehaviors || !pBehaviors->m_Value.IsA<WVariantArray>())
      return;

    WVariantArray behaviors = pBehaviors->m_Value.Get<WVariantArray>();

    // Create a new Wind behavior node if wind influence is greater than 0
    if (fWindInfluence > 0.0f)
    {
      WUuid windBehaviorGuid = WUuid::MakeUuid();
      WAbstractObjectNode* pWindNode = pGraph->AddNode(windBehaviorGuid, "WParticleBehaviorFactory_Wind", 1);
      pWindNode->AddProperty("WindInfluence", fWindInfluence);
      behaviors.PushBack(windBehaviorGuid);
    }

    // Create a new Move behavior node if rise speed is non-zero
    if (fRiseSpeed != 0.0f)
    {
      WUuid moveBehaviorGuid = WUuid::MakeUuid();
      WAbstractObjectNode* pMoveNode = pGraph->AddNode(moveBehaviorGuid, "WParticleBehaviorFactory_Move", 1);

      // Set Z-axis movement to constant mode with the rise speed value
      pMoveNode->AddProperty("MoveZ_Mode", (WInt32)0); // WMovementMode::Constant = 0
      pMoveNode->AddProperty("MoveZ_Speed", fRiseSpeed);

      behaviors.PushBack(moveBehaviorGuid);
    }

    // Update the behaviors array with the new behaviors
    pBehaviors->m_Value = behaviors;
  }
};

WParticleBehaviorFactory_Velocity_1_2 g_WParticleBehaviorFactory_Velocity_1_2;

//////////////////////////////////////////////////////////////////////////

/// Migrates ColorGradient behavior from old hGradient to new GradientSource system
class WParticleBehaviorFactory_ColorGradient_2_3 : public WGraphPatch
{
public:
  WParticleBehaviorFactory_ColorGradient_2_3()
    : WGraphPatch("WParticleBehaviorFactory_ColorGradient", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Check if there's an old hGradient property (from version 2)
    auto* pGradientHandle = pNode->FindProperty("Gradient");
    if (pGradientHandle)
    {
      // Rename the old property to the new SharedGradient name
      pNode->RenameProperty("Gradient", "SharedGradient");

      // Set GradientSource to SharedGradient (1) for backward compatibility
      pNode->AddProperty("GradientSource", (WInt32)1);
    }
    else
    {
      // If no old property exists, default to CustomGradient (0)
      pNode->AddProperty("GradientSource", (WInt32)0);
    }
  }
};

WParticleBehaviorFactory_ColorGradient_2_3 g_WParticleBehaviorFactory_ColorGradient_2_3;

//////////////////////////////////////////////////////////////////////////

/// Migrates RandomColor initializer from old hGradient to new GradientSource system
class WParticleInitializerFactory_RandomColor_2_3 : public WGraphPatch
{
public:
  WParticleInitializerFactory_RandomColor_2_3()
    : WGraphPatch("WParticleInitializerFactory_RandomColor", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Check if there's an old hGradient property (from version 2)
    auto* pGradientHandle = pNode->FindProperty("Gradient");
    if (pGradientHandle)
    {
      // Rename the old property to the new SharedGradient name
      pNode->RenameProperty("Gradient", "SharedGradient");

      // Set GradientSource to SharedGradient (1) for backward compatibility
      pNode->AddProperty("GradientSource", (WInt32)1);
    }
    else
    {
      // If no old property exists, default to CustomGradient (0)
      pNode->AddProperty("GradientSource", (WInt32)0);
    }
  }
};

WParticleInitializerFactory_RandomColor_2_3 g_WParticleInitializerFactory_RandomColor_2_3;
