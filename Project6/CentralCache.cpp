#define _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"PageCache.h"
CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(Spanlist& list, size_t byte_size) {
	//查看当前的spanlist中是否有还有未分配对象的span
	Span* it = list.begin();
	while (it != list.end()) {
		if (it ->_freelist != nullptr) {//表示it对应的span挂有对象
			return it;
		}
		else {
			it = it->_next;
		}
	}
	list._mtx.unlock();
	//之前需要把pagecache的桶锁解开，如果其他线程释放内存对象回来，不会阻塞
	//此处表示没有空闲span了，只能找pagecache要
	PageCache::GetInstance()->_pagemtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(byte_size));
	//计算出一次向系统申请了多少页，再去申请span
	span->_isUse = true;
	PageCache::GetInstance()->_pagemtx.unlock();
	//对span切分不需要加锁，因为其他线程目前访问不到
	char* start = (char*)(span->_pageId << PAGE_SHIFT);//Span的起始地址
	size_t bytes = span->_n << PAGE_SHIFT;//Span的内存字节数大小
	char* end = start + bytes;
	//把大块内存切成自由列表连接起来
	span->_freelist = start;
	start += byte_size; //先切一块下来做头，方便插入
	void* tail = span->_freelist;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += byte_size;//尾插
	}
	list._mtx.lock();//需要将span挂到桶里再加锁
	list.push_front(span);
	return span;


	return nullptr;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size) {
	size_t index = SizeClass::Index(byte_size);
	_spanlists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanlists[index], byte_size);
	assert(span);
	assert(span->_freelist);

	start = span->_freelist;//让end往后走n-1步，再让span指向end的next，将start到end之间的内容取走
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while (i < n-1 && NextObj(end)!= nullptr) {//从span中获取n个对象，不足n有多少取多少
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freelist = NextObj(end);
	NextObj(end) = nullptr;
	span->_useCount += actualNum;
	_spanlists[index]._mtx.unlock();
	return actualNum;
}

void CentralCache::ReleaseListToSpans(void* start, size_t size) {
	size_t index = SizeClass::Index(size);
	_spanlists[index]._mtx.lock();

	while (start) {
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjToSpan(start);//计算一下在哪一个span

		NextObj(start) = span->_freelist;//头插
		span->_freelist = start;
		span->_useCount--;

		if (span->_useCount == 0) {
			//此时说明span切分出去所有小块内存都回来了，这个span就可以再回收给pagecache，pagecache可以尝试做合并
			_spanlists[index].erase(span);
			span->_freelist = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;

			_spanlists[index]._mtx.unlock();//先把桶锁解掉
			PageCache::GetInstance()->_pagemtx.lock();//释放span，把内存还给pagcache要加锁
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pagemtx.unlock();

			_spanlists[index]._mtx.lock();
		}
		start = next;//一直循环下去把start开始到空的全部空间挂到span里，完成释放
	}
	_spanlists[index]._mtx.unlock();

}