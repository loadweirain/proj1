#pragma once
#include "common.h"


class PageCache {//pagecacheһ��1-128��Ͱ
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}
	Span* NewSpan(size_t size);
		//��ȡһ��Kҳ��span��ʵ�ʻ�ȡһ����ҳ��span��ͨ��ֱ�ӻ�ȡ128ҳ��span���зֳ�126��2��2���ظ�central
		//126�ҵ�126��Ͱ�ϸ���ȥ��span�������е�usecount���0����ʾspan�ֳ�ȥ���ڴ�ȫ��������
		//�Ϳ��԰��г�ȥ��span����pagecache������ٽ��п��У�ֱ�Ӻϲ���
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
