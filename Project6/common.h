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
#else //...Linux��
#endif // _WIN32


using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256*1024;
static const size_t NFREE_LIST = 208;//��ʾ�ܵĹ�ϣͰ����
static const size_t NPAGS = 129;//����128-129��һһӳ�䣬0��Ҫ
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
	//linux ����bkr mmap
#endif // _WIN
	if (ptr == nullptr) throw std::bad_alloc();
	return ptr;

}

static void*& NextObj(void* obj) {
	return *(void**)obj;
}
//�����зֺõ�С������������
class FreeList{
public:
	void Push(void* obj) {
		assert(obj);
		NextObj(obj) = _freelist;
		_freelist = obj;

		++_size;
	}

	void PushRange(void* start, void* end, size_t n) {
		NextObj(end) = _freelist;//start-end��ʾ����һ������ͳһ����β�壬end���뼴��
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

//���ڼ�������С�Ķ���ӳ�����
class SizeClass {
private:
	//size_t _RoundUp(size_t size, size_t AlignNum){//���������
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
			//��ʾ��256kb
			return _RoundUp(size, 1 << PAGE_SHIFT);//��һҳΪ��λ����

		}
	}
	//���ڼ���һ��thread cache �����Ļ����ȡ���ٸ�����
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);//[2,512]����������������
		if (size == 0) return 0;
		size_t num = MAX_BYTES / size; //256k/���������С
		if (num < 2) num = 2;//����һ��������
		if (num > 512) num = 512;
		return num;
	}
	static size_t NumMovePage(size_t size) {
		size_t num = NumMoveSize(size);
		size_t npage = num * size;

		npage >>= PAGE_SHIFT;
		if (npage == 0) npage = 1;//size��С��16�ֽڿ���ȡ��1ҳ
		return npage;//����һ����ϵͳ��ȡ����ҳ������size = 256�������num = 2��npage��521k/8k = 64ҳ

	}
	static inline size_t Index(size_t bytes) {//����ӳ�䵽��һ����������Ͱ

		assert(bytes <= MAX_BYTES);
		static int group_array[4] = { 16,56,56,56 };//��������Ͱ�ĸ���
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];//ǰ��һ���Ǳ�������±꣬�����Ǽ����ϸ���������±�
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
	//static inline size_t _Index(size_t bytes, size_t AlignNum) {// �ڶ�������ָ����2�ļ��η�
	//	if (bytes % AlignNum == 0) {
	//		return bytes / AlignNum - 1;
	//	}
	//	else {
	//		bytes / AlignNum;
	//	}
	//}
	static inline size_t _Index(size_t bytes, size_t AlignNum) {// �ڶ�������ָ����2�ļ��η�
		return ((bytes + (1 << AlignNum) - 1) >> AlignNum) - 1;
	}
};

struct Span {
	//����������ҳ����ڴ��Ƚṹ
	size_t _n = 0;//ҳ������
	PAGE_ID _pageId = 0;//����ڴ���ʼҳ��ҳ��
	Span* _next = nullptr;
	Span* _prev = nullptr;

	size_t _useCount = 0;//�кõ�С���ڴ汻�����thread cache�ļ���
	void* _freelist = nullptr;//�кõ�С���ڴ����������
	bool _isUse; //��ʾ�Ƿ��ڱ�ʹ��

};
//��ͷѭ��˫������
class Spanlist {
private:
	Span* _head = nullptr;
public:
	std::mutex _mtx;//ͭ��
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