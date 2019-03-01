#pragma once

namespace AW {

class TBluetoothZS040 {
protected:
    enum class EState {
        Error,
        AT,
        OK
    };

    EState State = EState::Error;
    TActor* Owner;
    TActor* Serial;
    bool Connected = false;
public:
    TBluetoothZS040(TActor* owner, TActor* serial)
        : Owner(owner)
        , Serial(serial) {}

    void Init(const TActorContext&) {
        //State = EState::AT;
        //context.Send(Owner, Serial, new TEventSerialData("AT"));
        State = EState::OK;
    }

    bool Receive(TUniquePtr<TEventData>& event, const TActorContext&) {
        switch (State) {
        case EState::Error:
            break;
        case EState::OK:
            if (Connected) {
                if (event->Data.starts_with("+")) {
                    if (event->Data == "+DISC:SUCCESS") {
                        Connected = false;
                    }
                }
            } else if (event->Data == "CONNECTED") {
                Connected = true;
            }
            return false;
        case EState::AT:
            if (event->Data == "OK") {
                State = EState::OK;
            } else {
                State = EState::Error;
            }
        }
        return true;
    }

    void Receive(TUniquePtr<TEventReceive>&, const TActorContext&) {
    }

    bool IsOK() const {
        return State == EState::OK;
    }

    bool IsConnected() const {
        return Connected;
    }
};
}