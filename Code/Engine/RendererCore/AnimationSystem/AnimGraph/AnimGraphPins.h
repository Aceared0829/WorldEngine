#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class WAnimGraphInstance;
class WAnimController;
class WStreamWriter;
class WStreamReader;
struct WAnimGraphPinDataBoneWeights;
struct WAnimGraphPinDataLocalTransforms;
struct WAnimGraphPinDataModelTransforms;

/// Shared bone weight mask used to control which bones are affected by animations.
///
/// Bone weights are globally shared across all characters to save memory. Created via
/// WAnimController::CreateBoneWeights() with a unique name for caching.
struct WAnimGraphSharedBoneWeights : public WRefCounted
{
  WDynamicArray<ozz::math::SimdFloat4, WAlignedAllocatorWrapper> m_Weights;
};

using WAnimPoseGeneratorLocalPoseID = WUInt32;
using WAnimPoseGeneratorModelPoseID = WUInt32;
using WAnimPoseGeneratorCommandID = WUInt32;

/// Base class for all animation graph pins, representing typed connections between nodes.
///
/// Pins are the connection points on nodes that allow data to flow through the graph. Each pin has
/// a type (trigger, number, bool, pose, etc.) and can be either an input or output. Pins are connected
/// in the graph editor to define data flow.
///
/// ## Pin Indices
///
/// During graph preparation, each pin is assigned an index (m_iPinIndex) that identifies its storage
/// location in the instance data arrays. Unconnected pins have index -1 and return default values.
///
/// ## Data Flow
///
/// Output pins write values to the instance's pin state arrays, input pins read from those arrays.
/// The mapping from output pin indices to input pin indices is pre-computed during PrepareForUse().
///
/// ## Pin Types
///
/// - **Trigger**: One-shot events (animation finished, state entered)
/// - **Number**: Floating-point values for blend weights, speeds, parameters
/// - **Bool**: Boolean flags and conditions
/// - **BoneWeights**: Masks controlling which bones are affected by animations
/// - **LocalPose**: Bone transforms in local space (parent-relative)
/// - **ModelPose**: Bone transforms in model space (skeleton root-relative)
class W_RENDERERCORE_DLL WAnimGraphPin : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphPin, WReflectedClass);

public:
  enum Type : WUInt8
  {
    Invalid,
    Trigger,     ///< One-shot events that occur for a single frame
    Number,      ///< Double-precision floating-point values
    Bool,        ///< Boolean true/false values
    BoneWeights, ///< Masks controlling bone influence
    LocalPose,   ///< Bone transforms in local space
    ModelPose,   ///< Bone transforms in model space
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  /// Returns whether this pin is connected to another pin.
  ///
  /// Unconnected pins have index -1 and return default values when queried.
  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  virtual WAnimGraphPin::Type GetPinType() const = 0;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);

protected:
  friend class WAnimGraph;

  WInt16 m_iPinIndex = -1;       ///< Index into the instance's pin state array, -1 if unconnected
  WUInt8 m_uiNumConnections = 0; ///< Number of connections to this pin
};

/// Base class for input pins that receive data from connected output pins.
class W_RENDERERCORE_DLL WAnimGraphInputPin : public WAnimGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphInputPin, WAnimGraphPin);

public:
};

/// Base class for output pins that send data to connected input pins.
class W_RENDERERCORE_DLL WAnimGraphOutputPin : public WAnimGraphPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphOutputPin, WAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving trigger events.
///
/// Triggers are one-shot events that are active for a single frame. Common uses include signaling
/// animation completion, state transitions, or specific animation events.
class W_RENDERERCORE_DLL WAnimGraphTriggerInputPin : public WAnimGraphInputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphTriggerInputPin, WAnimGraphInputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Trigger; }

  /// Returns whether at least one connected output pin triggered this frame.
  bool IsTriggered(WAnimGraphInstance& ref_graph) const;

  /// Returns whether all connected output pins triggered this frame.
  ///
  /// Useful for nodes that need all inputs to be ready before proceeding.
  bool AreAllTriggered(WAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending trigger events.
///
/// Trigger pins send one-shot events that last for a single frame.
class W_RENDERERCORE_DLL WAnimGraphTriggerOutputPin : public WAnimGraphOutputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphTriggerOutputPin, WAnimGraphOutputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Trigger; }

  /// Sets this output pin to the triggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(WAnimGraphInstance& ref_graph) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving number values.
///
/// Number pins carry double-precision floating-point values used for blend weights, animation speeds,
/// parameters, and other numerical data.
class W_RENDERERCORE_DLL WAnimGraphNumberInputPin : public WAnimGraphInputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphNumberInputPin, WAnimGraphInputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Number; }

  /// Retrieves the number value from connected output pins.
  ///
  /// Returns fFallback if the pin is not connected.
  double GetNumber(WAnimGraphInstance& ref_graph, double fFallback = 0.0) const;
};

/// Output pin for sending number values.
class W_RENDERERCORE_DLL WAnimGraphNumberOutputPin : public WAnimGraphOutputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphNumberOutputPin, WAnimGraphOutputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Number; }

  void SetNumber(WAnimGraphInstance& ref_graph, double value) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving boolean values.
///
/// Bool pins carry true/false values used for conditions, flags, and logical operations.
class W_RENDERERCORE_DLL WAnimGraphBoolInputPin : public WAnimGraphInputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphBoolInputPin, WAnimGraphInputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Bool; }

  /// Retrieves the boolean value from connected output pins.
  ///
  /// Returns bFallback if the pin is not connected.
  bool GetBool(WAnimGraphInstance& ref_graph, bool bFallback = false) const;
};

/// Output pin for sending boolean values.
class W_RENDERERCORE_DLL WAnimGraphBoolOutputPin : public WAnimGraphOutputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphBoolOutputPin, WAnimGraphOutputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::Bool; }

  void SetBool(WAnimGraphInstance& ref_graph, bool bValue) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving bone weight masks.
///
/// Bone weight pins carry masks that control which bones are affected by animations, enabling
/// partial skeleton animations (e.g., upper body only, lower body only).
class W_RENDERERCORE_DLL WAnimGraphBoneWeightsInputPin : public WAnimGraphInputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphBoneWeightsInputPin, WAnimGraphInputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::BoneWeights; }

  WAnimGraphPinDataBoneWeights* GetWeights(WAnimController& ref_controller, WAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending bone weight masks.
class W_RENDERERCORE_DLL WAnimGraphBoneWeightsOutputPin : public WAnimGraphOutputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphBoneWeightsOutputPin, WAnimGraphOutputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::BoneWeights; }

  void SetWeights(WAnimGraphInstance& ref_graph, WAnimGraphPinDataBoneWeights* pWeights) const;
};

//////////////////////////////////////////////////////////////////////////

/// Input pin for receiving a single local pose.
///
/// Local pose pins carry bone transforms in local space (relative to parent bone). This is the
/// output of animation sampling and blending before forward kinematics is applied.
class W_RENDERERCORE_DLL WAnimGraphLocalPoseInputPin : public WAnimGraphInputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphLocalPoseInputPin, WAnimGraphInputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::LocalPose; }

  WAnimGraphPinDataLocalTransforms* GetPose(WAnimController& ref_controller, WAnimGraphInstance& ref_graph) const;
};

/// Output pin for sending local poses.
class W_RENDERERCORE_DLL WAnimGraphLocalPoseOutputPin : public WAnimGraphOutputPin
{
  W_ADD_DYNAMIC_REFLECTION(WAnimGraphLocalPoseOutputPin, WAnimGraphOutputPin);

public:
  virtual WAnimGraphPin::Type GetPinType() const override { return WAnimGraphPin::LocalPose; }

  void SetPose(WAnimGraphInstance& ref_graph, WAnimGraphPinDataLocalTransforms* pPose) const;
};
