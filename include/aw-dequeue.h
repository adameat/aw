#pragma once

namespace AW {

template <typename ItemType, int Capacity>
class TDeque {};

template <typename ItemType, int Capacity>
class TDeque<TUniquePtr<ItemType>, Capacity> {
public:
    using TItemType = TUniquePtr<ItemType>;
    using Iterator = TItemType * ;

    TDeque()
        : BeginIdx(0)
        , EndIdx(0) {
    }

    int size() const {
        return EndIdx - BeginIdx;
    }

    int capacity() const {
        return Capacity;
    }

    bool empty() const {
        return BeginIdx == EndIdx;
    }

    Iterator begin() {
        return capacity_begin() + BeginIdx;
    }

    Iterator end() {
        return capacity_begin() + EndIdx;
    }

    TItemType& front() {
        return *begin();
    }

    void push_back(TItemType item) {
        insert(end(), Move(item));
    }

    void push_front(TItemType item) {
        insert(begin(), Move(item));
    }

    void pop_front() {
        *begin() = TItemType();
        if (++BeginIdx == EndIdx) {
            BeginIdx = EndIdx = 0;
        }
    }

    Iterator erase(Iterator it) {
        *it = nullptr;
        if (it == begin()) {
            ++BeginIdx;
            ++it;
        } else if (it == end() - 1) {
            --EndIdx;
        } else {
            // TODO: check the distance
            move(it + 1, end(), it);
            --EndIdx;
        }
        if (BeginIdx == EndIdx) {
            BeginIdx = EndIdx = 0;
            it = begin();
        }
        return it;
    }

    Iterator insert(Iterator it, TItemType item) {
        if (size() < capacity()) {
            if (it == begin()) {
                if (it > capacity_begin()) {
                    --BeginIdx;
                    *begin() = Move(item);
                    return begin();
                }
                move(it, end(), it + 1);
                ++EndIdx;
                *it = Move(item);
            } else
                if (it == end()) {
                    if (it < capacity_end()) {
                        *end() = Move(item);
                        ++EndIdx;
                        return it;
                    }
                    move(begin(), it, begin() - 1);
                    --BeginIdx;
                    --it;
                    *it = Move(item);
                }
        } else {
            Serial.print("\nOVERFLOW!\n");
        }
        return it;
    }

protected:
    void move(Iterator begin, Iterator end, Iterator to) {
        if (begin == end)
            return;
        if (to < begin) {
            Iterator it = begin;
            while (it != end) {
                *to = Move(*it);
                ++to;
                ++it;
            }
        } else {
            Iterator it = end;
            to += (end - begin);
            do {
                --to;
                --it;
                *to = Move(*it);
            } while (it != begin);
        }
    }

    Iterator capacity_begin() {
        return &Data[0];
    }

    Iterator capacity_end() {
        return &Data[Capacity];
    }

    TItemType Data[Capacity];
#ifndef ARDUINO
#pragma warning(disable:4201)
#endif
    struct {
        int BeginIdx : 4;
        int EndIdx : 4;
    };
#ifndef ARDUINO
#pragma warning(default:4201)
#endif
};

template <typename ItemType>
class TDeque<TUniquePtr<ItemType>, 0> {};

}
