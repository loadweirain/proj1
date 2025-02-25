#pragma once

#include"common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;//饿汉模式,静态的去获取对象
	}
	Span* GetOneSpan(Spanlist& list, size_t byte_size);//获取一个非空的span
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);//从中心缓存获取n个的对像给obj
	void ReleaseListToSpans(void* start, size_t byte_size);//将一定数量的对象释放到span跨度
private:
	Spanlist _spanlists[NFREE_LIST];
private:
	static CentralCache _sInst;

	CentralCache() {
		//构造函数私有别人不能创建对象，只能在类里面调用
	}
	CentralCache(const CentralCache&) = delete;
};
