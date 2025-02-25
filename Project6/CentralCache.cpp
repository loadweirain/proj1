#define _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"PageCache.h"
CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(Spanlist& list, size_t byte_size) {
	//�鿴��ǰ��spanlist���Ƿ��л���δ��������span
	Span* it = list.begin();
	while (it != list.end()) {
		if (it ->_freelist != nullptr) {//��ʾit��Ӧ��span���ж���
			return it;
		}
		else {
			it = it->_next;
		}
	}
	list._mtx.unlock();
	//֮ǰ��Ҫ��pagecache��Ͱ���⿪����������߳��ͷ��ڴ�����������������
	//�˴���ʾû�п���span�ˣ�ֻ����pagecacheҪ
	PageCache::GetInstance()->_pagemtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(byte_size));
	//�����һ����ϵͳ�����˶���ҳ����ȥ����span
	span->_isUse = true;
	PageCache::GetInstance()->_pagemtx.unlock();
	//��span�зֲ���Ҫ��������Ϊ�����߳�Ŀǰ���ʲ���
	char* start = (char*)(span->_pageId << PAGE_SHIFT);//Span����ʼ��ַ
	size_t bytes = span->_n << PAGE_SHIFT;//Span���ڴ��ֽ�����С
	char* end = start + bytes;
	//�Ѵ���ڴ��г������б���������
	span->_freelist = start;
	start += byte_size; //����һ��������ͷ���������
	void* tail = span->_freelist;
	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += byte_size;//β��
	}
	list._mtx.lock();//��Ҫ��span�ҵ�Ͱ���ټ���
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

	start = span->_freelist;//��end������n-1��������spanָ��end��next����start��end֮�������ȡ��
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while (i < n-1 && NextObj(end)!= nullptr) {//��span�л�ȡn�����󣬲���n�ж���ȡ����
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
		Span* span = PageCache::GetInstance()->MapObjToSpan(start);//����һ������һ��span

		NextObj(start) = span->_freelist;//ͷ��
		span->_freelist = start;
		span->_useCount--;

		if (span->_useCount == 0) {
			//��ʱ˵��span�зֳ�ȥ����С���ڴ涼�����ˣ����span�Ϳ����ٻ��ո�pagecache��pagecache���Գ������ϲ�
			_spanlists[index].erase(span);
			span->_freelist = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;

			_spanlists[index]._mtx.unlock();//�Ȱ�Ͱ�����
			PageCache::GetInstance()->_pagemtx.lock();//�ͷ�span�����ڴ滹��pagcacheҪ����
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pagemtx.unlock();

			_spanlists[index]._mtx.lock();
		}
		start = next;//һֱѭ����ȥ��start��ʼ���յ�ȫ���ռ�ҵ�span�����ͷ�
	}
	_spanlists[index]._mtx.unlock();

}