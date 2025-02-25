#define _CRT_SECURE_NO_WARNINGS 1
#include"ObjectPool.h"
#include"ConcurrentAlloc.h"

void Alloc1() {
    for (int i = 0; i < 5; ++i) {
        void* ptr = ConcurrentAlloc(6);
    }
}

void Alloc2() {
    for (int i = 0; i < 5; ++i) {
        void* ptr = ConcurrentAlloc(7);
    }
}

void TLSTest() {
    std::thread t1(Alloc1);//һ���߳�ִ��alloc1
    t1.join();
    std::thread t2(Alloc2);
    
    t2.join();
}

void MultiAlloc1() {
    std::vector<void*> v;
    for (int i = 0; i < 5; ++i) {
        void* ptr = ConcurrentAlloc(6);
        v.push_back(ptr);
    }

    for (auto e : v) {
        ConcurrentFree(e, 6);
    }
}

void MultiAlloc2() {
    std::vector<void*> v;
    for (int i = 0; i < 5; ++i) {
        void* ptr = ConcurrentAlloc(7);
        v.push_back(ptr);
    }

    for (auto e : v) {
        ConcurrentFree(e, 7);
    }
}

void TestMultiAlloc() {
    std::thread t1(MultiAlloc1);//һ���߳�ִ��alloc1
    
    std::thread t2(MultiAlloc2);
    t1.join();
    t2.join();
} 

void BigAlloc() {
    // <= 256kb ʹ�����㻺��
    // > 256kb(32*8k) һ����32*8k��128*8k֮��,ȥpagecache ��������128*8k,ȥϵͳ���Ͽ�
    void* p1 = ConcurrentAlloc(257 * 1024);
    ConcurrentFree(p1, 257 * 1024);
    void* p1 = ConcurrentAlloc(129 * 8 * 1024);
    ConcurrentFree(p1, 129 * 8 * 1024);
}

void TestConcurrenntAlloc1() {
    void* p1 = ConcurrentAlloc(6);
    //�Ƚ���concurrent �ȼ�������һ��Ͱ�Ͷ�����ͨ��TLSÿ���߳������ػ�ȡThreadCache
    //û���ݣ�ȥ��centralcacheҪ���ݣ���FetchFromCentralCache��������������������ȡ���
    // ��FetchRangeObj���ȼ���Ͱ������ȡһ���ж����span��
    // ��getonespan �鿴��ǰ��spanlist���Ƿ��л���δ��������span
    //�Ȱ�central cache�������������û�п���span��ȥ��page cacheҪ
    //pagecache�п��еĵ������أ�û�п���spanȥ�Ҷ�Ҫһ����span,�г�С�ķ���
    void* p2 = ConcurrentAlloc(8);
    void* p3 = ConcurrentAlloc(2);
    void* p4 = ConcurrentAlloc(7);
    void* p5 = ConcurrentAlloc(5);
    cout << p1 << endl;
    cout << p2 << endl;
    cout << p3 << endl;
    cout << p4 << endl;
    cout << p5 << endl;

    ConcurrentFree(p1, 6);
    ConcurrentFree(p2, 8);
    ConcurrentFree(p3, 2);
    ConcurrentFree(p4, 7);
    ConcurrentFree(p5, 5);
}

void TestConcurrenntAlloc2() {
    for (size_t i = 0; i < 5; ++i) {
        void* p1 = ConcurrentAlloc(6);
        cout << p1 << endl;
    }
    void* p2 = ConcurrentAlloc(4);
    cout << p2 << endl;

}

int main() {
   /* TestConcurrenntAlloc1();*/
    //TLSTest();
    TestMultiAlloc();
    return 0;
}