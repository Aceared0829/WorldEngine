#pragma once

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodePins.h>
#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

class WProcGenNodeBase : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenNodeBase, WReflectedClass);

public:
  struct GraphContext
  {
    enum OutputType
    {
      Unknown,
      Placement,
      Color,
    };

    WProcGenInternal::GraphSharedData m_SharedData;
    WSmallArray<WUInt8, 4> m_VolumeTagSetIndices;
    WSmallArray<WUInt8, 4> m_CurveIndices;
    OutputType m_OutputType = OutputType::Unknown;
  };

  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) = 0;
};

//////////////////////////////////////////////////////////////////////////

class WProcGenOutput : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGenOutput, WProcGenNodeBase);

public:
  bool m_bActive = true;

  void Save(WStreamWriter& inout_stream);
  void CopyValuesFromContext(const GraphContext& context);

  WString m_sName;

  WSmallArray<WUInt8, 4> m_VolumeTagSetIndices;
  WSmallArray<WUInt8, 4> m_CurveIndices;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_PlacementOutput : public WProcGenOutput
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_PlacementOutput, WProcGenOutput);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  void Save(WStreamWriter& inout_stream);

  WHybridArray<WString, 4> m_ObjectsToPlace;

  float m_fFootprint = 1.0f;

  WVec3 m_vMinOffset = WVec3(0);
  WVec3 m_vMaxOffset = WVec3(0);

  WAngle m_YawRotationSnap = WAngle::MakeFromRadian(0.0f);
  float m_fAlignToNormal = 1.0f;

  WVec3 m_vMinScale = WVec3(1);
  WVec3 m_vMaxScale = WVec3(1);

  float m_fCullDistance = 30.0f;

  WUInt32 m_uiCollisionLayer = 0;

  WString m_sSurface;

  WString m_sColorGradient;

  WEnum<WProcPlacementMode> m_PlacementMode;
  WUInt8 m_uiNumAdditionalRays = 4;
  float m_fRaySpread = 1.0f;

  WEnum<WProcPlacementPattern> m_PlacementPattern;

  WProcGenNodeInputPin m_DensityPin;
  WProcGenNodeInputPin m_ScalePin;
  WProcGenNodeInputPin m_ColorIndexPin;
  WProcGenNodeInputPin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_VertexColorOutput : public WProcGenOutput
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_VertexColorOutput, WProcGenOutput);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  void Save(WStreamWriter& inout_stream);

  WProcGenNodeInputPin m_RPin;
  WProcGenNodeInputPin m_GPin;
  WProcGenNodeInputPin m_BPin;
  WProcGenNodeInputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Random : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Random, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WInt32 m_iSeed = -1;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  WProcGenNodeOutputPin m_OutputValuePin;

private:
  void OnObjectCreated(const WAbstractObjectNode& node);

  WUInt32 m_uiAutoSeed;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_PerlinNoise : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_PerlinNoise, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WVec3 m_Scale = WVec3(10);
  WVec3 m_Offset = WVec3::MakeZero();
  WUInt32 m_uiNumOctaves = 3;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Blend : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Blend, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WEnum<WProcGenBinaryOperator> m_Operator;
  float m_fInputValueA = 1.0f;
  float m_fInputValueB = 1.0f;
  bool m_bClampOutput = false;

  WProcGenNodeInputPin m_InputValueAPin;
  WProcGenNodeInputPin m_InputValueBPin;
  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Remap : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Remap, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputMin = 0.0f;
  float m_fInputMax = 1.0f;
  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;
  bool m_bClampIntermediate = false;

  WProcGenNodeInputPin m_InputValuePin;
  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Curve : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Curve, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WSingleCurveData m_CurveData;
  WUInt32 m_uiNumSamples = 32;

  WProcGenNodeInputPin m_InputValuePin;
  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Contrast : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Contrast, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputValue = 0.5f;
  float m_fContrast = 0.0f;

  WProcGenNodeInputPin m_InputValuePin;
  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Height : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Height, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fMinHeight = 0.0f;
  float m_fMaxHeight = 1000.0f;
  float m_fLowerFade = 0.2f;
  float m_fUpperFade = 0.2f;

  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Slope : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Slope, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WAngle m_MinSlope = WAngle::MakeFromDegree(0.0f);
  WAngle m_MaxSlope = WAngle::MakeFromDegree(30.0f);
  float m_fLowerFade = 0.0f;
  float m_fUpperFade = 0.2f;

  WProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Position : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Position, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WProcGenNodeOutputPin m_XPin;
  WProcGenNodeOutputPin m_YPin;
  WProcGenNodeOutputPin m_ZPin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_Normal : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_Normal, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WProcGenNodeOutputPin m_XPin;
  WProcGenNodeOutputPin m_YPin;
  WProcGenNodeOutputPin m_ZPin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_MeshVertexColor : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_MeshVertexColor, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  WProcGenNodeOutputPin m_RPin;
  WProcGenNodeOutputPin m_GPin;
  WProcGenNodeOutputPin m_BPin;
  WProcGenNodeOutputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class WProcGen_ApplyVolumes : public WProcGenNodeBase
{
  W_ADD_DYNAMIC_REFLECTION(WProcGen_ApplyVolumes, WProcGenNodeBase);

public:
  virtual WExpressionAST::Node* GenerateExpressionASTNode(WTempHashedString sOutputName, WArrayPtr<WExpressionAST::Node*> inputs, WExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputValue = 0.0f;

  WTagSet m_IncludeTags;

  WEnum<WProcVolumeImageMode> m_ImageVolumeMode;
  WColorGammaUB m_RefColor;

  WProcGenNodeInputPin m_InputValuePin;
  WProcGenNodeOutputPin m_OutputValuePin;
};
