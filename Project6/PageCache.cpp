#define _CRT_SECURE_NO_WARNINGS 1
#include"PageCache.h"
#include"common.h"

PageCache PageCache::_sInst;
Span* PageCache::NewSpan(size_t k) {
	assert(k > 0);
	if (k > NPAGS - 1) {
		//ȥ����ֱ��Ҫ����
		void* ptr = SystemAlloc(k);
		Span* span = new Span;
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;
		_idSpanName[span->_pageId] = span;
		return span;
	}
	if (!_spanlists[k].empty()) {//�ȼ���k��Ͱ������span���еĻ���һ����ȥ����
		return _spanlists[k].pop_front();
	}
	//��k��Ͱ��յģ��������Ͱ��������span,����п��԰��������з�
	for (size_t i = k+1; i < NPAGS; ++i) {
		if (!_spanlists[i].empty()) {
			//��ʼ�� ,�зֳ�һ����kҳ��span��һ��i-kҳ��span��
			//kҳ��span���ظ�central cache  n-kҳ��span�ҵ���n-k��Ͱ
			Span* nSpan = _spanlists[i].pop_front();//������
			Span* kSpan = new Span;
			//��nspan��ͷ����һ��kҳ����
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanlists[nSpan->_n].push_front(nSpan);//����n-k
			//�浽hash��,��nspan��λҳ�ź�spanӳ��
			_idSpanName[nSpan->_pageId] = nSpan;
			_idSpanName[nSpan->_pageId + nSpan->_n - 1] = nSpan;

			for (PAGE_ID i = 0; i < kSpan->_n; ++i) {
				_idSpanName[kSpan->_pageId + i] = kSpan;
				//����id��span��ӳ���ϵ������central cache����С���ڴ���Ҷ�Ӧspan

			}
			return kSpan;
		}
	}	
	//�ߵ������ʾ����û�д��span
	//����һ��128ҳ�Ĵ�span��Ȼ����ȥ�п�
	Span* BigSpan = new Span;
	void* ptr = SystemAlloc(NPAGS - 1);//��ʾ�ڶ�������ռ�
	BigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	BigSpan->_n = NPAGS - 1;
	_spanlists[BigSpan->_n].push_front(BigSpan);
	return NewSpan(k);//�ݹ���Լ�����128�Ĵ�span�п�
}

Span* PageCache::MapObjToSpan(void* obj) {
	//����������span
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
	auto ret = _idSpanName.find(id);
	if (ret != _idSpanName.end()) {
		return ret->second;
	}
	else {
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span) {
	//��spanǰ���ҳ���кϲ�
	while (1) {
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = _idSpanName.find(prevId);
		if (ret == _idSpanName.end()) {
			break;
		}
		//1 ǰ��ҳ��û�У����ϲ�
		Span* prevSpan = ret->second;
		if (prevSpan->_isUse == true) {
			break;
		}
		// 2 ����ҳ����ʹ�ã����ϲ�
		if (prevSpan->_n + span->_n > NPAGS - 1) {
			break;
		}
		//3 ����128����Χ�����ϲ�
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;
		_spanlists[prevSpan->_n].erase(prevSpan);


		delete prevSpan;
	}

	//���ϲ�
	while (1) {
		PAGE_ID nextId = span->_pageId + span->_n;
		auto ret = _idSpanName.find(nextId);
		if (ret == _idSpanName.end()) {
			break;
		}
		//1 �� ��ҳ��û�У����ϲ�
		Span* nextSpan = ret->second;
		if (nextSpan->_isUse == true) {
			break;
		}
		// 2 ����ҳ����ʹ�ã����ϲ�
		if (nextSpan->_n + span->_n > NPAGS - 1) {
			break;
		}
		//3 ����128����Χ�����ϲ�
		span->_n += nextSpan->_n;
		_spanlists[nextSpan->_n].erase(nextSpan);


		delete nextSpan;
	}
	_spanlists[span->_n].push_front(span);
	span->_isUse = false;
	_idSpanName[span->_pageId + span->_n - 1] = span;
}