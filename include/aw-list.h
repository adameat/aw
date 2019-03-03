#pragma once

namespace AW {

template <typename ItemType>
class TList {};

template <typename ItemType>
class TList<TUniquePtr<ItemType>> {
public:
    using TItemType = TUniquePtr<ItemType>;

    class TItemBase {
        friend TList;
    private:
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
        return insert(end(), item);
    }

    Iterator push_front(TItemType item) {
        return insert(begin(), item);
    }

    Iterator pop_front() {
        return erase(begin());
    }

    TItemType pop_value(Iterator& it) {
        if (it == begin()) {
            TItemType value(Begin);
            it = (Begin = value->Next).Get();
            return value;
        } else {
            Iterator next = begin();
            while (next != end()) {
                Iterator prev = next++;
                if (next == it) {
                    TItemType value(prev.Get()->Next);
                    it = (prev.Get()->Next = value->Next).Get();
                    value->Next = nullptr;
                    return value;
                }
            }
        }
        return TItemType();
    }

    Iterator erase(Iterator it) {
        if (it == begin()) {
            Begin = Begin->Next;
            return Begin.Get();
        } else {
            Iterator next = begin();
            while (next != end()) {
                Iterator prev = next++;
                if (next == it) {
                    return (prev.Get()->Next = next.Get()->Next).Get();
                }
            }
        }
        return end();
    }

    Iterator insert(Iterator it, TItemType item) {
        if (it == begin()) {
            item->Next = Begin;
            Begin = item;
            return begin();
        } else {
            Iterator next = begin();
            while (next != end()) {
                Iterator prev = next++;
                if (next == it) {
                    item->Next = prev.Get()->Next;
                    prev.Get()->Next = item;
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