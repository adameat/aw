#include <aw.h>
#include "aw-memory.h"

#ifdef _DEBUG_HEAP
constexpr size_t GUARD_SIZE = 4;
void* dmalloc(size_t size) {
    size_t gsize = GUARD_SIZE + sizeof(size) + size + GUARD_SIZE;
    unsigned char* data = static_cast<unsigned char*>(malloc(gsize));
    *(size_t*)data = size;
    memset(data + sizeof(size), '\xFD', GUARD_SIZE);
    memset(data + sizeof(size) + GUARD_SIZE, '\xCD', size);
    memset(data + sizeof(size) + GUARD_SIZE + size, '\xFD', GUARD_SIZE);
    return data + sizeof(size) + GUARD_SIZE;
}

void dfree(void* ptr) {
    unsigned char* data = static_cast<unsigned char*>(ptr) - sizeof(size_t) - GUARD_SIZE;
    void* begin = data;
    size_t size = *(size_t*)data;
    data += sizeof(size);
    for (size_t p = 0; p < GUARD_SIZE; ++p) {
        VERIFY(data[p] == '\xFD');
    }
    data += GUARD_SIZE;
    memset(data, '\xDD', size);
    data += size;
    for (size_t p = 0; p < GUARD_SIZE; ++p) {
        VERIFY(data[p] == '\xFD');
    }
    free(begin);
}
#else
#define dmalloc malloc
#define dfree free
#endif

//#ifdef ARDUINO
void* operator new(size_t size) {
	void* r = dmalloc(size);
	if (r == nullptr) {
		AW::Reset("Memory");
	}
	return r;
}

void* operator new[](size_t size) {
	return operator new(size);
}

void operator delete(void* ptr) {
	dfree(ptr);
}

void operator delete[](void* ptr) {
	dfree(ptr);
}
//#endif
