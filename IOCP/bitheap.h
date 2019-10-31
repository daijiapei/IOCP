
#ifndef __BIT_HEAP_H__
#define __BIT_HEAP_H__

//�Ա���������Ĵ�С
typedef int(__stdcall *PFNBITHEAPCMPOBJECT)(void * pObject1, void * pObject2);

//֪ͨ��������С���ѵ�λ�øı���
typedef void(__stdcall *PFNBITHEAPUPDATEINDEX)(void * pObject, unsigned int uIndex);


typedef struct __HEAPELEMENT
{
    void * pObject;
}HEAPELEMENT;

typedef struct __BITHEAP
{
    unsigned int uIncrement;//�ռ䲻��ʱ,ÿ�ε������ٿռ�
    unsigned int uCount;//Ԫ������
    HEAPELEMENT * lpArrayElement;
    PFNBITHEAPCMPOBJECT pfnCompare;
    PFNBITHEAPUPDATEINDEX pfnUpdate;
}BITHEAP;

//������㷨��С���Ѳ���

#ifdef __cplusplus
extern "C" {
#endif

    __inline unsigned int bit_heap_get_count(BITHEAP * pBitHeap)
    {
        return pBitHeap->uCount;
    }

    BITHEAP * bit_heap_new(unsigned int uIncrement, PFNBITHEAPCMPOBJECT pfnCompare, PFNBITHEAPUPDATEINDEX pfnUpdate);
    void bit_heap_free(BITHEAP * pBitHeap);
    void bit_heap_clear(BITHEAP * pBitHeap);
    int bit_heap_top(BITHEAP * pBitHeap, HEAPELEMENT * pElem);
    int bit_heap_push(BITHEAP * pBitHeap, const HEAPELEMENT * pElem);
    int bit_heap_erase(BITHEAP * pBitHeap, unsigned int uIndex);
    int bit_heap_pop(BITHEAP * pBitHeap, HEAPELEMENT * pElem);

#ifdef __cplusplus
};
#endif


#endif