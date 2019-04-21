#include <aw.h>
#include "aw-memory.h"

#ifdef _DEBUG_HEAP
constexpr size_t GUARD_SIZE = 4;
void* dmalloc(size_t size) {
    size_t gsize = sizeof(size) + GUARD_SIZE + size + GUARD_SIZE;
    unsigned char* data = static_cast<unsigned char*>(malloc(gsize));
    if (data == nullptr) {
        return data;
    }
    *(size_t*)data = size;
    memset(data + sizeof(size), 0xfb, GUARD_SIZE);
    memset(data + sizeof(size) + GUARD_SIZE, 0xcd, size);
    memset(data + sizeof(size) + GUARD_SIZE + size, 0xfd, GUARD_SIZE);
    return data + sizeof(size) + GUARD_SIZE;
}

void dfree(void* ptr) {
    unsigned char* data = static_cast<unsigned char*>(ptr) - GUARD_SIZE - sizeof(size_t);
    void* begin = data;
    size_t size = *(size_t*)data;
    data += sizeof(size);
    for (size_t p = 0; p < GUARD_SIZE; ++p) {
        VERIFY(data[p] == 0xfb);
    }
    data += GUARD_SIZE;
    memset(data, 0xdd, size);
    data += size;
    for (size_t p = 0; p < GUARD_SIZE; ++p) {
        VERIFY(data[p] == 0xfd);
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
