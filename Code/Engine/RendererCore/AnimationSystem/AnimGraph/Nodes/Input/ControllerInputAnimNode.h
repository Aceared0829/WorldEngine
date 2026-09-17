#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Reads input from game controllers and outputs it as animation graph values.
///
/// This node provides direct access to controller input (sticks, triggers, buttons) within animation graphs.
/// Use it to drive blend spaces from player input, control animation playback speed, or trigger animations
/// from button presses.
///
/// Note: This is only meant convenience during developing. A real game should use the input system to drive animations.
class W_RENDERERCORE_DLL WControllerInputAnimNode : public WAnimGraphNode
{
  W_ADD_DYNAMIC_REFLECTION(WControllerInputAnimNode, WAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // WAnimGraphNode

protected:
  virtual WResult SerializeNode(WStreamWriter& stream) const override;
  virtual WResult DeserializeNode(WStreamReader& stream) override;

  virtual void Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // WControllerInputAnimNode

private:
  WAnimGraphNumberOutputPin m_OutLeftStickX;   // [ property ]
  WAnimGraphNumberOutputPin m_OutLeftStickY;   // [ property ]
  WAnimGraphNumberOutputPin m_OutRightStickX;  // [ property ]
  WAnimGraphNumberOutputPin m_OutRightStickY;  // [ property ]

  WAnimGraphNumberOutputPin m_OutLeftTrigger;  // [ property ]
  WAnimGraphNumberOutputPin m_OutRightTrigger; // [ property ]

  WAnimGraphBoolOutputPin m_OutButtonA;        // [ property ]
  WAnimGraphBoolOutputPin m_OutButtonB;        // [ property ]
  WAnimGraphBoolOutputPin m_OutButtonX;        // [ property ]
  WAnimGraphBoolOutputPin m_OutButtonY;        // [ property ]

  WAnimGraphBoolOutputPin m_OutLeftShoulder;   // [ property ]
  WAnimGraphBoolOutputPin m_OutRightShoulder;  // [ property ]

  WAnimGraphBoolOutputPin m_OutPadLeft;        // [ property ]
  WAnimGraphBoolOutputPin m_OutPadRight;       // [ property ]
  WAnimGraphBoolOutputPin m_OutPadUp;          // [ property ]
  WAnimGraphBoolOutputPin m_OutPadDown;        // [ property ]
};
