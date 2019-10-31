
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
        //��Ҫ��С
        //��������С, ������һ���ɹ���
        pBitHeap->lpArrayElement = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, pBitHeap->uIncrement * sizeof(HEAPELEMENT));
        assert(pBitHeap->lpArrayElement && "bit_heap_clear ��������Ԥ�ϵ����!");
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
        assert(0 && "bit_heap_top ������Ч��ָ��, ����!");
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
        assert(0 && "bit_heap_push ������Ч��ָ��, ����!");
        return -1;
    }

    if (pBitHeap->uCount > 0 && 0 == pBitHeap->uCount % pBitHeap->uIncrement)
    {
        //�ڴ�պù���, ���ڻ�Ҫ����һ��, �Ǿ���Ҫ���·���ռ�
        uBytes = sizeof(HEAPELEMENT) * (pBitHeap->uCount + pBitHeap->uIncrement);
        if (NULL == (lpArray = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, uBytes)))
        {
            return -1;
        }

        pBitHeap->lpArrayElement = lpArray;
        memset(pBitHeap->lpArrayElement + pBitHeap->uCount, 0, sizeof(HEAPELEMENT) * pBitHeap->uIncrement);
    }

    //�ڴ������Ѵ����, ���濪ʼ��ʽ����

    lpArray = pBitHeap->lpArrayElement;
    uIndex = pBitHeap->uCount;//nIndexָ�������Ԫ�ص�λ�ã����������±꣬��ʼָ����Ԫ�����ڵĶ�βλ��

    lpArray[pBitHeap->uCount++] = *pElem;//���β�����Ԫ��, ���������鳤��

    while (0 != uIndex)
    {
        j = (uIndex - 1) / 2; //jָ���±�ΪuIndex��Ԫ�ص�˫��
        //if (pElem->dwValue >= lpArray[j].dwValue) //����Ԫ�ش��ڴ�����Ԫ�ص�˫�ף���Ƚϵ����������˳�ѭ��
        if (pBitHeap->pfnCompare(pElem->pObject, lpArray[j].pObject) >= 0)
            break;

        lpArray[uIndex] = lpArray[j]; //��˫��Ԫ�����Ƶ�������Ԫ�ص�λ��

        pBitHeap->pfnUpdate(lpArray[uIndex].pObject, uIndex);

        uIndex = j; //ʹ������λ�ñ�Ϊ��˫��λ�ã�������һ��ѭ��
    }
    lpArray[uIndex] = *pElem;//����Ԫ�ص���������λ��
    pBitHeap->pfnUpdate(pElem->pObject, uIndex);
    return 0;
}

int bit_heap_erase(BITHEAP * pBitHeap, unsigned int uIndex)
{
    unsigned int uBytes = 0;
    HEAPELEMENT * lpArray = NULL;
    //unsigned long uIndex = 0;//��nIndexָ�������Ԫ�ص�λ�ã���ʼָ��Ѷ�λ��
    unsigned long j = 0;//j��Ĭ��ָ�����, �����߱��ұߴ�,��ôָ���ұ�
    HEAPELEMENT Element;
    BOOL bReAlloc = FALSE;

    assert(pBitHeap && "bit_heap_erase ������Ч��ָ��, ����!");

    if (uIndex > pBitHeap->uCount) return -1;

    lpArray = pBitHeap->lpArrayElement;

    pBitHeap->pfnUpdate(&lpArray[uIndex], -1);//�Ѿ����Ƴ�

    if (0 == --pBitHeap->uCount) return 0;

    Element = lpArray[pBitHeap->uCount]; //����������ԭ��βԪ���ݴ�Element�У��Ա��������λ��
    j = 2 * uIndex + 1;//��jָ��uIndex������λ�ã���ʼָ���±�Ϊ1��λ��

    while (pBitHeap->uCount - 1 >= j)//Ѱ�Ҵ�����Ԫ�ص�����λ�ã�ÿ��ʹ����Ԫ������һ�㣬����������Ϊ��ʱֹ
    {
        //�������Һ����ҽ�С��ʹjָ���Һ���
        //if (pMinHeap->dwCount - 1 > j && lpArray[j].dwValue > lpArray[j + 1].dwValue)//����Ҵ�

        if (pBitHeap->uCount - 1 > j && pBitHeap->pfnCompare(lpArray[j].pObject, lpArray[j + 1].pObject) > 0)//����Ҵ�
            j++;//ָ���ұ�, ָ��С��λ��

        //if (lpArray[j].dwValue >= Element.dwValue) 
        if (pBitHeap->pfnCompare(lpArray[j].pObject, Element.pObject) >= 0)//��Element�����С�ĺ��ӻ�С��������������˳�ѭ��
            break;

        lpArray[uIndex] = lpArray[j];//���򣬽�����Ԫ���Ƶ�˫��λ��
        pBitHeap->pfnUpdate(lpArray[uIndex].pObject, uIndex);

        uIndex = j; //��������λ�ñ�Ϊ���С�ĺ���λ��
        j = 2 * uIndex + 1;//��j��Ϊ�µĴ�����λ�õ�����λ�ã�������һ��ѭ��
    }

    lpArray[uIndex] = Element;
    pBitHeap->pfnUpdate(Element.pObject, uIndex);

    if (pBitHeap->uCount >= pBitHeap->uIncrement && 0 == pBitHeap->uCount % pBitHeap->uIncrement)
    {
        //��ʱ�ڴ�պö��һ��, ���ڻ��Ƴ�һ��, �Ǿ����·���ռ�, ���ռ�ѹ������С
        uBytes = sizeof(HEAPELEMENT) * (pBitHeap->uCount);

        //��Ϊ����С�ռ�, ����realloc�ǲ���ʧ�ܵ�
        pBitHeap->lpArrayElement = (HEAPELEMENT*)BHREALLOC(pBitHeap->lpArrayElement, uBytes);

        assert(lpArray == pBitHeap->lpArrayElement && "bit_heap_erase ��������Ԥ�ϵ����!");
    }
    return 0;
}
