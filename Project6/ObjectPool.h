#pragma once
#include<iostream>
#include<Windows.h>
using std::cout;
using std::endl;

////template<size_t N> //定长模板参数表示申请N个空间
//inline static void* SystemAlloc(size_t kpage) {
//#ifdef _WIN32
//    void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//#else
////linux 下是bkr mmap
//#endif // _WIN
//    if (ptr == nullptr) throw std::bad_alloc();
//    return ptr;
//
//}
template<class T>//可以根据传的类型开辟空间
class ObjectPool {//定长内存池

private:
    char* _memory = nullptr;//申请来的大块地址空间,用char的原因是最小单位，比较好实现
    void* _freelist = nullptr;//存从大块空间切下来的空间
    size_t _remainBytes = 0;//切分过程中剩余的字节数
public:
    T* New() {
        T* obj = nullptr;
        if (_freelist) {
            //此时是链表中有切好的内存块，不继续切，直接复用就行
            //头删
            void* next = *((void**)_freelist);
            obj = _freelist;//把头节点处的地址空间给obj
            _freelist = next;//向后更新
        }
        else {
            if (_remainBytes < sizeof(T)) {//有不足一个对象长度时开空间
                _remainBytes = 128 * 1024;
               _memory = (char*)malloc(_remainBytes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    );
                if (_memory == nullptr) {
                    throw std::bad_alloc(); 
                }
                obj = (T*)_memory;
                size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);//防止在64位下没地方存指针
                _memory += sizeof(T);
                _remainBytes -= sizeof(T);
            }
        }
        //定位new，显式调，对这块空间初始化
        new(obj) T;
        return obj;
    }

    void Delete(T* obj) {
        //if(_freelist == nullptr){
            //_freelist = obj;
            //*(int*)obj = nullptr; //前四个字节存下一个对象的地址，但32-64位不同
            //*(void**)obj = nullptr;//根据void*在32/64位下的不同大小给出4-8不同大小的空间
        //}
        //else{//头插
        //显式调用析构i函数清理对象
        obj->~T();
        *(void**)obj = _freelist;//是void*的大小,4/8
        _freelist = obj;
    }
};
