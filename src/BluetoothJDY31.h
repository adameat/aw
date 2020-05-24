#pragma once

namespace AW {

class TBluetoothJDY31 {
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
    TBluetoothJDY31(TActor* owner, TActor* serial)
        : Owner(owner)
        , Serial(serial) {}

    void Init(const TActorContext&) {
    }

    bool Receive(TUniquePtr<TEventData>& event, const TActorContext&) {
        return true;
    }

    void Receive(TUniquePtr<TEventReceive>&, const TActorContext&) {
    }
};

}