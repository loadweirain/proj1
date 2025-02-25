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
    std::thread t1(Alloc1);//一个线程执行alloc1
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
    std::thread t1(MultiAlloc1);//一个线程执行alloc1
    
    std::thread t2(MultiAlloc2);
    t1.join();
    t2.join();
} 

void BigAlloc() {
    // <= 256kb 使用三层缓存
    // > 256kb(32*8k) 一、在32*8k到128*8k之间,去pagecache 二、大于128*8k,去系统堆上开
    void* p1 = ConcurrentAlloc(257 * 1024);
    ConcurrentFree(p1, 257 * 1024);
    void* p1 = ConcurrentAlloc(129 * 8 * 1024);
    ConcurrentFree(p1, 129 * 8 * 1024);
}

void TestConcurrenntAlloc1() {
    void* p1 = ConcurrentAlloc(6);
    //先进入concurrent 先计算在哪一号桶和对齐数通过TLS每个线程无锁地获取ThreadCache
    //没数据，去找centralcache要数据，走FetchFromCentralCache，慢增长，计算批量获取多大
    // 走FetchRangeObj，先加上桶锁，获取一个有对象的span，
    // 走getonespan 查看当前的spanlist中是否有还有未分配对象的span
    //先把central cache的锁解掉，发现没有空闲span，去找page cache要
    //pagecache有空闲的弹出返回，没有空闲span去找堆要一个大span,切成小的返回
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