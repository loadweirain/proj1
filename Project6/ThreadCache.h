#pragma once
#include "common.h"

class ThreadCache {
public:
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	void* FetchFromCentralCache(size_t index, size_t size);//ȥ�����������ڴ�
	void ListTooLong(FreeList& list, size_t size);//�ͷŶ���ʱ�����������������ڴ浽��������
private:
	FreeList _freelist[NFREE_LIST];//����ṹ�����ƹ�ϣͰ��һ���ڵ��Ϲ���һ��freelist
};

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;//ȫ��

