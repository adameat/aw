#pragma once

namespace AW {

struct TEventLedOn : TBasicEvent<TEventLedOn> {
    constexpr static TEventID EventID = TEventID::EventPrivate0;
};

struct TEventLedOff : TBasicEvent<TEventLedOff> {
    constexpr static TEventID EventID = TEventID::EventPrivate1;
};

struct TEventLedBlink : TBasicEvent<TEventLedBlink> {
    constexpr static TEventID EventID = TEventID::EventPrivate2;
    TTime Period;

    TEventLedBlink(TTime period)
        : Period(period) {}
};

class TLedActor : public TActor {
protected:
    TDigitalPin<LED_BUILTIN> LedPin;
    bool Led = false;

    TLedActor()
    {}

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventLedOn::EventID:
            return OnLedOn(static_cast<TEventLedOn*>(event.Release()), context);
        case TEventLedOff::EventID:
            return OnLedOff(static_cast<TEventLedOff*>(event.Release()), context);
        case TEventLedBlink::EventID:
            return OnLedBlink(static_cast<TEventLedBlink*>(event.Release()), context);
        case TEventReceive::EventID:
            return OnReceive(static_cast<TEventReceive*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnLedOn(TUniquePtr<TEventLedOn>, const TActorContext&) {
        LedPin = Led = true;
    }

    void OnLedOff(TUniquePtr<TEventLedOff>, const TActorContext&) {
        LedPin = Led = false;
    }

    void OnLedBlink(TUniquePtr<TEventLedBlink> event, const TActorContext& context) {
        if (!Led) {
            context.Send(this, this, new TEventReceive());
            context.Send(this, this, new TEventReceive(context.Now + event->Period));
        }
    }

    void OnReceive(TUniquePtr<TEventReceive> /*event*/, const TActorContext& /*context*/) {
        LedPin = Led = !Led;
    }
};

}
