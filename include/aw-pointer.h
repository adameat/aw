#pragma once

namespace AW {

template <typename Type>
struct TUniquePtr {
    TUniquePtr(Type* ptr = nullptr)
        : Ptr(ptr) {
    }

    /*TUniquePtr(TUniquePtr<Type>& ptr) {
        Ptr = ptr.Ptr;
        ptr.Ptr = nullptr;
    }*/

    TUniquePtr(TUniquePtr<Type>&& ptr) {
        Ptr = ptr.Ptr;
        ptr.Ptr = nullptr;
    }

    ~TUniquePtr() {
        delete Ptr;
        Ptr = (Type*)(-2);
    }

    /*TUniquePtr<Type>& operator =(Type* ptr) {
        delete Ptr;
        Ptr = ptr;
        return *this;
    }

    TUniquePtr<Type>& operator =(TUniquePtr<Type>& ptr) {
        TUniquePtr<Type> _this = *this;
        Ptr = ptr.Ptr;
        ptr.Ptr = nullptr;
        return *this;
    }*/

    TUniquePtr<Type>& operator =(TUniquePtr<Type>&& ptr) {
        delete Ptr;
        Ptr = ptr.Ptr;
        ptr.Ptr = nullptr;
        return *this;
    }

    Type* operator ->() const {
        return Ptr;
    }

    Type& operator *() const {
        return *Ptr;
    }

    Type* Get() const {
        return Ptr;
    }

    Type* Release() {
        Type* ptr = Ptr;
        Ptr = nullptr;
        return ptr;
    }

protected:
    Type* Ptr;
};

}
