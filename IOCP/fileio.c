

#include "fileio.h"
#include "channel.h"
#include <assert.h>




void iocp_file_free(HIOCPFILE hObject)
{
    assert(0 == hObject->nRecursionCount && "iocp_tcp_free 错误的释放时机");

    if (INVALID_HANDLE_VALUE != hObject->hFile)
    {
        if (IOCP_OBJECT_PIPE == hObject->dwObject)
        {
            CloseHandle(hObject->hFile);
        }
        else
        {
            closesocket(hObject->hFile);
        }
            
        hObject->hFile = INVALID_HANDLE_VALUE;
    }

    if (hObject->pReadChannel)
    {
        IOCP_FREE(hObject->pReadChannel);
        hObject->pReadChannel = NULL;
    }

    if (hObject->pWriteChannel)
    {
        IOCP_FREE(hObject->pWriteChannel);
        hObject->pWriteChannel = NULL;
    }

    if (hObject->pCloseChannel)
    {
        IOCP_FREE(hObject->pCloseChannel);
        hObject->pCloseChannel = NULL;
    }

    if (hObject->pOutput)
    {
        stream_buffer_free(hObject->pOutput);
        hObject->pOutput = NULL;
    }

    if (hObject->pInput)
    {
        stream_buffer_free(hObject->pInput);
        hObject->pInput = NULL;
    }

    if (hObject->pOutpack)
    {
        cache_free(hObject->pOutpack);
        hObject->pOutpack = NULL;
    }

    if (hObject->pInpack)
    {
        cache_free(hObject->pInpack);
        hObject->pInpack = NULL;
    }

    if (hObject->pLock)
    {
        DeleteCriticalSection(&hObject->CriticalSection);
    }

    IOCP_FREE(hObject);
}

HIOCPFILE iocp_file_reference(HIOCPFILE hObject)
{
    //主要是增加一个引用计数, 如果你想和其他线程共享此对象时,
    //调用该函数能够确保在其他线程安全地使用该对象
    //不要忘记, 每次reference都必须有一个对应的close操作
    HIOCPFILE hObject2 = hObject;
    if (hObject)
    {
        iocp_file_increment(hObject);
    }
    return hObject2;
}

long iocp_file_increment(HIOCPFILE hObject)
{ 
    return InterlockedIncrement(&hObject->nRecursionCount);
}

long iocp_file_decrement(HIOCPFILE hObject)
{
    long nRecursionCount = InterlockedDecrement(&hObject->nRecursionCount);
    if (0 == nRecursionCount)
    {
        iocp_file_free(hObject);;
    }
    return nRecursionCount;
}

void iocp_file_set_userdata(HIOCPFILE hObject, void * pUserData)
{
    hObject->pUserData = pUserData;
}

void *iocp_file_get_userdata(HIOCPFILE hObject)
{
    return hObject->pUserData;
}

BOOL iocp_file_work_in_loop(HIOCPFILE hObject)
{
    return iocp_base_get_threadid(hObject->hIocp) == GetCurrentThreadId();
}

HIOCPBASE iocp_file_get_iocpbase(HIOCPFILE hObject)
{
    return hObject->hIocp;
}


void iocp_file_set_tag(HIOCPFILE hObject, LPCTSTR strTag)
{
#ifdef  _DEBUG
    for (int i = 0; strTag[i] && sizeof(hObject->strTag) - 1 > i; i++)
    {
        hObject->strTag[i] = strTag[i];
    }
#endif

}

void iocp_file_set_read_watermark(HIOCPFILE hObject, unsigned int uVal)
{
    hObject->uReadWaterMark = uVal;
}

unsigned int iocp_file_get_read_watermark(HIOCPFILE hObject)
{
    return hObject->uReadWaterMark;
}

void iocp_file_set_write_watermark(HIOCPFILE hObject, unsigned int uVal)
{
    hObject->uWriteWaterMark = uVal;
}

unsigned int iocp_file_get_write_watermark(HIOCPFILE hObject)
{
    return hObject->uWriteWaterMark;
}

void iocp_file_set_max_writebuf(HIOCPFILE hObject, unsigned int uSize)
{
    stream_buffer_get_maxbuffer(hObject->pOutput, uSize);
}

unsigned int iocp_file_get_max_writebuf(HIOCPFILE hObject)
{
    return stream_buffer_get_maxbuffer(hObject->pOutput);
}

unsigned int iocp_file_get_output_lenght(HIOCPFILE hObject)
{
    unsigned int ret = 0;

    if (hObject->pOutput)
    {
        ret = stream_buffer_lenght(hObject->pOutput);
    }

    return ret;
}

unsigned int iocp_file_get_input_lenght(HIOCPFILE hObject)
{
    unsigned int ret = 0;

    if (hObject->pInput)
    {
        ret = stream_buffer_lenght(hObject->pInput);
    }
    else if (hObject->pInpack)
    {
        ret = hObject->pInpack->Lenght;
    }

    return ret;
}

void iocp_file_set_readtime(HIOCPFILE hObject, time_t tiVal)
{
    hObject->tiLastRead = tiVal;
}

time_t iocp_file_get_readtime(HIOCPFILE hObject)
{
    return hObject->tiLastRead;
}

void iocp_file_set_writetime(HIOCPFILE hObject, time_t tiVal)
{
    hObject->tiLastWrite = tiVal;
}

time_t iocp_file_get_writetime(HIOCPFILE hObject)
{
    return hObject->tiLastWrite;
}

BOOL iocp_file_set_read_timeout(HIOCPFILE hObject, DWORD dwTimeOut)
{
    channel_set_timeout(hObject->pReadChannel, dwTimeOut);
     return TRUE;
}

BOOL iocp_file_set_write_timeout(HIOCPFILE hObject, DWORD dwTimeOut)
{
    channel_set_timeout(hObject->pWriteChannel, dwTimeOut);
    return TRUE;
}

DWORD iocp_file_get_read_timeout(HIOCPFILE hObject)
{
    return channel_get_timeout(hObject->pReadChannel);
}

DWORD iocp_file_get_write_timeout(HIOCPFILE hObject)
{
    return channel_get_timeout(hObject->pWriteChannel);
}

void iocp_file_set_read_timeoutcb(HIOCPFILE hObject, PFN_TIMERPROC pfnTimerProc)
{
    channel_set_timeoutcb(hObject->pReadChannel, pfnTimerProc);
}

void iocp_file_set_write_timeoutcb(HIOCPFILE hObject, PFN_TIMERPROC pfnTimerProc)
{
    channel_set_timeoutcb(hObject->pWriteChannel, pfnTimerProc);
}

