#define _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocate(size_t size) {
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);//对齐的大小eg: 1->8, 10->16
	size_t index = SizeClass::Index(size);//在第几个桶eg: [0],[1]


	if (!_freelist[index].empty()) {
		return _freelist[index].Pop();//不为空表示哈希桶此时有空间给线程使用，弹一块内存出去即可
	}//并不是临界资源不用上锁
	else {
		//访问中心区域需要上锁
		return FetchFromCentralCache(index, alignSize);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size) {
	assert(ptr);
	assert(size <= MAX_BYTES);
	//找出对应映射的自由链表桶，进行头插
	size_t index = SizeClass::Index(size);
	_freelist[index].Push(ptr);
	//当链表长度大于一次批量申请的内存时，就开始还一段list给centralcache
	if (_freelist[index].size() >= _freelist[index].MaxSize()) {//此处表示链表太长了
		ListTooLong(_freelist[index], size);
	}
}
//TLS 线程本地存储，一种变量的存储技术，这个变量在所在的线程内是全局可访问的，但是不能被其他线程
//访问到，保证线程独立性，可以不用锁去控制

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize) {
	//由于一开始桶中挂的是小内存的对象，选择内存小的多给几块，内存大的少给几块，防止浪费
	//但是内存小的对象也不可以一次给太多，第一次也就申请三四个--慢开始反馈调节算法
	size_t batchNum = min(_freelist[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
	//计算一次给多少空间
	if (_freelist[index].MaxSize() == batchNum) {
		_freelist[index].MaxSize() += 1;//慢增长，如果在前面小内存部分，每次获取完了最大数便++
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);//实际取到多少对象
	assert(actualNum > 0);

	if (actualNum == 1) {
		assert(start == end);
		return start;
	}
	else {
		_freelist[index].PushRange(NextObj(start), end, actualNum-1);
		return start;
	}
	return nullptr;
}

void ThreadCache::ListTooLong(FreeList& list, size_t size) {
	void* start = nullptr;
	void* end = nullptr;//去获取一批内存
	list.PopRange(start, end, list.MaxSize());
	//将内存还给centralcache
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);

}