namespace AW {

class TToneActor : public TActor {
protected:
    unsigned char Pin;
public:
    TToneActor(unsigned char pin)
        : Pin(pin)
    {}

    void OnEvent(TEventPtr event, const TActorContext& context) override {
        switch (event->EventID) {
        case TEventData::EventID:
            return OnData(static_cast<TEventData*>(event.Release()), context);
        default:
            break;
        }
    }

    void OnData(TUniquePtr<TEventData> event, const TActorContext& context) {
        noTone(Pin);
        while (!event->Data.empty()) {
            char c = event->Data[0];
            event->Data.erase(0, 1);
            if (c >= 'A' && c <= 'Z') {
                unsigned long duration = 0;
                if (!event->Data.empty()) {
                    char d = event->Data[0];
                    if (d >= '0' && d <= '9') {
                        duration = (unsigned long)(c - '0') * 100;
                    }
                }
                if (duration > 0) {
                    tone(Pin, 500 + (c - 'A') * 300);
                } else {
                    tone(Pin, 500 + (c - 'A') * 300, duration);
                }
            } else if (c >= '0' && c <= '9') {
                event->NotBefore = context.Now + TTime::MilliSeconds((unsigned long)(c - '0') * 100);
                context.Resend(this, event.Release());
                return;
            }
        }
    }
};

}
