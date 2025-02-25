#pragma once

#include"common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;//����ģʽ,��̬��ȥ��ȡ����
	}
	Span* GetOneSpan(Spanlist& list, size_t byte_size);//��ȡһ���ǿյ�span
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);//�����Ļ����ȡn���Ķ����obj
	void ReleaseListToSpans(void* start, size_t byte_size);//��һ�������Ķ����ͷŵ�span���
private:
	Spanlist _spanlists[NFREE_LIST];
private:
	static CentralCache _sInst;

	CentralCache() {
		//���캯��˽�б��˲��ܴ�������ֻ�������������
	}
	CentralCache(const CentralCache&) = delete;
};
