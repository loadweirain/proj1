#pragma once
#include "common.h"


class PageCache {//pagecache一共1-128个桶
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}
	Span* NewSpan(size_t size);
		//获取一个K页的span，实际获取一个两页的span是通过直接获取128页的span，切分成126和2，2返回给central
		//126挂到126号桶上给出去的span对象，其中的usecount如果0，表示span分出去的内存全部回来了
		//就可以把切出去的span还给pagecache，如果临近有空闲，直接合并。
	Span* MapObjToSpan(void* obj);

	void ReleaseSpanToPageCache(Span* span);
	std::mutex _pagemtx;
private:
	Spanlist _spanlists[NPAGS];
	std::unordered_map<PAGE_ID, Span*> _idSpanName;

	static PageCache _sInst;
	PageCache() {};
	PageCache(const PageCache&) = delete;
};
