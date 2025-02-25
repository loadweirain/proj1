#define _CRT_SECURE_NO_WARNINGS 1
#include"PageCache.h"
#include"common.h"

PageCache PageCache::_sInst;
Span* PageCache::NewSpan(size_t k) {
	assert(k > 0);
	if (k > NPAGS - 1) {
		//去堆上直接要即可
		void* ptr = SystemAlloc(k);
		Span* span = new Span;
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;
		_idSpanName[span->_pageId] = span;
		return span;
	}
	if (!_spanlists[k].empty()) {//先检查第k个桶里有无span，有的话弹一个出去就行
		return _spanlists[k].pop_front();
	}
	//第k个桶里空的，检查后面的桶里面有无span,如果有可以把它进行切分
	for (size_t i = k+1; i < NPAGS; ++i) {
		if (!_spanlists[i].empty()) {
			//开始切 ,切分成一个第k页的span和一个i-k页的span，
			//k页的span返回给central cache  n-k页的span挂到第n-k个桶
			Span* nSpan = _spanlists[i].pop_front();//弹出来
			Span* kSpan = new Span;
			//在nspan的头部切一个k页下来
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanlists[nSpan->_n].push_front(nSpan);//挂上n-k
			//存到hash里,存nspan首位页号和span映射
			_idSpanName[nSpan->_pageId] = nSpan;
			_idSpanName[nSpan->_pageId + nSpan->_n - 1] = nSpan;

			for (PAGE_ID i = 0; i < kSpan->_n; ++i) {
				_idSpanName[kSpan->_pageId + i] = kSpan;
				//建立id和span的映射关系，方便central cache回收小块内存查找对应span

			}
			return kSpan;
		}
	}	
	//走到这里表示后面没有大块span
	//申请一个128页的大span，然后再去切开
	Span* BigSpan = new Span;
	void* ptr = SystemAlloc(NPAGS - 1);//表示在堆上申请空间
	BigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	BigSpan->_n = NPAGS - 1;
	_spanlists[BigSpan->_n].push_front(BigSpan);
	return NewSpan(k);//递归调自己，将128的大span切开
}

Span* PageCache::MapObjToSpan(void* obj) {
	//给对象后计算span
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
	//对span前后的页进行合并
	while (1) {
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = _idSpanName.find(prevId);
		if (ret == _idSpanName.end()) {
			break;
		}
		//1 前面页号没有，不合并
		Span* prevSpan = ret->second;
		if (prevSpan->_isUse == true) {
			break;
		}
		// 2 相邻页号在使用，不合并
		if (prevSpan->_n + span->_n > NPAGS - 1) {
			break;
		}
		//3 超过128管理范围，不合并
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;
		_spanlists[prevSpan->_n].erase(prevSpan);


		delete prevSpan;
	}

	//向后合并
	while (1) {
		PAGE_ID nextId = span->_pageId + span->_n;
		auto ret = _idSpanName.find(nextId);
		if (ret == _idSpanName.end()) {
			break;
		}
		//1 后 面页号没有，不合并
		Span* nextSpan = ret->second;
		if (nextSpan->_isUse == true) {
			break;
		}
		// 2 相邻页号在使用，不合并
		if (nextSpan->_n + span->_n > NPAGS - 1) {
			break;
		}
		//3 超过128管理范围，不合并
		span->_n += nextSpan->_n;
		_spanlists[nextSpan->_n].erase(nextSpan);


		delete nextSpan;
	}
	_spanlists[span->_n].push_front(span);
	span->_isUse = false;
	_idSpanName[span->_pageId + span->_n - 1] = span;
}