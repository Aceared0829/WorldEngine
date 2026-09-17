#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

class WRenderPipelineNode;

/// Pin for connecting render pipeline nodes.
///
/// Pins represent input or output connections on render pipeline nodes. They can be used to pass
/// textures, render targets, or other data between pipeline nodes.
struct WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Input = W_BIT(0),           ///< Pin accepts input from other nodes.
      Output = W_BIT(1),          ///< Pin provides output to other nodes.
      PassThrough = W_BIT(2),     ///< Pin passes data through without modification.
      TextureProvider = W_BIT(3), ///< Pass provides pin texture to the pipeline each frame.
      Buffer = W_BIT(4),          ///< Pin is used for buffer connections instead of texture connections.

      Default = 0
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
      StorageType PassThrough : 1;
      StorageType TextureProvider : 1;
      StorageType Buffer : 1;
    };
  };

  WBitflags<Type> m_Type;
  WUInt8 m_uiInputIndex = 0xFF;
  WUInt8 m_uiOutputIndex = 0xFF;
  WRenderPipelineNode* m_pParent = nullptr;
};
W_DECLARE_FLAGS_OPERATORS(WRenderPipelineNodePin::Type);

/// Input pin for receiving data from other nodes.
struct WRenderPipelineNodeInputPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeInputPin() { m_Type = Type::Input; }
};

/// Output pin for sending data to other nodes.
struct WRenderPipelineNodeOutputPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeOutputPin() { m_Type = Type::Output; }
};

/// Input pin that also provides a texture each frame.
struct WRenderPipelineNodeInputProviderPin : public WRenderPipelineNodeInputPin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeInputProviderPin() { m_Type = Type::Input | Type::TextureProvider; }
};

/// Output pin that also provides a texture each frame.
struct WRenderPipelineNodeOutputProviderPin : public WRenderPipelineNodeOutputPin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider; }
};

/// Pass-through pin that forwards data without modification.
struct WRenderPipelineNodePassThroughPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodePassThroughPin() { m_Type = Type::PassThrough; }
};

/// Input pin for receiving buffer data from other nodes.
struct WRenderPipelineNodeBufferInputPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeBufferInputPin() { m_Type = Type::Input | Type::Buffer; }
};

/// Output pin for sending buffer data to other nodes.
struct WRenderPipelineNodeBufferOutputPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeBufferOutputPin() { m_Type = Type::Output | Type::Buffer; }
};

/// Buffer input pin that also provides the buffer's texture each frame.
struct WRenderPipelineNodeBufferInputProviderPin : public WRenderPipelineNodeBufferInputPin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeBufferInputProviderPin() { m_Type = Type::Input | Type::TextureProvider | Type::Buffer; }
};

/// Buffer output pin that also provides the buffer's texture each frame.
struct WRenderPipelineNodeBufferOutputProviderPin : public WRenderPipelineNodeBufferOutputPin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeBufferOutputProviderPin() { m_Type = Type::Output | Type::TextureProvider | Type::Buffer; }
};

/// Buffer pin that forwards data without modification.
struct WRenderPipelineNodeBufferPassThroughPin : public WRenderPipelineNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WRenderPipelineNodeBufferPassThroughPin() { m_Type = Type::PassThrough | Type::Buffer; }
};

/// Base class for nodes in a render pipeline.
///
/// Nodes represent stages in the rendering pipeline and are connected via pins.
/// Each node can have multiple input and output pins for passing textures and render targets.
class W_RENDERERCORE_DLL WRenderPipelineNode : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRenderPipelineNode, WReflectedClass);

public:
  virtual ~WRenderPipelineNode() = default;

  void InitializePins();

  /// Returns the name that the pin is registered under.
  ///
  /// Pins that were added through AddDynamicPins are also reachable under their reflected property name, in which case it is unspecified which of the two names is returned.
  WHashedString GetPinName(const WRenderPipelineNodePin* pPin) const;
  const WRenderPipelineNodePin* GetPinByName(WTempHashedString sName) const;
  const WArrayPtr<const WRenderPipelineNodePin* const> GetInputPins() const { return m_InputPins; }
  const WArrayPtr<const WRenderPipelineNodePin* const> GetOutputPins() const { return m_OutputPins; }

  /// Allows a node to expose its reflected pins under additional, data driven names.
  ///
  /// Called at the end of InitializePins. This only adds name lookups, the pins themselves still have to be reflected members, so GetInputPins and GetOutputPins are unaffected.
  virtual void AddDynamicPins(WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin) {}

private:
  WDynamicArray<const WRenderPipelineNodePin*> m_InputPins;
  WDynamicArray<const WRenderPipelineNodePin*> m_OutputPins;
  WHashTable<WHashedString, const WRenderPipelineNodePin*> m_NameToPin;
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodePin);

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeInputPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeOutputPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeInputProviderPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeOutputProviderPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodePassThroughPin);

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeBufferInputPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeBufferOutputPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeBufferInputProviderPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeBufferOutputProviderPin);
W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WRenderPipelineNodeBufferPassThroughPin);
