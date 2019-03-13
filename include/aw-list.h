#pragma once

#include "aw-debug.h"

namespace AW {

template <typename ItemType>
class TList {};

template <typename ItemType>
class TList<TUniquePtr<ItemType>> {
public:
    using TItemType = TUniquePtr<ItemType>;

    struct TItemBase {
        friend TList;
    //private:
        TUniquePtr<ItemType> Next;
    };

    class Iterator {
        friend TList;
    public:
        Iterator(ItemType* item)
            : Item(item) {
        }

        bool operator ==(const Iterator& iterator) {
            return Item == iterator.Item;
        }

        bool operator !=(const Iterator& iterator) {
            return Item != iterator.Item;
        }

        Iterator& operator ++() {
            Item = Item->Next.Get();
            return *this;
        }

        Iterator operator ++(int) {
            Iterator prev(Item);
            operator ++();
            return prev;
        }

        ItemType* Get() {
            return Item;
        }

    protected:
        ItemType* Item;
    };

    int size() {
        int s = 0;
        Iterator it = begin();
        while (it != end()) {
            ++s;
            ++it;
        }
        return s;
    }

    bool empty() {
        return begin() == end();
    }

    Iterator begin() {
        return Iterator(Begin.Get());
    }

    Iterator end() {
        return Iterator(nullptr);
    }

    const TItemType& front() {
        return Begin;
    }

    Iterator push_back(TItemType item) {
        return insert(end(), Move(item));
    }

    Iterator push_front(TItemType item) {
        return insert(begin(), Move(item));
    }

    Iterator pop_front() {
        return erase(begin());
    }

    TItemType pop_value(Iterator& it) {
        if (it == begin()) {
            TItemType oldBegin(Move(Begin));
            it = (Begin = Move(oldBegin->Next)).Get();
            return oldBegin;
        } else {
            Iterator next = begin();
            while (next != end()) {
                Iterator prev = next++;
                if (next == it) {
                    TItemType& prevNext(prev.Get()->Next);
                    TItemType value(Move(prevNext));
                    prevNext = Move(value->Next);
                    it = prevNext.Get();
                    return value;
                }
            }
        }
        return TItemType();
    }

    Iterator erase(Iterator it) {
        if (it == begin()) {
            TItemType oldNext(Move(Begin->Next));
            Begin = Move(oldNext);
            return Begin.Get();
        } else {
            Iterator curr = begin();
            while (curr != end()) {
                Iterator prev = curr++;
                if (curr == it) {
                    TItemType oldNext(Move(curr.Get()->Next));
                    return (prev.Get()->Next = Move(oldNext)).Get();
                }
            }
        }
        return end();
    }

    Iterator insert(Iterator it, TItemType item) {
        if (it == begin()) {
            item->Next = Move(Begin);
            Begin = Move(item);
            return begin();
        } else {
            Iterator curr = begin();
            while (curr != end()) {
                Iterator prev = curr++;
                if (curr == it) {
                    item->Next = Move(prev.Get()->Next);
                    prev.Get()->Next = Move(item);
                    return prev.Get()->Next.Get();
                }
            }
        }
        return end();
    }

protected:
    TUniquePtr<ItemType> Begin;
};

}