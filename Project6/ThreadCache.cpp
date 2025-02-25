#define _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocate(size_t size) {
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeClass::RoundUp(size);//����Ĵ�Сeg: 1->8, 10->16
	size_t index = SizeClass::Index(size);//�ڵڼ���Ͱeg: [0],[1]


	if (!_freelist[index].empty()) {
		return _freelist[index].Pop();//��Ϊ�ձ�ʾ��ϣͰ��ʱ�пռ���߳�ʹ�ã���һ���ڴ��ȥ����
	}//�������ٽ���Դ��������
	else {
		//��������������Ҫ����
		return FetchFromCentralCache(index, alignSize);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size) {
	assert(ptr);
	assert(size <= MAX_BYTES);
	//�ҳ���Ӧӳ�����������Ͱ������ͷ��
	size_t index = SizeClass::Index(size);
	_freelist[index].Push(ptr);
	//�������ȴ���һ������������ڴ�ʱ���Ϳ�ʼ��һ��list��centralcache
	if (_freelist[index].size() >= _freelist[index].MaxSize()) {//�˴���ʾ����̫����
		ListTooLong(_freelist[index], size);
	}
}
//TLS �̱߳��ش洢��һ�ֱ����Ĵ洢������������������ڵ��߳�����ȫ�ֿɷ��ʵģ����ǲ��ܱ������߳�
//���ʵ�����֤�̶߳����ԣ����Բ�����ȥ����

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize) {
	//����һ��ʼͰ�йҵ���С�ڴ�Ķ���ѡ���ڴ�С�Ķ�����飬�ڴ����ٸ����飬��ֹ�˷�
	//�����ڴ�С�Ķ���Ҳ������һ�θ�̫�࣬��һ��Ҳ���������ĸ�--����ʼ���������㷨
	size_t batchNum = min(_freelist[index].MaxSize(), SizeClass::NumMoveSize(alignSize));
	//����һ�θ����ٿռ�
	if (_freelist[index].MaxSize() == batchNum) {
		_freelist[index].MaxSize() += 1;//�������������ǰ��С�ڴ沿�֣�ÿ�λ�ȡ�����������++
	}

	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);//ʵ��ȡ�����ٶ���
	assert(actualNum > 0);

	if (actualNum == 1) {
		assert(start == end);
		return start;
	}
	else {
		_freelist[index].PushRange(NextObj(start), end, actualNum-1);
		return start;
	}
	return nullptr;
}

void ThreadCache::ListTooLong(FreeList& list, size_t size) {
	void* start = nullptr;
	void* end = nullptr;//ȥ��ȡһ���ڴ�
	list.PopRange(start, end, list.MaxSize());
	//���ڴ滹��centralcache
	CentralCache::GetInstance()->ReleaseListToSpans(start, size);

}