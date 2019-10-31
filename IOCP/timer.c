
#include "timer.h"
#include "fileio.h"
#include "channel.h"


#define iocp_timer_increment    iocp_file_increment
#define iocp_timer_decrement    iocp_file_decrement


static void iocp_timer_iocall(HIOCPFILE hSocket, IOCHANNEL * pChannel);

HIOCPFILE iocp_timer_new(HIOCPBASE hIocp, DWORD dwFlag)
{
    BOOL bOK = FALSE;
    HIOCPFILE hTimer = NULL;

    long lMode = 1;
    if (NULL == hIocp) return NULL;

    do
    {
        hTimer = (HIOCPFILE)IOCP_MALLOC(sizeof(struct __IOCPFILE));

        if (NULL == hTimer) break;
        memset(hTimer, 0, sizeof(struct __IOCPFILE));

        hTimer->dwObject = IOCP_OBJECT_TIMER;
        hTimer->pfnIoCall = (PFN_IOCPOBJECTIOPROC)iocp_timer_iocall;
        hTimer->hFile = INVALID_HANDLE_VALUE;

        hTimer->dwFlag = dwFlag;

        if (iocp_base_is_enble_lock(hIocp))
        {
            InitializeCriticalSection(&hTimer->CriticalSection);
            hTimer->pLock = &hTimer->CriticalSection;
        }

        if (NULL == (hTimer->pTimerChannel = IOCP_MALLOC(sizeof(IOCHANNEL))))
        {
            break;
        }

        if (NULL == (hTimer->pCloseChannel = IOCP_MALLOC(sizeof(IOCHANNEL))))
        {
            break;
        }

        memset(hTimer->pTimerChannel, 0, sizeof(IOCHANNEL));
        memset(hTimer->pCloseChannel, 0, sizeof(IOCHANNEL));

        hTimer->pTimerChannel->Event.hObject = hTimer;
        hTimer->pTimerChannel->Event.dwEvent = IOEVENT_TIMEOUT;
        hTimer->pTimerChannel->uBitHeapIndex = -1;

        hTimer->pCloseChannel->Event.hObject = hTimer;
        hTimer->pCloseChannel->Event.dwEvent = IOEVENT_CLOSE;
        hTimer->pCloseChannel->uBitHeapIndex = -1;

        hTimer->hIocp = hIocp;
        bOK = TRUE;
    } while (FALSE);

    if (FALSE == bOK && hTimer)
    {
        hTimer = NULL;
    }
    hTimer->nRecursionCount = 1;
    return hTimer;
}

static void iocp_timer_iocall(HIOCPFILE hTimer, IOCHANNEL * pChannel)
{
    BOOL bRet = TRUE;
    HIOCPBASE hIocp = hTimer->hIocp;
    PFN_TIMERPROC pfnTimerProc = pChannel->pfnTimerProc;
    PFN_SIGNALPROC pfnSignalProc = NULL;
    switch (pChannel->dwEvent)
    {
    case IOEVENT_TIMEOUT:
    {
        if (pfnTimerProc)
        {
            pfnTimerProc(hTimer, pChannel, hTimer);
        }

        IOCP_LOCK(hIocp);

        if (pChannel->dwTimeOut && MAXDWORD > pChannel->dwTimeOut
            && IOCPFILE_FLAG_KEEPTIMER & hTimer->dwFlag)
        {
            if (ULONG_MAX == pChannel->uBitHeapIndex)
            {
                pChannel->tiAbsoluteTimeOut = hIocp->tiCache + pChannel->dwTimeOut;
                if (bit_heap_push(hIocp, pChannel))
                {
                    //发生了错误, 赋值
                    pfnSignalProc = hTimer->pfnSignalProc;
                }
            }
        }
        IOCP_UNLOCK(hIocp);

        if (pfnSignalProc)
        {
            //一定是因为内存不足
            pfnSignalProc(hTimer, pChannel, ERROR_NOT_ENOUGH_MEMORY);
        }
    }break;
    case IOEVENT_CLOSE:
    {
        iocp_timer_del(hTimer);
        iocp_timer_decrement(hTimer);
    }break;
    default:
        break;
    }
    return;
}

void iocp_timer_set_signalcb(HIOCPFILE hTimer, PFN_SIGNALPROC pfnSignalProc)
{
    hTimer->pfnSignalProc = pfnSignalProc;
}

BOOL iocp_timer_add(HIOCPFILE hTimer, DWORD dwTimeOut, PFN_TIMERPROC pfnTimerProc)
{
    BOOL bRet = FALSE;
    HIOCPBASE hIocp = hTimer->hIocp;
    IOCHANNEL * pChannel = hTimer->pTimerChannel;
    if (0 == dwTimeOut || MAXDWORD == dwTimeOut) return FALSE;

    IOCP_LOCK(hIocp);

    if (ULONG_MAX != pChannel->uBitHeapIndex)
    {
        bit_heap_erase(hIocp->pTimerHeap, pChannel->uBitHeapIndex);
    }
    
    pChannel->dwTimeOut = dwTimeOut;
    pChannel->tiAbsoluteTimeOut = iocp_get_time() + dwTimeOut;
    pChannel->pfnTimerProc = pfnTimerProc;
    if (0 == bit_heap_push(hIocp, pChannel))
    {
        bRet = TRUE;
    }

    IOCP_UNLOCK(hIocp);

    if (bRet && 0 == pChannel->uBitHeapIndex)
    {
        bRet = iocp_base_active(hIocp);
    }
    return bRet;
}

BOOL iocp_timer_del(HIOCPFILE hTimer)
{
    HIOCPBASE hIocp = hTimer->hIocp;
    IOCHANNEL * pChannel = hTimer->pTimerChannel;
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

BOOL iocp_timer_close(HIOCPFILE hTimer)
{
    return PostQueuedCompletionStatus(hTimer->hIocp, 0,
        (ULONG_PTR)hTimer, &hTimer->pCloseChannel->Event.pending);
}

