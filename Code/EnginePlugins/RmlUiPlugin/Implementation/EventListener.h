#pragma once

#include <Foundation/Strings/HashedString.h>

#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/EventListenerInstancer.h>

class WRmlUiContext;

namespace WRmlUiInternal
{
  class EventListener final : public Rml::EventListener
  {
  public:
    virtual void ProcessEvent(Rml::Event& ref_event) override;

    virtual void OnDetach(Rml::Element* pElement) override;

  private:
    friend class EventListenerInstancer;
    WHashedString m_sIdentifier;
    WUInt32 m_uiIndex = 0;
  };

  class EventListenerInstancer final : public Rml::EventListenerInstancer
  {
  public:
    EventListenerInstancer();
    ~EventListenerInstancer();

    virtual Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element* pElement) override;

    void ReturnToPool(EventListener& ref_listener);

  private:
    WDeque<EventListener> m_EventListenerPool;
    WDynamicArray<WUInt32> m_EventListenerFreelist;
  };
} // namespace WRmlUiInternal
