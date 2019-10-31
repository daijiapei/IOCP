

#include <WinSock2.h>
#include "tcpsocket.h"
#include "socketbuffer.h"
#include "iocpbase.h"
#include "iocpenv.h"
#include "channel.h"
#include <assert.h>

#define ADDROFFSET   64

#define iocp_tcp_increment    iocp_file_increment
#define iocp_tcp_decrement    iocp_file_decrement
#define iocp_tcp_free         iocp_file_free

static void iocp_tcp_iocall(void * object, DWORD dwBytes, HIOEVENT hEvent);
static BOOL iocp_tcp_listen_call(HIOCPFILE hListen, HIOCPFILE hAccept);
static BOOL iocp_tcp_event_call(HIOCPFILE hSocket, IOCHANNEL * pChannel);
static BOOL iocp_tcp_signal_call(HIOCPFILE hSocket, IOCHANNEL * pChannel, DWORD dwError);

static void iocp_tcp_read_event(HIOCPFILE hSocket, IOCHANNEL * pChannel);
static void iocp_tcp_write_event(HIOCPFILE hSocket, IOCHANNEL * pChannel);
static void iocp_tcp_keep_read(HIOCPFILE hSocket);
static void iocp_tcp_keep_write(HIOCPFILE hSocket);
static BOOL iocp_tcp_iorecv_nolock(HIOCPFILE hSocket);
static BOOL iocp_tcp_iosend_nolock(HIOCPFILE hSocket);



//生命周期
HIOCPFILE iocp_tcp_new(HIOCPBASE hIocp, SOCKET hFile, DWORD dwFlag)
{
    BOOL bOK = FALSE;
    HIOCPFILE hSocket = NULL;
    long lMode = 1;
    if (NULL == hIocp) return NULL;

    //因为如果失败，我们是不应该关闭用户的fd的
    if (0 == hFile) hFile = SOCKET_ERROR;

    do
    {
        hSocket = (HIOCPFILE)IOCP_MALLOC(sizeof(struct __IOCPFILE));

        if (NULL == hSocket) break;
        memset(hSocket, 0, sizeof(struct __IOCPFILE));

        hSocket->dwObject = IOCP_OBJECT_TCP;
        hSocket->pfnIoCall = (PFN_IOCPOBJECTIOPROC)iocp_tcp_iocall;
        hSocket->hFile = (HANDLE)hFile;

        if (INVALID_HANDLE_VALUE == hSocket->hFile)
        {
            hSocket->hFile = (HANDLE)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        }
        else
        {
            //原来的socket可能是非阻塞的,设置为阻塞
            if (SOCKET_ERROR == ioctlsocket((SOCKET)hSocket->hFile, FIONBIO, (unsigned long *)&lMode))
            {
                break;
            }
        }

        if (INVALID_HANDLE_VALUE == hSocket->hFile) break;

        hSocket->dwFlag = dwFlag;
        if (0 == hSocket->dwFlag)
        {
            hSocket->dwFlag = IOCPFILE_FLAG_WRITEBUF | IOCPFILE_FLAG_READBUF | IOCPFILE_FLAG_KEEPREAD;
        }

        if (iocp_base_is_enble_lock(hIocp))
        {
            InitializeCriticalSection(&hSocket->CriticalSection);
            hSocket->pLock = &hSocket->CriticalSection;
        }

        if (NULL == (hSocket->pReadChannel = IOCP_MALLOC(sizeof(IOCHANNEL))))
        {
            break;
        }

        if (NULL == (hSocket->pWriteChannel = IOCP_MALLOC(sizeof(IOCHANNEL))))
        {
            break;
        }

        memset(hSocket->pReadChannel, 0, sizeof(IOCHANNEL));
        memset(hSocket->pWriteChannel, 0, sizeof(IOCHANNEL));
        hSocket->pReadChannel->Event.hObject = hSocket;
        hSocket->pWriteChannel->Event.hObject = hSocket;
        hSocket->pReadChannel->uBitHeapIndex = -1;
        hSocket->pWriteChannel->uBitHeapIndex = -1;

        if (IOCPFILE_FLAG_READBUF & hSocket->dwFlag)
        {
            //默认开启
            hSocket->pInput = stream_buffer_new(BUFFERCHAIN_LENGHT);
            if (NULL == hSocket->pInput) break;
        }

        if (IOCPFILE_FLAG_WRITEBUF & hSocket->dwFlag)
        {
            //默认开启
            hSocket->pOutput = stream_buffer_new(BUFFERCHAIN_LENGHT);
            if (NULL == hSocket->pOutput) break;
        }

        if (NULL == (hSocket->pOutpack = cache_new(FILEBUFFER_SIZE))) break;
        if (NULL == (hSocket->pInpack = cache_new(FILEBUFFER_SIZE))) break;

        if (NULL == CreateIoCompletionPort((HANDLE)hSocket->hFile, iocp_base_handle(hIocp), (ULONG_PTR)hSocket, 0))
        {
            //要放到最后，因为绑定后， 就不能解绑了，中途失败了，fd也不能
            //放到别的IOCP中
            break;
        }

        hSocket->hIocp = hIocp;
        bOK = TRUE;
    } while (FALSE);

    if (FALSE == bOK && hSocket)
    {
        iocp_tcp_free(hSocket);
        hSocket = NULL;
    }
    hSocket->nRecursionCount = 1;
    return hSocket;
}


//呼叫
static void iocp_tcp_iocall(HIOCPFILE hSocket, IOCHANNEL * pChannel)
{
    unsigned int uSize = pChannel->Event.pending.InternalHigh;
    unsigned int uBytes = 0;
    time_t tiCurrent = iocp_get_time();//时间缓冲区
    DWORD dwError = pChannel->Event.pending.Internal;

    switch (pChannel->dwEvent)
    {
    case IOEVENT_ACCEPT:
    {
        if (0 == dwError)
        {
            iocp_tcp_listen_call(hSocket->hParent, hSocket);
            iocp_tcp_set_readtime(hSocket, tiCurrent);
            iocp_tcp_set_writetime(hSocket, tiCurrent);
        }
        else
        {
            iocp_tcp_signal_call(hSocket, pChannel, dwError);
        }
        iocp_tcp_decrement(hSocket->hParent);
    }break;
    case IOEVENT_CONNECT:
    {
        if (0 == dwError)
        {
            iocp_tcp_event_call(hSocket, pChannel);
            iocp_tcp_set_readtime(hSocket, tiCurrent);
            iocp_tcp_set_writetime(hSocket, tiCurrent);
        }
        else
        {
            iocp_tcp_signal_call(hSocket, pChannel, dwError);
        }
    }break;
    case IOEVENT_READ:
    {
        iocp_tcp_read_event(hSocket, pChannel);
    }break;
    case IOEVENT_WRITE:
    {
        iocp_tcp_write_event(hSocket, pChannel);
    }break;
    case IOEVENT_CLOSE:
    {
        iocp_tcp_close(hSocket, 0);
    }break;
    case IOEVENT_TIMEOUT:
    {
        IOCP_LOCK(hSocket);

        if (IOEVENT_READ == pChannel->Event.dwEvent)
        {
            hSocket->dwFlag &= (~IOCPFILE_FLAG_ENABLE_READ);
        }
        if (IOEVENT_WRITE == pChannel->Event.dwEvent)
        {
            hSocket->dwFlag &= (~IOCPFILE_FLAG_ENABLE_WRITE);
        }
        PFN_TIMERPROC pfnTimerProc = pChannel->pfnTimerProc;

        CHANNEL_UNLOCK(pChannel);
        if (!INVALID_HANDLE(hSocket) && pfnTimerProc)
        {
            IOCP_UNLOCK(hSocket);
            pfnTimerProc(hSocket, pChannel, hSocket->pUserData);
        }
        else
        {
            IOCP_UNLOCK(hSocket);
        }
    }break;
    default:
    {
        assert(0 && "iocp_tcp_iocall 未知的处理方式");
    }break;
    }

    iocp_tcp_decrement(hSocket);
    return;
}

void iocp_tcp_setcb(HIOCPFILE hSocket, PFN_TCPREADPROC pfnReadProc,
    PFN_TCPWRITEPROC pfnWriteProc, PFN_SIGNALPROC pfnSignalProc, void * pUserData)
{
    hSocket->pReadChannel->pfnEventProc = pfnReadProc;
    hSocket->pWriteChannel->pfnEventProc = pfnWriteProc;
    hSocket->pfnSignalProc = pfnSignalProc;
    hSocket->pUserData = pUserData;
}

static BOOL iocp_tcp_listen_call(HIOCPFILE hListen, HIOCPFILE hAccept)
{
#ifndef SO_UPDATE_ACCEPT_CONTEXT
#define SO_UPDATE_ACCEPT_CONTEXT    0x700B
#endif
    int nAddrSize = 0;
    PFN_TCPLISTENPROC pfnLisenProc = hListen->pListenChannel->pfnEventProc;
    CACHE * pCache = hAccept->pInpack;

    if (0 == setsockopt((SOCKET)hAccept->hFile, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        (char*)&hListen->hFile, sizeof(hListen->hFile)))
    {
        nAddrSize = sizeof(hAccept->siLocal);
        getsockname((SOCKET)hAccept->hFile, (struct sockaddr *)&hAccept->siLocal, &nAddrSize);
        nAddrSize = sizeof(hAccept->siRemote);
        getpeername((SOCKET)hAccept->hFile, (struct sockaddr *)&hAccept->siRemote, &nAddrSize);
    }

    hAccept->pReadChannel->Event.dwEvent = IOEVENT_READ;
    hAccept->pWriteChannel->Event.dwEvent = IOEVENT_WRITE;

    if (pfnLisenProc)
    {
        pfnLisenProc(hListen, hAccept);
        return TRUE;
    }
    return FALSE;
}

static BOOL iocp_tcp_event_call(HIOCPFILE hSocket, IOCHANNEL * pChannel)
{
    union
    {
        void * pFunc;
        PFN_TCPREADPROC pfnReadProc;
        PFN_TCPWRITEPROC pfnWriteProc;
        PFN_TCPCONNECTPROC pfnConnectProc;
    }Call = { 0 };
    
    switch (pChannel->dwEvent)
    {
    case IOEVENT_READ:
    {
        if (Call.pfnReadProc = pChannel->pfnEventProc)
        {
            Call.pfnReadProc(hSocket, hSocket->pUserData);
        }
    }break;
    case IOEVENT_WRITE:
    {
        if (Call.pfnWriteProc = pChannel->pfnEventProc)
        {
            Call.pfnWriteProc(hSocket, hSocket->pUserData);
        }
        
    }break;
    case IOEVENT_CONNECT:
    {
        if (Call.pfnConnectProc = hSocket->pfnConnectProc)
        {
            Call.pfnConnectProc(hSocket, hSocket->pUserData);
        }
    }break;
    default:
    {
        assert(0 && "iocp_tcp_event_call 未知的处理方式");
    }break;
    }
    return !!Call.pFunc;

}

static BOOL iocp_tcp_signal_call(HIOCPFILE hSocket, IOCHANNEL * pChannel, DWORD dwError)
{
    PFN_SIGNALPROC pfnSignalProc = hSocket->pfnSignalProc;

    IOCP_LOCK(hSocket);
    if (INVALID_HANDLE_VALUE == hSocket->hFile)
    {
        pfnSignalProc = NULL;
    }
    IOCP_UNLOCK(hSocket);

    if (pfnSignalProc)
    {
        pfnSignalProc(hSocket, dwError, hSocket->pUserData);
        return TRUE;
    }

    return FALSE;
}


//非读写
BOOL iocp_tcp_listen(HIOCPFILE hSocket, SOCKADDR_IN * pBindAddr, PFN_TCPLISTENPROC pfnListenProc)
{
    HIOCPBASE hIocp = hSocket->hIocp;
    SOCKADDR_IN SocketAddr = { 0 };
    time_t tiCurrent = 0;

    hSocket->pListenChannel->pfnEventProc = pfnListenProc;
    if (pBindAddr && SOCKET_ERROR == bind((SOCKET)hSocket->hFile, (struct sockaddr *)pBindAddr, sizeof(SOCKADDR_IN)))
    {
        //不绑定的话， lpfnConnectEx 会返回GetLastError = 10022
        return FALSE;
    }

    if (SOCKET_ERROR == listen((SOCKET)hSocket->hFile, SOMAXCONN))
    {
        return FALSE;
    }

    hSocket->pListenChannel->Event.dwEvent = IOEVENT_ACCEPT;

    if (hSocket->pWriteChannel)
    {
        IOCP_FREE(hSocket->pWriteChannel);
        hSocket->pWriteChannel = NULL;
    }

    if (hSocket->pOutput)
    {
        //listen对象不需要写缓冲, 释放空间
        stream_buffer_free(hSocket->pOutput);
        hSocket->pOutput = NULL;
        hSocket->dwFlag &= ~IOCPFILE_FLAG_WRITEBUF;
    }

    if (hSocket->pInput)
    {
        stream_buffer_free(hSocket->pInput);
        hSocket->pInput = NULL;
        hSocket->dwFlag &= ~IOCPFILE_FLAG_READBUF;
    }

    if (hSocket->pOutpack)
    {
        cache_free(hSocket->pOutpack);
        hSocket->pOutpack = NULL;
    }

    return TRUE;
}

BOOL iocp_tcp_accept(HIOCPFILE hListen, HIOCPFILE hAccept)
{
    //这是一个异步操作, 可以调用多次, 添加多个链接套接字
    BOOL bResult = FALSE;
    DWORD dwBytes = 0;
    HIOCPBASE hIocp = hListen->hIocp;
    CACHE * pCache = hAccept->pInpack;

    assert(IOEVENT_ACCEPT == hListen->pListenChannel->Event.dwEvent && \
        "iocp_tcp_accept::Event.dwEvent 此时应该为IOEVENT_ACCEPT, 请检查!");

    if (NULL == hAccept)
    {
        assert(0 && "iocp_tcp_accept hAccept对象无效!");
        return FALSE;
    }

    //GetQueuedCompletionStatus时, key 是 hSocket, event 是 hClient->ioeRead
    //所以这个现在event 设置成跟 hSocket 一样,真正 accept时, 再改回 IOEVENT_READ
    //hClient->ioeRead.dwEvent = IOEVENT_ACCEPT;

    iocp_tcp_increment(hListen);
    iocp_tcp_increment(hAccept);
    memset(pCache->Buffer, 0, sizeof(pCache->Lenght));

    hAccept->pReadChannel->Event.dwEvent = hListen->pListenChannel->Event.dwEvent;//复制为IOEVENT_ACCEPT
    hAccept->hParent = hListen;
    assert(pCache->Lenght >= 64 + 64 && "iocp_tcp_accept hAccept提供的缓冲长度不够");
    bResult = IOCP_ACCEPT((SOCKET)hListen->hFile, hAccept->hFile, pCache->Buffer, 0,
        ADDROFFSET, ADDROFFSET, &dwBytes, &hAccept->pReadChannel->Event.pending);

    if (FALSE == bResult && ERROR_IO_PENDING != GetLastError())
    {
        iocp_tcp_decrement(hAccept);
        iocp_tcp_decrement(hListen);
        return FALSE;
    }

    return TRUE;
}

BOOL iocp_tcp_connect(HIOCPFILE hSocket, SOCKADDR_IN * pRemote, PFN_TCPCONNECTPROC pfnConnectProc)
{
    HIOCPBASE hIocp = hSocket->hIocp;

    BOOL bResult = FALSE;
    DWORD dwBytes = 0;
    SOCKADDR_IN SockAddr = { 0 };
    DWORD dwError = 0;
    int nAddrSize = sizeof(SOCKADDR_IN);
    if (NULL == pRemote || NULL == pfnConnectProc) return FALSE;

    IOCP_LOCK(hSocket);
    SockAddr.sin_family = AF_INET;
    if (SOCKET_ERROR == bind((SOCKET)hSocket->hFile, (struct sockaddr *)(&SockAddr), sizeof(SOCKADDR_IN))
        && WSAEINVAL != (dwError = GetLastError()))//已经绑定
    {
        IOCP_LOCK(hSocket);
        //不绑定的话， lpfnConnectEx 会返回GetLastError = 10022
        return FALSE;
    }
    getsockname((SOCKET)hSocket->hFile, (struct sockaddr *)&hSocket->siLocal, &nAddrSize);
    SockAddr = *pRemote;
    dwBytes = 0;

    //加入超时
    if (FALSE == channel_timer_reset(hSocket->hIocp, hSocket->pWriteChannel))
    {
        IOCP_LOCK(hSocket);
        return FALSE;
    }

    hSocket->pfnConnectProc = pfnConnectProc;
    hSocket->pWriteChannel->Event.dwEvent = IOEVENT_CONNECT;
    iocp_tcp_increment(hSocket);
    bResult = IOCP_CONNECT((SOCKET)hSocket->hFile,
        (const struct sockaddr *)&SockAddr,  // [in] 对方地址
        sizeof(SOCKADDR_IN),               // [in] 对方地址长度
        NULL,       // [in] 连接后要发送的内容，这里不用
        0,   // [in] 发送内容的字节数 ，这里不用
        &dwBytes,       // [out] 发送了多少个字节，这里不用
        &hSocket->pWriteChannel->Event.pending); // [in] 

    if (FALSE == bResult && ERROR_IO_PENDING != (dwError = GetLastError()))
    {
        IOCP_LOCK(hSocket);
        iocp_tcp_decrement(hSocket);
        return FALSE;
    }

    hSocket->siRemote = *pRemote;
    IOCP_LOCK(hSocket);
    return TRUE;
}

BOOL iocp_tcp_reconnect(HIOCPFILE hSocket)
{
    SOCKADDR_IN SockAddr = hSocket->siRemote;
    //这个函数应该是收到IOEVENT_CLOSE的时候调用的
    assert(SockAddr.sin_family && "iocp_tcp_reconnect 此对象之前并没有进行过链接操作");
    assert(INVALID_HANDLE_VALUE == hSocket->hFile && "iocp_tcp_reconnect 套接字此时应该为无效状态");
    //assert(0 == InterlockedExchangeAdd(&hSocket->nRecursionCount, 0) && "iocp_tcp_reconnect 错误的调用时机");

    IOCPFILE_UNLOCK_WRITE(hSocket);
    IOCPFILE_UNLOCK_READ(hSocket);

    if (hSocket->pOutput)
    {
        stream_buffer_clear(hSocket->pOutput);
    }

    if (hSocket->pInput)
    {
        stream_buffer_clear(hSocket->pInput);
    }

    if (hSocket->pOutpack)
    {
        hSocket->pOutpack->Offset = 0;
    }

    if (hSocket->pInpack)
    {
        hSocket->pInpack->Lenght = 0;
    }

    //iocp_tcp_set_writemark(hSocket, 0);
    //iocp_tcp_set_readmark(hSocket, 0);
    //重建fd
    hSocket->hFile = (HANDLE)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    if (INVALID_HANDLE_VALUE == hSocket->hFile) return FALSE;

    //此时引用计数应该为0 , 所以需要回复计数
    iocp_tcp_increment(hSocket);
    if (FALSE == iocp_tcp_connect(hSocket, &SockAddr, hSocket->pfnConnectProc))
    {
        iocp_tcp_decrement(hSocket);
        return FALSE;
    }

    return TRUE;
}

BOOL iocp_tcp_close(HIOCPFILE hSocket, DWORD dwDelayTime)
{
    BOOL bRet = FALSE;
    if (-1 == dwDelayTime) return FALSE;

    IOCP_LOCK(hSocket);

    assert(hSocket->hFile && "iocp_tcp_close 我们并没有设置fd为空的代码，请检查");

    if (SOCKET_ERROR != hSocket->hFile)
    {
        if (dwDelayTime > 0)
        {
            assert(NULL == hSocket->pCloseChannel && "iocp_tcp_close pCloseChannel 应该是最后才会用到的");

            if (hSocket->pCloseChannel = IOCP_MALLOC(sizeof(IOCHANNEL)))
            {
                memset(hSocket->pCloseChannel, 0, sizeof(IOCHANNEL));
                FLAG_REMOVE(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_READ);
                FLAG_REMOVE(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_WRITE);

                channel_timer_del(hSocket->hIocp, hSocket->pReadChannel);
                channel_timer_del(hSocket->hIocp, hSocket->pWriteChannel);

                memset(hSocket->pCloseChannel, 0, sizeof(IOCHANNEL));
                hSocket->pCloseChannel->Event.dwEvent = IOEVENT_CLOSE;
                hSocket->pCloseChannel->Event.hObject = hSocket;
                hSocket->pCloseChannel->uBitHeapIndex = -1;

                if (channel_timer_add(hSocket->hIocp, hSocket->pCloseChannel, dwDelayTime))
                {
                    iocp_tcp_increment(hSocket);
                    bRet = TRUE;
                }
                else
                {
                    dwDelayTime = 0;
                }
            }
            else
            {
                dwDelayTime = 0;
            }
        }

        if (0 == dwDelayTime)
        {
            bRet = TRUE;
            closesocket(hSocket->hFile);
            hSocket->hFile = INVALID_HANDLE_VALUE;
            iocp_tcp_decrement(hSocket);
        }
    }
    IOCP_LOCK(hSocket);

    return bRet;
}


//读写
int iocp_tcp_read(HIOCPFILE hSocket, char * pBuffer, unsigned int uSize)
{
    int nRet = 0;
    int uOldLenght = 0;
    IOCP_LOCK(hSocket);
    if (hSocket->pInput)
    {
        uOldLenght = stream_buffer_lenght(hSocket->pInput);
        nRet = stream_buffer_read(hSocket->pInput, pBuffer, uSize);

        //缓冲减少了, 检查是否需要重新唤醒recv
        if ((hSocket->dwFlag & IOCPFILE_FLAG_SUSPEND_READ) && FALSE == INVALID_HANDLE(hSocket))
        {
            if (hSocket->uMaxRead || hSocket->uMaxRead > stream_buffer_lenght(hSocket->pInput))
            {
                hSocket->dwFlag &= (~IOCPFILE_FLAG_SUSPEND_READ);

                if ((hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_READ)
                    && (hSocket->dwFlag & IOCPFILE_FLAG_KEEPREAD))
                {
                    iocp_tcp_iorecv_nolock(hSocket);
                }
            }
        }
    }
    else if (hSocket->pInpack->Lenght)
    {
        nRet = min(pBuffer, hSocket->pInpack->Lenght);
        memcpy(pBuffer, hSocket->pInpack->Buffer, nRet);

        hSocket->pInpack->Lenght -= nRet;

        if (hSocket->pInpack->Lenght)
        {
            memmove(hSocket->pInpack->Buffer, hSocket->pInpack->Buffer + nRet, hSocket->pInpack->Lenght);
        }
    }
    IOCP_UNLOCK(hSocket);
    return nRet;
}

int iocp_tcp_write(HIOCPFILE hSocket, const char * pBuffer, unsigned int uSize)
{
    int nRet = -1;
    CACHE * pCache = hSocket->pOutpack;

    IOCP_LOCK(hSocket);

    if (INVALID_HANDLE(hSocket))
    {
        nRet = -1;
        goto FINISH;
    }

    if (hSocket->dwFlag & IOCPFILE_FLAG_SUSPEND_WRITE)
    {
        nRet = 0;
        goto FINISH;
    }

    if (hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_WRITE)
    {
        SetLastError(ERROR_OPLOCK_NOT_GRANTED);//操作无效
        nRet = -1;
        goto FINISH;
    }

    if (hSocket->pOutput)//必须拥有写缓冲
    {
        nRet = stream_buffer_write(hSocket->pOutput, pBuffer, uSize);

        if (FALSE == iocp_tcp_iosend_nolock(hSocket))
        {
            nRet = -1;
        }
    }

FINISH:
    IOCP_UNLOCK(hSocket);
    //else if (uSize > 0)
    //{
    //    nRet = 0;
    //    if (TRUE == IOCPFILE_TRYLOCK_WRITE(hSocket))//没有写缓冲只能直接抢锁
    //    {
    //        nRet = pCache->Offset = min(uSize, pCache->Lenght);
    //        memcpy(pCache->Buffer, pBuffer, pCache->Offset);
    //        iocp_tcp_increment(hSocket);
    //        if (SOCKET_ERROR == WSASend((SOCKET)hSocket->hFile, (LPWSABUF)pCache, 1, 
    //            NULL, 0, &hSocket->pWriteChannel->Event.pending, NULL)
    //            && ERROR_IO_PENDING != GetLastError())
    //        {
    //            iocp_tcp_decrement(hSocket);
    //            IOCPFILE_UNLOCK_WRITE(hSocket);
    //            nRet = -1;
    //        }
    //    }
    //}

    return nRet;
}

BOOL iocp_tcp_enable_read(HIOCPFILE hSocket, BOOL bEnable)
{
    BOOL bRet = TRUE;
    IOCP_LOCK(hSocket);
    if (FALSE == bEnable)
    {
        if (TRUE == INVALID_HANDLE(hSocket))
        {
            goto FINISH;
        }

        if (hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_READ)
        {
            FLAG_REMOVE(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_READ);
            bRet = channel_timer_del(hSocket->hIocp, hSocket->pReadChannel);
        }
    }
    else
    {
        if (TRUE == INVALID_HANDLE(hSocket))
        {
            bRet = FALSE;
            goto FINISH;
        }
 
        FLAG_INSTER(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_READ);

        //如果是持续读模式且缓冲有数据, 我们需要发出读通知,我们通知IOcall帮我们完成
        if ((IOCPFILE_FLAG_KEEPREAD & hSocket->dwFlag)
            && iocp_tcp_get_input_lenght(hSocket)
            && TRUE == IOCPFILE_TRYLOCK_READ(hSocket))
        {
            bRet = PostQueuedCompletionStatus(hSocket->hIocp, MAXDWORD, hSocket,
                &hSocket->pReadChannel->Event.pending);

            if (FALSE == bRet)
            {
                IOCPFILE_UNLOCK_READ(hSocket);
            }
        }
        else
        {
            bRet = iocp_tcp_iorecv_nolock(hSocket);
        }

    }
        
FINISH:
    IOCP_UNLOCK(hSocket);
    return bRet;
}

BOOL iocp_tcp_enable_write(HIOCPFILE hSocket, BOOL bEnable)
{
    BOOL bRet = FALSE;

    IOCP_LOCK(hSocket);
    if (FALSE == bEnable)
    {
        if (TRUE == INVALID_HANDLE(hSocket))
        {
            goto FINISH;
        }

        FLAG_REMOVE(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_WRITE);
        bRet = channel_timer_del(hSocket->hIocp, hSocket->pWriteChannel);
    }
    else
    {
        if (FALSE == INVALID_HANDLE(hSocket))
        {
            FLAG_INSTER(hSocket->dwFlag, IOCPFILE_FLAG_ENABLE_WRITE);
            bRet = iocp_tcp_iosend_nolock(hSocket);
        }
        else
        {
            bRet = FALSE;
        }
    }

FINISH:
    IOCP_UNLOCK(hSocket);

    return bRet;
}

static void iocp_tcp_read_event(HIOCPFILE hSocket, IOCHANNEL * pChannel)
{
    CACHE * pCache = hSocket->pInpack;
    STREAMBUFFER * pStream = hSocket->pInput;
    unsigned int uSize = pChannel->Event.pending.InternalHigh;
    unsigned int uBytes = 0;
    time_t tiCurrent = iocp_get_time();//时间缓冲区

    DWORD dwError = pChannel->Event.pending.Internal;

    if (dwError || 0 == uSize)
    {
        IOCPFILE_UNLOCK_READ(hSocket);
        iocp_tcp_signal_call(hSocket, pChannel, dwError);
        return;
    }

    if (MAXDWORD == uSize) uSize = 0;//这个是为了标识手动发出的信号

    if (pStream && uSize)
    {
        //写入缓冲
        IOCP_LOCK(hSocket);
        uBytes = stream_buffer_write(pStream, pCache->Buffer, uSize);

        if (hSocket->uMaxRead && stream_buffer_lenght(pStream) >= hSocket->uMaxRead)
        {
            hSocket->dwFlag |= IOCPFILE_FLAG_SUSPEND_READ;
        }

        IOCP_UNLOCK(hSocket);
    }

    IOCPFILE_UNLOCK_READ(hSocket);//放开RECV

    if (pStream && uBytes != uSize)//写入缓冲时内存不足
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;//内存不足，不能够写入完整的数据
        iocp_tcp_signal_call(hSocket, pChannel, dwError);
        return;
    }

    do//检测是否需要发出通知
    {
        //如果多线程中有个无聊的家伙一直在读，那么loop线程有可能收不到读通知，
        //但这是正常情况

        IOCP_LOCK(hSocket);

        uBytes = iocp_tcp_get_input_lenght(hSocket);
        if (NULL == pStream ||//没有缓冲的话，必须将消息通知用户
            ((hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_READ) && uBytes > 0
            && uBytes >= iocp_tcp_get_read_watermark(hSocket)))
        {
            //唤醒条件:
            //1. 缓冲不为0
            //2. 缓冲大于等于水位
            //不检查句柄有效性
            IOCP_UNLOCK(hSocket);
            iocp_tcp_event_call(hSocket, pChannel);

        }
        else
        {
            IOCP_UNLOCK(hSocket);
            break;
        }
    } while (iocp_tcp_get_read_watermark(hSocket));
    //用户读完数据后, 更新最后发生读事件的时间
    iocp_tcp_set_readtime(hSocket, tiCurrent);
    //进行下一波读取
    iocp_tcp_keep_read(hSocket);
}

static void iocp_tcp_write_event(HIOCPFILE hSocket, IOCHANNEL * pChannel)
{
    CACHE * pCache = hSocket->pOutpack;
    STREAMBUFFER * pStream = hSocket->pOutput;
    unsigned int uSize = pChannel->Event.pending.InternalHigh;
    unsigned int uBytes = 0;
    time_t tiCurrent = iocp_get_time();//时间缓冲区

    DWORD dwError = pChannel->Event.pending.Internal;

    IOCPFILE_UNLOCK_WRITE(hSocket);
    if (0 == dwError && uSize > 0)
    {
        do
        {
            IOCP_LOCK(hSocket);
            //写水位事件, 每次IO最多通知一次
            if ((hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_WRITE) && !INVALID_HANDLE(hSocket) &&
                iocp_tcp_get_write_watermark(hSocket) >= iocp_tcp_get_output_lenght(hSocket))
            {
                //1. 缓冲为0
                //2. 水位大于缓冲
                //3. 句柄有效
                IOCP_UNLOCK(hSocket);
                iocp_tcp_event_call(hSocket, pChannel);
            }
            else
            {
                IOCP_UNLOCK(hSocket);
                break;
            }
        } while (FALSE);
        //更新最后写完成的时间
        iocp_tcp_set_writetime(hSocket, tiCurrent);
        //进行下一波发送
        iocp_tcp_keep_write(hSocket);
    }
    else
    {
        iocp_tcp_signal_call(hSocket, pChannel, dwError);
    }
}

static void iocp_tcp_keep_read(HIOCPFILE hSocket)
{
    DWORD dwError = 0;
    BOOL bKeep = FALSE;
    IOCP_LOCK(hSocket);

    if (hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_READ
        && !(hSocket->dwFlag & IOCPFILE_FLAG_SUSPEND_READ)
        && (hSocket->dwFlag & IOCPFILE_FLAG_KEEPREAD)
        && !INVALID_HANDLE(hSocket))
    {
        if (FALSE == iocp_tcp_iorecv_nolock(hSocket))
        {
            dwError = GetLastError();
        }
    }

    IOCP_UNLOCK(hSocket);

    if (dwError)
    {
        iocp_tcp_signal_call(hSocket, hSocket->pReadChannel, dwError);
    }
}

static void iocp_tcp_keep_write(HIOCPFILE hSocket)
{
    DWORD dwError = 0;
    BOOL bKeep = FALSE;
    IOCP_LOCK(hSocket);

    if (hSocket->dwFlag & IOCPFILE_FLAG_ENABLE_WRITE
        && !(hSocket->dwFlag & IOCPFILE_FLAG_SUSPEND_WRITE)
        && stream_buffer_lenght(hSocket->pOutput)
        && !INVALID_HANDLE(hSocket))
    {
        if (FALSE == iocp_tcp_iosend_nolock(hSocket))
        {
            dwError = GetLastError();
        }
    }

    IOCP_UNLOCK(hSocket);

    if (dwError)
    {
        iocp_tcp_signal_call(hSocket, hSocket->pReadChannel, dwError);
    }
}

static BOOL iocp_tcp_iorecv_nolock(HIOCPFILE hSocket)
{
    BOOL bRet = FALSE;
    CACHE * pCache = hSocket->pInpack;
    DWORD dwFlag = 0;

    if (TRUE == INVALID_HANDLE(hSocket))
    {
        SetLastError(WSAENOTCONN);
        return FALSE;
    }

    if (TRUE == IOCPFILE_TRYLOCK_READ(hSocket))
    {
        //插入定时器对象
        if (TRUE == channel_timer_reset(hSocket->hIocp, hSocket->pReadChannel))
        {
            pCache->Lenght = 0;
            iocp_tcp_increment(hSocket);
            if (SOCKET_ERROR == WSARecv((SOCKET)hSocket->hFile, (LPWSABUF)pCache, 1, NULL,
                &dwFlag, &hSocket->pReadChannel->Event.pending, NULL)
                && ERROR_IO_PENDING != GetLastError())
            {
                iocp_tcp_decrement(hSocket);
                IOCPFILE_UNLOCK_READ(hSocket);
            }
            else
            {
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

static BOOL iocp_tcp_iosend_nolock(HIOCPFILE hSocket)
{
    BOOL bRet = FALSE;
    CACHE * pCache = hSocket->pOutpack;

    if (TRUE == INVALID_HANDLE(hSocket))
    {
        SetLastError(WSAENOTCONN);
        return FALSE;
    }

    if (FALSE == IOCPFILE_TRYLOCK_WRITE(hSocket))
    {
        return TRUE;
    }

    //抢到锁了, 开始执行发送
    pCache->Offset = stream_buffer_read(hSocket->pOutput, pCache->Buffer, pCache->Lenght);

    if (pCache->Offset > 0)
    {
        if (TRUE == channel_timer_reset(hSocket->hIocp, hSocket->pWriteChannel))
        {
            iocp_tcp_increment(hSocket);

            if (SOCKET_ERROR == WSASend((SOCKET)hSocket->hFile, (LPWSABUF)pCache, 1, NULL, 0,
                &hSocket->pWriteChannel->Event.pending, NULL)
                && ERROR_IO_PENDING != GetLastError())
            {
                //WSAENOTSOCK 不是套接字
                //WSAENOTCONN 套接字未连接
                iocp_tcp_decrement(hSocket);
                IOCPFILE_UNLOCK_WRITE(hSocket);
            }
            else
            {
                bRet = TRUE;
            }
        }
    }
    else
    {
        IOCPFILE_UNLOCK_WRITE(hSocket);
        bRet = TRUE;
    }

    return bRet;
}


//utils
BOOL iocp_tcp_get_remote(HIOCPFILE hSocket, SOCKADDR_IN * pRemote)
{
    if (pRemote && hSocket->siRemote.sin_family)
    {
        *pRemote = hSocket->siRemote;
        return TRUE;
    }
    return FALSE;
}

BOOL iocp_tcp_get_local(HIOCPFILE hSocket, SOCKADDR_IN * pLocal)
{
    if (pLocal && hSocket && hSocket->siLocal.sin_family)
    {
        *pLocal = hSocket->siLocal;
        return TRUE;
    }
    return FALSE;
}

BOOL iocp_tcp_bind(HIOCPFILE hSocket, SOCKADDR_IN * pBindAddr)
{
    if (NULL == pBindAddr ||
        SOCKET_ERROR == bind((SOCKET)hSocket->hFile, (struct sockaddr *)pBindAddr, sizeof(SOCKADDR_IN)))
    {
        //不绑定的话， lpfnConnectEx 会返回GetLastError = 10022
        return FALSE;
    }

    return TRUE;
}

BOOL iocp_tcp_shutdown(HIOCPFILE hSocket, int nHow)
{
    if (0 == shutdown((SOCKET)(hSocket)->hFile, nHow))
    {
        switch (nHow)
        {
        case SD_RECEIVE:
        {
            iocp_tcp_enable_read(hSocket, FALSE);
        }break;
        case SD_SEND:
        {
            iocp_tcp_enable_write(hSocket, FALSE);
        }break;
        default:
        {
            iocp_tcp_enable_read(hSocket, FALSE);
            iocp_tcp_enable_write(hSocket, FALSE);
        }break;
        }
        return TRUE;
    }

    return FALSE;
}

BOOL iocp_tcp_keepalive(HIOCPFILE hSocket, DWORD dwIdleMilliseconds,
    DWORD dwIntervalMilliseconds)
{
    const char chOpt = 1;
    DWORD dwBytes = 0;
    if (setsockopt((SOCKET)hSocket->hFile, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)))
    {
        return FALSE;
    }

    // 设置超时详细信息
    struct{
        ULONG onoff;
        ULONG keepalivetime;
        ULONG keepaliveinterval;
    }klive = { 0 };

#ifndef SIO_KEEPALIVE_VALS
#define SIO_KEEPALIVE_VALS 0x98000004
#endif

    klive.onoff = 1; //启用保活
    klive.keepalivetime = dwIdleMilliseconds;//空闲多久发起一个心跳包
    klive.keepaliveinterval = dwIntervalMilliseconds; //重试间隔

    return !WSAIoctl((SOCKET)hSocket->hFile, SIO_KEEPALIVE_VALS, &klive, sizeof(klive),
        NULL, 0, &dwBytes, NULL, NULL);
}


