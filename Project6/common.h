#pragma once
#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>
#include<mutex>
#include<unordered_map>
#include<algorithm>
#ifdef _WIN32
#include<windows.h>
#else //...Linux下
#endif // _WIN32


using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256*1024;
static const size_t NFREE_LIST = 208;//表示总的哈希桶数量
static const size_t NPAGS = 129;//修正128-129：一一映射，0不要
static const size_t PAGE_SHIFT = 13;

#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif // _WIN32

inline static void* SystemAlloc(size_t kpage) {
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux 下是bkr mmap
#endif // _WIN
	if (ptr == nullptr) throw std::bad_alloc();
	return ptr;

}

static void*& NextObj(void* obj) {
	return *(void**)obj;
}
//管理切分好的小对象，自由链表
class FreeList{
public:
	void Push(void* obj) {
		assert(obj);
		NextObj(obj) = _freelist;
		_freelist = obj;

		++_size;
	}

	void PushRange(void* start, void* end, size_t n) {
		NextObj(end) = _freelist;//start-end表示的是一条链，统一进行尾插，end插入即可
		_freelist = start;

		_size += n;
	}

	void PopRange(void*& start, void*& end, size_t n) {
		assert(n >= _size);
		start = _freelist;
		end = start;
		for (size_t i = 0; i < n-1; ++i) {
			end = NextObj(end);
		}

		_freelist = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}
	void* Pop(){
		assert(_freelist);
		void* obj = _freelist;
		_freelist = NextObj(obj);

		--_size;
		return obj;
	}

	size_t size() {
		return _size;
	}
	bool empty() {
		return _freelist == nullptr;
	}
	size_t& MaxSize() {
		return _maxSize;
	}
private:
	void* _freelist = nullptr;
	size_t _maxSize = 1;
	size_t _size = 0;
};

//用于计算对象大小的对齐映射规则
class SizeClass {
private:
	//size_t _RoundUp(size_t size, size_t AlignNum){//计算对齐数
	//	size_t ALignSize;
	//	if (size % AlignNum != 0) {
	//		ALignSize = (size / AlignNum + 1) * AlignNum;
	//	}
	//	else {
	//		ALignSize = size;
	//	}
	//}

public:
	static inline size_t _RoundUp(size_t size, size_t AlignNum) {
		return (((size)+AlignNum - 1) & ~(AlignNum - 1));
	}
	static inline size_t RoundUp(size_t size) {
		if (size <= 128) {
			return _RoundUp(size, 8);
		}
		else if (size <= 1024) {
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024) {
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024) {
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024) {
			return _RoundUp(size, 8 * 1024);
		}
		else {
			//表示到256kb
			return _RoundUp(size, 1 << PAGE_SHIFT);//以一页为单位对齐

		}
	}
	//用于计算一次thread cache 从中心缓存获取多少个对象
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);//[2,512]是慢启动的上下限
		if (size == 0) return 0;
		size_t num = MAX_BYTES / size; //256k/单个对象大小
		if (num < 2) num = 2;//控制一下上下限
		if (num > 512) num = 512;
		return num;
	}
	static size_t NumMovePage(size_t size) {
		size_t num = NumMoveSize(size);
		size_t npage = num * size;

		npage >>= PAGE_SHIFT;
		if (npage == 0) npage = 1;//size最小在16字节可以取到1页
		return npage;//计算一次向系统获取多少页，例如size = 256，计算出num = 2，npage是521k/8k = 64页

	}
	static inline size_t Index(size_t bytes) {//计算映射到哪一个自由链表桶

		assert(bytes <= MAX_BYTES);
		static int group_array[4] = { 16,56,56,56 };//各个区间桶的个数
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];//前面一半是本区间的下标，后面是加上上个区间的总下标
		}
		else if (bytes <= 8*1024) {
			return _Index(bytes - 1024, 7) + group_array[0]+ group_array[1];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8*1024, 10) + group_array[2] + group_array[1];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[2] + group_array[3];
		}
	}
	//static inline size_t _Index(size_t bytes, size_t AlignNum) {// 第二个参数指的是2的几次方
	//	if (bytes % AlignNum == 0) {
	//		return bytes / AlignNum - 1;
	//	}
	//	else {
	//		bytes / AlignNum;
	//	}
	//}
	static inline size_t _Index(size_t bytes, size_t AlignNum) {// 第二个参数指的是2的几次方
		return ((bytes + (1 << AlignNum) - 1) >> AlignNum) - 1;
	}
};

struct Span {
	//管理多个连续页大块内存跨度结构
	size_t _n = 0;//页的数量
	PAGE_ID _pageId = 0;//大块内存起始页的页号
	Span* _next = nullptr;
	Span* _prev = nullptr;

	size_t _useCount = 0;//切好的小块内存被分配给thread cache的计数
	void* _freelist = nullptr;//切好的小块内存的自由链表
	bool _isUse; //表示是否在被使用

};
//带头循环双向链表
class Spanlist {
private:
	Span* _head = nullptr;
public:
	std::mutex _mtx;//铜锁
public:
	Spanlist() {
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	Span* begin() {
		return _head->_next;
	}
	void push_front(Span* span) {
		insert(begin(), span);
	}
	Span* pop_front() {
		Span* front = _head->_next;
		erase(front);
		return front;
	}
	Span* end() {
		return _head;
	}

	bool empty() {
		return _head->_next == _head;
	}
	void insert(Span* pos, Span* newSpan) {
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}

	void erase(Span* pos) {
		assert(pos);
		assert(pos != _head);
		Span* prev = pos->_prev;
		prev->_next = pos->_next;
		pos->_next->_prev = prev;
	}
};