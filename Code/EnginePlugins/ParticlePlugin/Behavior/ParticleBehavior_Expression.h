#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Declarations.h>

W_DECLARE_FLAGS(WUInt16, WParticleStreamMask, Position, Velocity, Color, Size, LifeTime, Rotation);

/// One input slot for the expression behavior, holding either an inline or shared curve.
struct W_PARTICLEPLUGIN_DLL WParticleExpressionInput
{
  WEnum<WCurveSource> m_CurveSource;
  WSingleCurveData m_Curve;        ///< Edit-time curve data (available in editor). Converted to m_RuntimeCurve on save.
  WCurve1DResourceHandle m_hSharedCurve;
  mutable WCurve1D m_RuntimeCurve; ///< Populated from m_Curve on save or directly from stream on load.
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WParticleExpressionInput);

/// Evaluates a math expression per-particle, either to modify an attribute or to discard particles.
///
/// Particle streams are accessible via inPos, inVel, inColor, inSize, inRotSpeed, inLifeTime
/// and outPos, outVel, outColor, outSize, outRotSpeed, outLifeTime, outDiscard.
/// The global variable timeDiff provides the frame delta time.
///
/// Curve inputs defined on the factory can be sampled via sampleCurve(index, at),
/// where index is the 0-based position in the Inputs array and at is the lookup position.
///
/// Uses the WExpressionVM for SIMD-batched evaluation.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Expression final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Expression, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Expression();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(WSet<const WRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WString m_sExpression;
  WHybridArray<WParticleExpressionInput, 2> m_Inputs;

  /// Null if the expression is empty or failed to compile.
  const WExpressionByteCode* GetByteCode() const { return m_bBytecodeValid ? &m_ByteCode : nullptr; }

private:
  /// Parses and compiles m_sExpression into bytecode. Derives m_InputStreams, m_OutputStreams, and parameter name lists from which stream names appear in the expression text. No-ops if the expression has not changed since the last call.
  void CompileExpression(const WParticleEffectDescriptor& ownerEffectDescriptor);

  /// Evaluates all input curves (inline or shared) and stores fixed-size sample tables in m_CurveSamples for use by sampleCurve() at runtime.
  void BuildCurveSamples();

  WExpressionByteCode m_ByteCode;
  WString m_sCompiledExpression;
  bool m_bBytecodeValid = false;
  bool m_bUsesDiscard = false;
  WBitflags<WParticleStreamMask> m_InputStreams;
  WBitflags<WParticleStreamMask> m_OutputStreams;
  WDynamicArray<WSampledCurve1D> m_CurveSamples;
  WDynamicArray<WHashedString> m_FloatParamNames;
  WDynamicArray<WHashedString> m_ColorParamNames;
};

class W_PARTICLEPLUGIN_DLL WParticleBehavior_Expression final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Expression, WParticleBehavior);

public:
  WParticleBehavior_Expression();

  bool m_bUsesDiscard = false;
  WBitflags<WParticleStreamMask> m_InputStreams;
  WBitflags<WParticleStreamMask> m_OutputStreams;

  /// Points to the byte code owned by the factory. Null if the expression is invalid.
  const WExpressionByteCode* m_pByteCode = nullptr;

  /// Points to the pre-sampled curve data owned by the factory. Null if there are no curve inputs.
  const WDynamicArray<WSampledCurve1D>* m_pCurveSamples = nullptr;

  /// Points to the effect parameter name lists owned by the factory. Null if no parameters are used.
  const WDynamicArray<WHashedString>* m_pFloatParamNames = nullptr;
  const WDynamicArray<WHashedString>* m_pColorParamNames = nullptr;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override;

  void UpdateElements(WUInt64 uiStartIndex, WUInt64 uiNumElements);

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamVelocity = nullptr;
  WProcessingStream* m_pStreamSize = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WProcessingStream* m_pStreamLifeTime = nullptr;
  WProcessingStream* m_pStreamRotationSpeed = nullptr;

  WExpressionVM m_VM;
};
