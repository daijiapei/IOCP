
#include "bitheap.h"
#include "iocpdef.h"

#define BHMALLOC   IOCP_MALLOC
#define BHREALLOC  IOCP_REALLOC
#define BHFREE     IOCP_FREE


BITHEAP * bit_heap_new(unsigned int uIncrement, PFNBITHEAPCMPOBJECT pfnCompare, PFNBITHEAPUPDATEINDEX pfnUpdate)
{
    if (0 == uIncrement || 0 == pfnUpdate || 0 == pfnUpdate) return NULL;
    BITHEAP * pBitHeap = (BITHEAP *)BHMALLOC(sizeof(BITHEAP));
    memset(pBitHeap, 0, sizeof(BITHEAP));
    pBitHeap->uIncrement = uIncrement;
    pBitHeap->pfnCompare = pfnCompare;
    pBitHeap->pfnUpdate = pfnUpdate;
    pBitHeap->lpArrayElement = (HEAPELEMENT *)BHMALLOC(sizeof(HEAPELEMENT) * uIncrement);

    if (NULL == pBitHeap->lpArrayElement)
    {
        BHFREE(pBitHeap);
        pBitHeap = NULL;
    }
    return pBitHeap;
}

void bit_heap_free(BITHEAP * pBitHeap)
{
    if (pBitHeap->lpArrayElement)
    {
        BHFREE(pBitHeap->lpArrayElement);
    }
    BHFREE(pBitHeap);
}

void bit_heap_clear(BITHEAP * pBitHeap)
{
    if (pBitHeap->uCount > pBitHeap->uIncrement)
    {
        //需要缩小
        //由于是缩小, 这里是一定成功的
        pBitHeap->lpArrayElement = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, pBitHeap->uIncrement * sizeof(HEAPELEMENT));
        assert(pBitHeap->lpArrayElement && "bit_heap_clear 发生不可预料的情况!");
    }

    for (unsigned int i = 0; pBitHeap->uCount > i; i++)
    {
        pBitHeap->pfnUpdate(pBitHeap->lpArrayElement[i].pObject, -1);
    }

    pBitHeap->uCount = 0;
    memset(pBitHeap->lpArrayElement, 0, pBitHeap->uIncrement * sizeof(HEAPELEMENT));
}

int bit_heap_top(BITHEAP * pBitHeap, HEAPELEMENT * pElem)
{
    if (NULL == pBitHeap || NULL == pElem)
    {
        assert(0 && "bit_heap_top 传入无效的指针, 请检查!");
        return -1;
    }

    if (0 == pBitHeap->uCount) return -1;

    *pElem = pBitHeap->lpArrayElement[0];

    return 0;
}

int bit_heap_pop(BITHEAP * pBitHeap, HEAPELEMENT * pElem)
{
    if (0 == bit_heap_top(pBitHeap, pElem))
    {
        return bit_heap_erase(pBitHeap, 0);
    }

    return -1;
}

int bit_heap_push(BITHEAP * pBitHeap, const HEAPELEMENT * pElem)
{
    unsigned int uBytes = 0;
    HEAPELEMENT * lpArray = NULL;
    unsigned int uIndex = 0;
    unsigned int j = 0;
    if (NULL == pBitHeap || NULL == pElem)
    {
        assert(0 && "bit_heap_push 传入无效的指针, 请检查!");
        return -1;
    }

    if (pBitHeap->uCount > 0 && 0 == pBitHeap->uCount % pBitHeap->uIncrement)
    {
        //内存刚好够用, 现在还要插入一个, 那就需要重新分配空间
        uBytes = sizeof(HEAPELEMENT) * (pBitHeap->uCount + pBitHeap->uIncrement);
        if (NULL == (lpArray = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, uBytes)))
        {
            return -1;
        }

        pBitHeap->lpArrayElement = lpArray;
        memset(pBitHeap->lpArrayElement + pBitHeap->uCount, 0, sizeof(HEAPELEMENT) * pBitHeap->uIncrement);
    }

    //内存问题已处理好, 下面开始正式插入

    lpArray = pBitHeap->lpArrayElement;
    uIndex = pBitHeap->uCount;//nIndex指向待调整元素的位置，即其数组下标，初始指向新元素所在的堆尾位置

    lpArray[pBitHeap->uCount++] = *pElem;//向堆尾添加新元素, 并增加数组长度

    while (0 != uIndex)
    {
        j = (uIndex - 1) / 2; //j指向下标为uIndex的元素的双亲
        //if (pElem->dwValue >= lpArray[j].dwValue) //若新元素大于待调整元素的双亲，则比较调整结束，退出循环
        if (pBitHeap->pfnCompare(pElem->pObject, lpArray[j].pObject) >= 0)
            break;

        lpArray[uIndex] = lpArray[j]; //将双亲元素下移到待调整元素的位置

        pBitHeap->pfnUpdate(lpArray[uIndex].pObject, uIndex);

        uIndex = j; //使待调整位置变为其双亲位置，进行下一次循环
    }
    lpArray[uIndex] = *pElem;//把新元素调整到最终位置
    pBitHeap->pfnUpdate(pElem->pObject, uIndex);
    return 0;
}

int bit_heap_erase(BITHEAP * pBitHeap, unsigned int uIndex)
{
    unsigned int uBytes = 0;
    HEAPELEMENT * lpArray = NULL;
    //unsigned long uIndex = 0;//用nIndex指向待调整元素的位置，初始指向堆顶位置
    unsigned long j = 0;//j是默认指向左边, 如果左边比右边大,那么指向右边
    HEAPELEMENT Element;
    BOOL bReAlloc = FALSE;

    assert(pBitHeap && "bit_heap_erase 传入无效的指针, 请检查!");

    if (uIndex > pBitHeap->uCount) return -1;

    lpArray = pBitHeap->lpArrayElement;

    pBitHeap->pfnUpdate(&lpArray[uIndex], -1);//已经被移除

    if (0 == --pBitHeap->uCount) return 0;

    Element = lpArray[pBitHeap->uCount]; //将待调整的原堆尾元素暂存Element中，以便放入最终位置
    j = 2 * uIndex + 1;//用j指向uIndex的左孩子位置，初始指向下标为1的位置

    while (pBitHeap->uCount - 1 >= j)//寻找待调整元素的最终位置，每次使孩子元素上移一层，调整到孩子为空时止
    {
        //若存在右孩子且较小，使j指向右孩子
        //if (pMinHeap->dwCount - 1 > j && lpArray[j].dwValue > lpArray[j + 1].dwValue)//左比右大

        if (pBitHeap->uCount - 1 > j && pBitHeap->pfnCompare(lpArray[j].pObject, lpArray[j + 1].pObject) > 0)//左比右大
            j++;//指向右边, 指向小的位置

        //if (lpArray[j].dwValue >= Element.dwValue) 
        if (pBitHeap->pfnCompare(lpArray[j].pObject, Element.pObject) >= 0)//若Element比其较小的孩子还小，则调整结束，退出循环
            break;

        lpArray[uIndex] = lpArray[j];//否则，将孩子元素移到双亲位置
        pBitHeap->pfnUpdate(lpArray[uIndex].pObject, uIndex);

        uIndex = j; //将待调整位置变为其较小的孩子位置
        j = 2 * uIndex + 1;//将j变为新的待调整位置的左孩子位置，继续下一次循环
    }

    lpArray[uIndex] = Element;
    pBitHeap->pfnUpdate(Element.pObject, uIndex);

    if (pBitHeap->uCount >= pBitHeap->uIncrement && 0 == pBitHeap->uCount % pBitHeap->uIncrement)
    {
        //此时内存刚好多出一个, 现在还移除一个, 那就重新分配空间, 将空间压缩到最小
        uBytes = sizeof(HEAPELEMENT) * (pBitHeap->uCount);

        //因为是缩小空间, 所以realloc是不会失败的
        pBitHeap->lpArrayElement = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, uBytes);

        assert(lpArray == pBitHeap->lpArrayElement && "bit_heap_erase 发生不可预料的情况!");
    }
    return 0;
}
