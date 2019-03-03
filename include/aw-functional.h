#pragma once

namespace AW {

template <typename Type>
inline Type&& Move(Type&& value) {
    return static_cast<Type&&>(value);
}

struct IFunctionState {
    virtual ~IFunctionState() = default;
    virtual void Call() = 0;
};

template <typename FunctionType>
struct TFunctionState : IFunctionState {
    TFunctionState(FunctionType function)
        : Function(Move(function)) {
    }

    void Call() override {
        Function();
    }

    FunctionType Function;
};

class TFunction {
public:
    ~TFunction() {
        delete State;
    }

    TFunction()
        : State(nullptr) {
    }

    TFunction(const TFunction&) = delete;
    TFunction& operator =(const TFunction&) = delete;

    template <typename ArgumentType>
    TFunction(ArgumentType function)
        : State(new TFunctionState<ArgumentType>(Move(function))) {
    }

    void operator ()() {
        State->Call();
    }

protected:
    IFunctionState* State;
};

}