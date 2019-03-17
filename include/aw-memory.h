#pragma once

#ifdef ARDUINO
/*void* operator new(size_t size) {
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
}*/
#endif
