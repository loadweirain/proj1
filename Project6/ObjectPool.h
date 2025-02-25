#pragma once
#include<iostream>
#include<Windows.h>
using std::cout;
using std::endl;

////template<size_t N> //����ģ�������ʾ����N���ռ�
//inline static void* SystemAlloc(size_t kpage) {
//#ifdef _WIN32
//    void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//#else
////linux ����bkr mmap
//#endif // _WIN
//    if (ptr == nullptr) throw std::bad_alloc();
//    return ptr;
//
//}
template<class T>//���Ը��ݴ������Ϳ��ٿռ�
class ObjectPool {//�����ڴ��

private:
    char* _memory = nullptr;//�������Ĵ���ַ�ռ�,��char��ԭ������С��λ���ȽϺ�ʵ��
    void* _freelist = nullptr;//��Ӵ��ռ��������Ŀռ�
    size_t _remainBytes = 0;//�зֹ�����ʣ����ֽ���
public:
    T* New() {
        T* obj = nullptr;
        if (_freelist) {
            //��ʱ�����������кõ��ڴ�飬�������У�ֱ�Ӹ��þ���
            //ͷɾ
            void* next = *((void**)_freelist);
            obj = _freelist;//��ͷ�ڵ㴦�ĵ�ַ�ռ��obj
            _freelist = next;//������
        }
        else {
            if (_remainBytes < sizeof(T)) {//�в���һ�����󳤶�ʱ���ռ�
                _remainBytes = 128 * 1024;
               _memory = (char*)malloc(_remainBytes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    );
                if (_memory == nullptr) {
                    throw std::bad_alloc(); 
                }
                obj = (T*)_memory;
                size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);//��ֹ��64λ��û�ط���ָ��
                _memory += sizeof(T);
                _remainBytes -= sizeof(T);
            }
        }
        //��λnew����ʽ���������ռ��ʼ��
        new(obj) T;
        return obj;
    }

    void Delete(T* obj) {
        //if(_freelist == nullptr){
            //_freelist = obj;
            //*(int*)obj = nullptr; //ǰ�ĸ��ֽڴ���һ������ĵ�ַ����32-64λ��ͬ
            //*(void**)obj = nullptr;//����void*��32/64λ�µĲ�ͬ��С����4-8��ͬ��С�Ŀռ�
        //}
        //else{//ͷ��
        //��ʽ��������i�����������
        obj->~T();
        *(void**)obj = _freelist;//��void*�Ĵ�С,4/8
        _freelist = obj;
    }
};
