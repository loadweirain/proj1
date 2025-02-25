#pragma once
#include"PageCache.h"
#include"common.h"
#include"ThreadCache.h"
static void* ConcurrentAlloc(size_t size) {
	//通过TLS无锁获取自己专属的ThreadCache对象
	if (size > MAX_BYTES) {
		size_t alignSize = SizeClass::RoundUp(size);
		size_t kpage = alignSize >> PAGE_SHIFT;//计算出来在第几页
		PageCache::GetInstance()->_pagemtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kpage);//页号*8k就表示地址
		PageCache::GetInstance()->_pagemtx.unlock();
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	if (pTLSThreadCache == nullptr) {
		pTLSThreadCache = new ThreadCache;
	 }
	cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;
	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr,size_t size) {
	if (size > MAX_BYTES) {
		Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
	}
	assert(pTLSThreadCache);
	pTLSThreadCache->Deallocate(ptr, size);
}