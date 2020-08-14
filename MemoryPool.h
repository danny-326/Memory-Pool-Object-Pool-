//
// Created by Danny on 2020/8/12.
//

#ifndef MEMORYPOOL_MEMORYPOOL_H
#define MEMORYPOOL_MEMORYPOOL_H
#include<cstddef>
#include<list>
#include"RawList.h"
#define MEMPOOL_ALIGNMENT 8
template <size_t size>
struct MemoryUnit;
template <size_t size>
union MemoryUnitData
{
    MemoryUnit<size>* next;
    char data[size];
}__attribute__ ((packed));//turn off mem alignment

template <size_t unitSize>
struct MemoryBlock;

template <size_t size>
struct MemoryUnit
{
    MemoryUnitData<size> data;
    MemoryBlock<size>* source;
}__attribute__ ((packed));
template <size_t unitSize>
struct MemoryBlock
{
    static inline void * operator new (size_t ,int sizeN);
    static inline void operator delete(void *ptr);
    explicit inline MemoryBlock(int sizeN);
    inline void *fetch();
    inline bool check(void *ptr);   //check whether ptr in this block;
    inline void recycle(void *ptr);
    MemoryBlock *next;
    MemoryBlock *last;
    MemoryUnit<unitSize> *data;
    MemoryUnit<unitSize> *end;
    int nfree;
    int sizeN;
    MemoryUnit<unitSize>* first;
    char dataStart[1];
private:

};
template <typename Type>
class MemoryPool
{
public:
    MemoryPool(int growSizeN,size_t maxPreserveBlockN);
    inline void *alloc();
    inline int recycle(void* ptr);
private:
    int growSizeN;
    int maxPreserve;
    RawList<MemoryBlock<sizeof(Type)> > noFree;
    RawList<MemoryBlock<sizeof(Type)> > free;
};

template<typename Type>
MemoryPool<Type>::MemoryPool(int growSizeN,size_t maxPreserveBlockN):
growSizeN(growSizeN),
maxPreserve(maxPreserveBlockN)
{

}

template<typename Type>
inline void *MemoryPool<Type>::alloc()
{
    void *ret;
    while(true)
    {
        if(this->free.size())
        {
            ret = this->free.front()->fetch();
            if(!this->free.front()->nfree)
            {
                auto front = this->free.front();
                this->free.popFront();
                this->noFree.pushBack(front);
            }
            return ret;
        }
        else
        {
            this->free.pushBack(new(this->growSizeN) MemoryBlock<sizeof(Type)>(this->growSizeN));
        }
    }
}

template<typename Type>
inline int MemoryPool<Type>::recycle(void *ptr)
{
    auto p = (MemoryUnit<sizeof(Type)>*)ptr;
    if(!p->source->nfree)
    {
        p->source->recycle(ptr);
        this->noFree.remove(p->source);
        this->free.pushBack(p->source);
    }
    else
    {
        p->source->recycle(ptr);
    }
    if(this->maxPreserve<this->free.size()+this->noFree.size()&&p->source->nfree==p->source->sizeN)
    {
        //delete a block;
        this->free.remove(p->source);
        delete p->source;
    }
    return 0;
}

template<size_t unitSize>
inline MemoryBlock<unitSize>::MemoryBlock(int sizeN):sizeN(sizeN),nfree(sizeN),next(nullptr),last(nullptr)
{
    this->data = (MemoryUnit<unitSize>*)this->dataStart;
    this->end = this->data+sizeN;

    this->first = this->data;
    for(auto p = this->data;p+1<this->end;p++)
    {
        p->data.next = p+1;
    }

}

template<size_t unitSize>
inline void *MemoryBlock<unitSize>::fetch()
{
    if(!this->nfree)
    {
        return nullptr;
    }
    auto temp = this->first;
    temp->source = this;
    this->first = this->first->data.next;
    --this->nfree;
    return temp;
}

template<size_t unitSize>
inline bool MemoryBlock<unitSize>::check(void *ptr)
{
    return ptr<=this->end&&ptr>=this->data;
}

template<size_t unitSize>
inline void MemoryBlock<unitSize>::recycle(void *ptr)
{
    ((MemoryUnit<unitSize>*)ptr)->data.next = this->first;
    this->first = (MemoryUnit<unitSize>*)ptr;
    ++this->nfree;
}

template<size_t unitSize>
inline void *MemoryBlock<unitSize>::operator new(size_t, int sizeN)
{
    return ::operator new(sizeof(MemoryBlock<unitSize>)+sizeN*sizeof(MemoryUnit<unitSize>));
}

template<size_t unitSize>
inline void  MemoryBlock<unitSize>::operator delete(void *ptr)
{
    ::operator delete(ptr);
}

#endif //MEMORYPOOL_MEMORYPOOL_H
