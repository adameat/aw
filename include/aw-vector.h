#pragma once

namespace AW {

template <typename ItemType, int Capacity>
class TVector {
public:
    using Iterator = ItemType * ;

    TVector()
        : End(begin()) {
    }

    int size() const {
        return end() - begin();
    }

    bool empty() const {
        return begin() == end();
    }

    Iterator begin() {
        return reinterpret_cast<ItemType*>(&Data);
    }

    Iterator end() {
        return End;
    }

    void push_back(const ItemType& item) {
        insert(end(), item);
    }

    void push_front(const ItemType& item) {
        insert(begin(), item);
    }

    Iterator erase(Iterator it) {
        it->~ItemType();
        while (it != end()) {
            Iterator next = it;
            ++next;
            *it = *next;
            it = next;
        }
        --End;
        return it;
    }

    Iterator insert(Iterator it, const ItemType& item) {
        if (end() < capacity_end()) {
            if (it < end()) {
                Iterator to = end();
                while (to >= it) {
                    Iterator from = to;
                    --from;
                    *to = *from;
                    to = from;
                }
            }
            new (&*it) ItemType(item);
            ++End;
        }
        return it;
    }

protected:
    Iterator capacity_end() {
        return reinterpret_cast<ItemType*>(&Data[sizeof(ItemType) * Capacity]);
    }

    char Data[sizeof(ItemType) * Capacity];
    Iterator End;
};

}
