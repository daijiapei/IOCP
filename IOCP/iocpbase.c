
#include <WinSock2.h>
#include "iocpbase.h"
#include <mswsock.h>
#include <assert.h>
#include <process.h>
#include "fileio.h"
#include "channel.h"

static void iocp_event_handle(HIOCPBASE hIocp, OVERLAPPED_ENTRY * pOverLappedEntry, ULONG uNumEntriesRemoved);

HIOCPBASE iocp_base_new()
{
    BOOL bOK = FALSE;
    HIOCPBASE hIocp = NULL;
    DWORD dwBytes = 0;

    do
    {
        hIocp = (HIOCPBASE)IOCP_MALLOC(sizeof(struct __IOCPBASE));
        if (NULL == hIocp) break;

        memset(hIocp, 0, sizeof(struct __IOCPBASE));
        hIocp->dwState = IOCP_STATE_NONE;

        hIocp->ioeCustom.dwEvent = IOEVENT_CUSTOM;
        hIocp->ioeActive.dwEvent = IOEVENT_ACTIVE;

        if (NULL == (hIocp->hFile = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)))
        {
            break;
        }

        hIocp->pLock = NULL;
        InitializeCriticalSection(&hIocp->CriticalSection);

        if (NULL == (hIocp->pTimerHeap = bit_heap_new(1024, channel_compare_object, channel_update_index)))
        {
            break;
        }

        if (NULL == (hIocp->hSuspendEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) break;
        hIocp->dwSuspendMillisecond = 0;

        hIocp->dwOverLappedCount = OVERLAPPED_COUNT;
        hIocp->dwPriorityCount = 1;

        bOK = TRUE;
    } while (FALSE);

    if (FALSE == bOK && hIocp)
    {
        iocp_base_free(hIocp);
        hIocp = NULL;
    }

    return hIocp;
}

void iocp_base_free(HIOCPBASE hIocp)
{
    if (hIocp->hThread)
    {
        if (hIocp->dwThreadId != GetCurrentThreadId())
        {
            WaitForSingleObject(hIocp->hThread, INFINITE);
        }
        
        CloseHandle(hIocp->hThread);
    }

    if (hIocp->PriorityListHead)
    {
        IOCP_FREE(hIocp->PriorityListHead);
    }

    if (hIocp->hFile)
    {
        CloseHandle(hIocp->hFile);
    }

    DeleteCriticalSection(&hIocp->CriticalSection);

    if (hIocp->pTimerHeap)
    {
        bit_heap_free(hIocp->pTimerHeap);
    }

    if (hIocp->hSuspendEvent)
    {
        CloseHandle(hIocp->hSuspendEvent);
    }

    IOCP_FREE(hIocp);
}

static DWORD iocp_base_first_timeout(HIOCPBASE hIocp)
{
    //取得第一个需要超时的时间
    DWORD dwTimeOut = INFINITE;
    IOCHANNEL * pChannel = NULL;
    time_t tiCurrent = iocp_get_time();

    if (0 == bit_heap_top(hIocp->pTimerHeap, (HEAPELEMENT*)&pChannel))
    {
        dwTimeOut = pChannel->tiAbsoluteTimeOut > tiCurrent ? min(pChannel->tiAbsoluteTimeOut - tiCurrent, MAXDWORD -1) :0;
    }

    return dwTimeOut;
}

static void iocp_base_timer_process(HIOCPBASE hIocp)
{
    time_t tiVal = iocp_get_time();
    IOCHANNEL * pChannel = NULL;
    HIOCPFILE hObject = NULL;

    while (0 == bit_heap_top(hIocp->pTimerHeap, &pChannel))
    {
        if (FALSE == channel_check_timeout(pChannel, tiVal))
        {
            //已经没有超时事件了
            break;
        }

        //超时已触发,剔除这个超时对象
        bit_heap_erase(hIocp->pTimerHeap, pChannel->uBitHeapIndex);

        pChannel->dwEvent = IOEVENT_TIMEOUT;//设置为超时通知,并且加入对应的激活队列
        LIST_INSERT_HEAD(&hIocp->PriorityListHead[pChannel->dwPriorityLevel], pChannel, ActiveList);

        if (IOEVENT_TIMEOUT != pChannel->Event.dwEvent)
        {
            //这不是定时器, 而是来源于其他事件的超时,需要取消其操作
            hObject = pChannel->Event.hObject;

            if (IOEVENT_CLOSE == pChannel->Event.dwEvent)
            {
                //延时关闭,将IOEVENT_TIMEOUT类型改为IOEVENT_CLOSE
                pChannel->dwEvent = IOEVENT_CLOSE;
                continue;
            }

            if (IOEVENT_ACCEPT == pChannel->Event.dwEvent)
            {
                //IOEVENT_ACCEPT 事件依赖于listen套接字, 所以要依靠hParent(即listen套接字)
                //来取消对应的重叠操作
                hObject = pChannel->Event.hObject->hParent;
            }
            
            //环境都准备好了, 开始取消
            if (!INVALID_HANDLE(hObject) && TRUE == CancelIoEx(hObject->hFile, &pChannel->Event.pending))
            {
                LIST_REMOVE(pChannel, ActiveList);
                pChannel->bTimeOut = TRUE;
            }
        }
    }//while
}

static void iocp_base_object_process(HIOCPBASE hIocp, OVERLAPPED_ENTRY * pOverLappedEntry, ULONG uNumEntriesRemoved)
{
    DWORD dwEvent = 0;
    HIOCPFILE hObject = NULL;
    HIOEVENT hEvent = NULL;
    IOCHANNEL * pChannel = NULL;
    PFN_CUSTOMEVENTPROC pfnCustomEventCall = hIocp->pfnCustomEventCall;

    //如果检测到iocp自定义通知, 会优先执行, 如果超时事件和读写事件同时完成
    //那么超时事件将会改成读写事件, 
    for (ULONG i = 0; uNumEntriesRemoved > i; i++)
    {
        hObject = (HIOCPFILE)pOverLappedEntry[i].lpCompletionKey;
        pChannel = hEvent = (HIOEVENT)pOverLappedEntry[i].lpOverlapped;

        if (hEvent == &hIocp->ioeActive)
        {
            //我就激活一下，什么也不干
        }
        else if (hEvent == &hIocp->ioeCustom)
        {
            IOCP_UNLOCK(hIocp);
            if (pfnCustomEventCall)
            {
                //这个时候dwBytes 作为event参数, hSocket 作为arg参数
                pfnCustomEventCall(hIocp, pOverLappedEntry[i].dwNumberOfBytesTransferred, hObject);
            }
            IOCP_LOCK(hIocp);
        }
        else
        {
            if (ULONG_MAX != pChannel->uBitHeapIndex)
            {
                //事件已经发生,但是定时器还未触发, 那么就要取消定时器
                bit_heap_erase(hIocp->pTimerHeap, pChannel->uBitHeapIndex);
            }

            if (ERROR_CANCELLED == channel_k2u_error(pChannel) && pChannel->bTimeOut)
            {
                //这是一个被定时器取消的超时事件
                pChannel->bTimeOut = FALSE;
                pChannel->dwEvent = IOEVENT_TIMEOUT;
            }
            else
            {
                //这是一个普通的事件
                //可能超时和IO异步同时产生, pChannel->dwEvent会变成IOEVENT_TIMEOUT,所以此处重新赋值一次
                pChannel->dwEvent = pChannel->Event.dwEvent;
            }
            
            if (!LIST_ELEMENT_VALID(pChannel, ActiveList))
            {
                //还没加入激活队列,此处加入
                LIST_INSERT_HEAD(&hIocp->PriorityListHead[pChannel->dwPriorityLevel], pChannel, ActiveList);
            }
        }
    }//for
}

static void iocp_base_active_channel(HIOCPBASE hIocp)
{
    LIST_HEAD(PRIORITYLISTHEAD, __IOCPFILE) * pPriorityListHead = NULL;

    IOCHANNEL * pChannel = NULL;

    for (ULONG u = 0; hIocp->dwPriorityCount > u; u++)
    {
        pPriorityListHead = &hIocp->PriorityListHead[u];
        
        while (IOCP_STATE_RUN == hIocp->dwState)
        {
            if (pChannel = pPriorityListHead->First)
            {
                LIST_REMOVE(pChannel, ActiveList);
            }
            
            if (pChannel)
            {
                IOCP_UNLOCK(hIocp);
                pChannel->Event.hObject->pfnIoCall(pChannel->Event.hObject, pChannel);
                IOCP_LOCK(hIocp);
            }
            else
            {
                break;
            }

        }//while
    }//for
}


void iocp_base_set_custom_cb(HIOCPBASE hIocp, PFN_CUSTOMEVENTPROC pfnCustomEventCall)
{
    hIocp->pfnCustomEventCall = pfnCustomEventCall;
}

//post 一个自定义时间
BOOL iocp_base_custom_event_call(HIOCPBASE hIocp, DWORD dwEvent, void * arg)
{
    if (NULL == hIocp || NULL == hIocp->pfnCustomEventCall) return FALSE;

    return PostQueuedCompletionStatus(hIocp->hFile, dwEvent, (ULONG_PTR)arg, &hIocp->ioeCustom.pending);
}


BOOL iocp_base_active(HIOCPBASE hIocp)
{
    if (hIocp->dwThreadId != GetCurrentThreadId())//等待中
    {
        return PostQueuedCompletionStatus(hIocp->hFile, 0, (ULONG_PTR)NULL, &hIocp->ioeActive.pending);
    }
    return TRUE;
}

unsigned int __stdcall iocp_base_dispatch(HIOCPBASE hIocp)
{
    BOOL bOK = FALSE;
    DWORD dwError = 0;
    DWORD dwTimeOut = INFINITE;
    HIOCPFILE hObject = NULL;
    BOOL bAlertable = FALSE;
    ULONG uNumEntriesRemoved = 0;
    OVERLAPPED_ENTRY *pOverLappedEntry = NULL;
    DWORD dwOverLappedCount = hIocp->dwOverLappedCount;
    if (!hIocp->hFile)
    {
        assert(hIocp->hFile && "iocp_base_dispatch 无效的IOCP句柄");
        return IOCP_STATE_RUN;//如果进入了loop , 是不可能返回这个值的, 所以这个值代表错误
    }

    if (IOCP_STATE_RUN == hIocp->dwState)
    {
        assert(0 && "iocp_base_dispatch loop已经在运行中");
        return IOCP_STATE_RUN;
    }

    if (0 == hIocp->dwPriorityCount)
    {
        assert(0 && "iocp_base_dispatch 无效的等级数量");
        return IOCP_STATE_RUN;
    }

    hIocp->PriorityListHead = IOCP_MALLOC(sizeof(hIocp->PriorityListHead) * hIocp->dwPriorityCount);
    if (!hIocp->PriorityListHead)
    {
        assert(hIocp->PriorityListHead && "iocp_base_dispatch 致命的错误");
        return IOCP_STATE_RUN;
    }
    memset(hIocp->PriorityListHead, 0, sizeof(hIocp->PriorityListHead) * hIocp->dwPriorityCount);
    
    pOverLappedEntry = IOCP_MALLOC(sizeof(OVERLAPPED_ENTRY) * dwOverLappedCount);
    if (!pOverLappedEntry)
    {
        assert(pOverLappedEntry && "iocp_base_dispatch 致命的错误");
        return IOCP_STATE_RUN;
    }

    hIocp->dwThreadId = GetCurrentThreadId();
    hIocp->hThread = GetCurrentThread();

    hIocp->dwState = IOCP_STATE_RUN;
    
    IOCP_LOCK(hIocp);

    while (IOCP_STATE_RUN == hIocp->dwState)
    {

#if 0//默认不提供挂起功能
        dwTimeOut = InterlockedExchangeAdd(&hIocp->dwSuspendMillisecond, 0);
        if (dwTimeOut)
        {
            //需要将IOCP挂起
            WaitForSingleObjectEx(hIocp->hSuspendEvent, dwTimeOut, FALSE);
            InterlockedExchange(&hIocp->dwSuspendMillisecond, 0);//挂起结束
            continue;
        }
#endif

        hObject = NULL;
        bOK = FALSE;
        dwError = WAIT_TIMEOUT;
        uNumEntriesRemoved = 0;

        hIocp->uActive = 0;//进入等待, 没有执行任务, 要放在定时器前面
        dwTimeOut = iocp_base_first_timeout(hIocp);
        bAlertable = hIocp->bAlertable;
        IOCP_UNLOCK(hIocp);
#if 1
        bOK = GetQueuedCompletionStatusEx(hIocp->hFile, pOverLappedEntry, dwOverLappedCount,
            &uNumEntriesRemoved, dwTimeOut, bAlertable);
#else
        uNumEntriesRemoved = 1;

        bOK = GetQueuedCompletionStatus(hIocp->fd, &pOverLappedEntry->dwNumberOfBytesTransferred,
            &pOverLappedEntry->lpCompletionKey, &pOverLappedEntry->lpOverlapped, dwTimeOut);

#endif
        IOCP_LOCK(hIocp);
        hIocp->uActive = 1;//被激活, 开始执行任务
        //将超时事件加入激活队列
        iocp_base_timer_process(hIocp);

        //将读写等, 事件加入激活队列
        iocp_base_object_process(hIocp, pOverLappedEntry, uNumEntriesRemoved);

        //按优先级, 轮流执行激活的事件
        iocp_base_active_channel(hIocp);
    }/*hIocp->dwState 循环结束*/
    
    IOCP_UNLOCK(hIocp);
    IOCP_FREE(pOverLappedEntry);
    SetLastError(0);
    hIocp->uActive = 0;//退出
    return hIocp->dwState;
}


//Get Set
DWORD iocp_base_set_state(HIOCPBASE hIocp, DWORD dwState)
{
    DWORD dwOldState = hIocp->dwState;
    hIocp->dwState = dwState;
    return dwOldState;
}

void iocp_base_loopexit(HIOCPBASE hIocp)//正常退出
{
    iocp_base_set_state(hIocp, IOCP_STATE_QUIT);
}

DWORD iocp_base_get_threadid(HIOCPBASE hIocp)
{
    return hIocp->dwThreadId;
}

void iocp_base_set_priority_count(HIOCPBASE hIocp, DWORD dwCount)
{
    hIocp->dwPriorityCount = min(1, dwCount);
}

DWORD iocp_base_get_priority_count(HIOCPBASE hIocp)
{
    return hIocp->dwPriorityCount;
}

void iocp_base_set_userdata(HIOCPBASE hIocp, void * pUserData)
{
    hIocp->pUserData = pUserData;
}

void * iocp_base_get_userdata(HIOCPBASE hIocp)
{
    return hIocp->pUserData;
}

void iocp_base_enable_lock(HIOCPBASE hIocp)
{
    hIocp->bLock = TRUE;
    hIocp->pLock = &hIocp->CriticalSection;
}

void iocp_base_disable_lock(HIOCPBASE hIocp)
{
    //这个函数不应该被调用, 应该默认就是不使用锁的
    hIocp->bLock = FALSE;
    hIocp->pLock = NULL;
}

BOOL iocp_base_is_enble_lock(HIOCPBASE hIocp)
{
    return hIocp->bLock;
}

void iocp_base_enable_apc(HIOCPBASE hIocp)
{
    IOCP_LOCK(hIocp);
    hIocp->bAlertable = TRUE;
    IOCP_UNLOCK(hIocp);
}

void iocp_base_disable_apc(HIOCPBASE hIocp)
{
    hIocp->bAlertable = FALSE;
}

BOOL iocp_base_is_enble_apc(HIOCPBASE hIocp)
{
    return hIocp->bAlertable;
}

DWORD iocp_base_get_state(HIOCPBASE hIocp)
{
    return hIocp->dwState;
}

BOOL iocp_base_is_suspend(HIOCPBASE hIocp)
{
    if (0 == InterlockedExchangeAdd(&hIocp->dwSuspendMillisecond, 0))
    {
        //0 == hIocp->dwSuspendMillisecond 代表没有挂起
        return FALSE;
    }
    return TRUE;
}
//将iocpbase对象挂起
BOOL iocp_base_suspend(HIOCPBASE hIocp, DWORD dwMilliSecond)
{
    BOOL bRet = FALSE;
     
    if (0 == InterlockedCompareExchange(&hIocp->dwSuspendMillisecond, dwMilliSecond, 0))
    {
        //没有挂起就该为挂起
        bRet = TRUE;
    }

    return bRet;
}

BOOL iocp_base_resume(HIOCPBASE hIocp)
{
    if (0 != InterlockedExchangeAdd(&hIocp->dwSuspendMillisecond, 0))
    {
        //0 == hIocp->dwSuspendSecond 代表没有挂起
        SetEvent(hIocp->hSuspendEvent);
        return TRUE;
    }
    return TRUE;
}