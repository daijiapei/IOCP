
#include "channel.h"

//对比两个对象的大小

IOCHANNEL * channel_new()
{
    IOCHANNEL * pChannel = IOCP_MALLOC(sizeof(IOCHANNEL));
    if (pChannel)
    {
        memset(pChannel, 0, sizeof(IOCHANNEL));
        pChannel->uBitHeapIndex = -1;
    }
    return pChannel;
}

void channel_free(IOCHANNEL * pChannel)
{
    IOCP_FREE(pChannel);
}

int __stdcall channel_compare_object(void * pObject1, void * pObject2)
{
    IOCHANNEL * pChannel1 = pObject1, * pChannel2 = pObject2;

    if (pChannel1->tiAbsoluteTimeOut > pChannel2->tiAbsoluteTimeOut)
    {
        return 1;
    }
    else if (pChannel1->tiAbsoluteTimeOut < pChannel2->tiAbsoluteTimeOut)
    {
        return -1;
    }

    return 0;
}

//通知对象，他在小根堆的位置改变了
void __stdcall channel_update_index(void * pObject, unsigned int uIndex)
{
    IOCHANNEL * pChannel = pObject;
    pChannel->uBitHeapIndex = uIndex;
}


BOOL channel_check_timeout(IOCHANNEL * pChannel, time_t tiCurrent)
{
    return tiCurrent >= pChannel->tiAbsoluteTimeOut;
}

DWORD channel_k2u_error(IOCHANNEL * pChannel)
{
    DWORD dwError = 0;
#define STATUS_CANCELLED          0xC0000120
    //将系统的内核错误转为用户态错误, 方便用户识别
    switch (pChannel->Event.pending.Internal)
    {
    case STATUS_CANCELLED:
    {
        //操作被用户取消
        dwError = ERROR_CANCELLED;
    }break;
    default:
        break;
    }
    pChannel->Event.pending.Internal = dwError;

    return dwError;
}

void channel_set_timeoutcb(IOCHANNEL * pChannel, PFN_TIMERPROC pfnTimerProc)
{
    pChannel->pfnTimerProc = pfnTimerProc;
}

void channel_set_timeout(IOCHANNEL * pChannel, DWORD dwTimeOut)
{
    IOCP_LOCK(pChannel->Event.hObject->hIocp);
    pChannel->dwTimeOut = dwTimeOut;
    IOCP_UNLOCK(pChannel->Event.hObject->hIocp);
}

DWORD channel_get_timeout(IOCHANNEL * pChannel)
{
    return pChannel->dwTimeOut;
}

static BOOL channel_timer_reset_nolock(HIOCPBASE hIocp, IOCHANNEL * pChannel)
{
    BOOL bRet = FALSE;

    if (0 == bit_heap_push(hIocp, pChannel))
    {
        bRet = TRUE;
    }

    return bRet;
}

BOOL channel_timer_add(HIOCPBASE hIocp, IOCHANNEL * pChannel, DWORD dwTimeOut)
{
    BOOL bRet = FALSE;
    time_t tiCurrent = iocp_get_time();
    if (0 == dwTimeOut || dwTimeOut > MAXINT) return FALSE;

    IOCP_LOCK(hIocp);
    
    if (ULONG_MAX != pChannel->uBitHeapIndex)
    {
        bit_heap_erase(hIocp->pTimerHeap, pChannel->uBitHeapIndex);
    }

    pChannel->dwTimeOut = dwTimeOut;
    pChannel->tiAbsoluteTimeOut = tiCurrent + dwTimeOut;
    bRet = channel_timer_reset_nolock(hIocp, pChannel);

    IOCP_UNLOCK(hIocp);

    if (bRet && 0 == pChannel->uBitHeapIndex)
    {
        bRet = iocp_base_active(hIocp);
    }
    return bRet;
}

BOOL channel_timer_del(HIOCPBASE hIocp, IOCHANNEL * pChannel)
{
    IOCP_LOCK(hIocp);

    if (ULONG_MAX != pChannel->uBitHeapIndex)
    {
        bit_heap_erase(hIocp->pTimerHeap, pChannel->uBitHeapIndex);

        if (IOEVENT_TIMEOUT == pChannel->Event.dwEvent && LIST_ELEMENT_VALID(pChannel, ActiveList))
        {
            LIST_REMOVE(pChannel, ActiveList);
        }
    }

    IOCP_UNLOCK(hIocp);

    return TRUE;
}

BOOL channel_timer_reset(HIOCPBASE hIocp, IOCHANNEL * pChannel)
{
    BOOL bRet = TRUE;

    if (0 == pChannel->dwTimeOut) return TRUE;
    IOCP_LOCK(hIocp);

    pChannel->tiAbsoluteTimeOut = hIocp->tiCache + pChannel->dwTimeOut;

    if (pChannel->dwTimeOut && ULONG_MAX == pChannel->uBitHeapIndex)
    {
        bRet = channel_timer_reset_nolock(hIocp->pTimerHeap, pChannel);
    }

    IOCP_UNLOCK(hIocp);
    return bRet;
}