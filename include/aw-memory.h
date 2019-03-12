#pragma once

#ifdef ARDUINO
inline void* operator new(size_t size) {
	void* r = malloc(size);
	if (r == nullptr) {
		AW::Reset("Memory");
	}
	return r;
}

inline void* operator new[](size_t size) {
	return operator new(size);
}

inline void operator delete(void* ptr) {
	free(ptr);
}

inline void operator delete[](void* ptr) {
	free(ptr);
}
#endif
