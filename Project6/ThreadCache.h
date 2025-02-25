#pragma once
#include "common.h"

class ThreadCache {
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	void* FetchFromCentralCache(size_t index, size_t size);//去中心区域找内存
	void ListTooLong(FreeList& list, size_t size);//释放对象时如果链表过长，回收内存到中心区域
private:
	FreeList _freelist[NFREE_LIST];//这个结构是类似哈希桶，一个节点上挂着一个freelist
};

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//全局

