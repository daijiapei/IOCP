
#ifndef __IOCP_DEFINE_H__
#define __IOCP_DEFINE_H__

// Remove pointless warning messages
#ifdef _MSC_VER

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // eliminate deprecation warnings for VS2005
#endif
#endif // _MSC_VER


#include "socketbuffer.h"
#include <Windows.h>
#include <time.h>
#include <assert.h>
#include "bitheap.h"
#include "list.h"

typedef BOOL (PASCAL FAR * CONNECTEXPTR) (
_In_ SOCKET s,
_In_reads_bytes_(namelen) const struct sockaddr FAR *name,
_In_ int namelen,
_In_reads_bytes_opt_(dwSendDataLength) PVOID lpSendBuffer,
_In_ DWORD dwSendDataLength,
_Out_ LPDWORD lpdwBytesSent,
_Inout_ LPOVERLAPPED lpOverlapped
);

typedef BOOL(PASCAL FAR * ACCEPTEXPTR)(
    _In_ SOCKET sListenSocket,
    _In_ SOCKET sAcceptSocket,
    _Out_writes_bytes_(dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength) PVOID lpOutputBuffer,
    _In_ DWORD dwReceiveDataLength,
    _In_ DWORD dwLocalAddressLength,
    _In_ DWORD dwRemoteAddressLength,
    _Out_ LPDWORD lpdwBytesReceived,
    _Inout_ LPOVERLAPPED lpOverlapped
    );


typedef BOOL(PASCAL FAR * DISCONNECTEXPTR) (
_In_ SOCKET s,
_Inout_opt_ LPOVERLAPPED lpOverlapped,
_In_ DWORD  dwFlags,
_In_ DWORD  dwReserved
);



//全局环境
typedef struct __IOCPGLOBALENV
{
    void * (*_iocp_mallo)(unsigned int);
    void * (*_iocp_realloc)(void *, unsigned int);
    void * (*_iocp_calloc)(unsigned int, unsigned int);
    void(*_iocp_free)(void *);

    CONNECTEXPTR lpfnConnectEx;
    ACCEPTEXPTR  lpfnAcceptEx;
    DISCONNECTEXPTR lpfnDisconnectEx;//不推荐使用
}HIOCPGLOBALENV;


#ifdef __cplusplus
extern "C" HIOCPGLOBALENV IocpEnv;
#else
extern HIOCPGLOBALENV IocpEnv;
#endif


#define IOCP_MALLOC(size) IocpEnv._iocp_mallo(size)
#define IOCP_REALLOC(ptr, size) IocpEnv._iocp_realloc(ptr, size)
#define IOCP_CALLOC(num, size) IocpEnv._iocp_calloc(num, size)
#define IOCP_FREE(ptr) IocpEnv._iocp_free(ptr)


#define IOCP_CONNECT      (IocpEnv.lpfnConnectEx)
#define IOCP_ACCEPT       (IocpEnv.lpfnAcceptEx)
#define IOCP_DISCONNECT   (IocpEnv.lpfnDisconnectEx)

#define IOCP_SPIN_LOCK(obj) \
    do {\
            while(InterlockedCompareExchange(&(obj)->uSpinLock, 1, 0));\
        } while (0)

#define IOCP_SPIN_UNLOCK(obj) InterlockedExchange(&(obj)->uSpinLock, 0)



#define IOCP_ACQUIRE_LOCK(obj, lockvar) \
    do { \
        if((obj)->lockvar) \
            EnterCriticalSection((LPCRITICAL_SECTION)(obj)->lockvar); \
            }while (0)

#define IOCP_RELEASE_LOCK(obj, lockvar) \
    do { \
        if((obj)->lockvar) \
            LeaveCriticalSection((LPCRITICAL_SECTION)(obj)->lockvar); \
            } while (0)

#define IOCP_LOCK(obj) IOCP_ACQUIRE_LOCK((obj), pLock)
#define IOCP_UNLOCK(obj) IOCP_RELEASE_LOCK((obj), pLock)
//#define IOCP_WAIT_COND(obj)  SleepConditionVariableCS()
//#define IOCP_WAKE_COND  WakeConditionVariable

#define FLAG_INSTER(f, d)  ((f) |= (d))
#define FLAG_REMOVE(f, d)  ((f) &= (~d))
#define STRUCT_OFFSET(type, member) (unsigned long)&(((type *)0)->member)

//是否无效句柄
#define INVALID_HANDLE(obj)  (INVALID_HANDLE_VALUE == (obj)->hFile)


#ifdef __cplusplus
extern "C" {
#endif

    BOOL iocp_init_env();//main开始时, 初始化环境
    void iocp_uninit_env();//main结束时, 释放环境
    time_t iocp_get_time();//取得当前时间, 毫秒
    int __cdecl _iocp_output(const char * funcname, const char * format, ...);
    
#ifdef _DEBUG
#define IOCPOUTPUT(format, ...)  _iocp_output(__FUNCTION__, format, ##__VA_ARGS__)
#else
#define IOCPOUTPUT __noop
#endif

#ifdef __cplusplus
};
#endif


typedef struct __IOEVENT
{
    OVERLAPPED pending;
#define IOEVENT_NON        0x0000
#define IOEVENT_READ       0x0001
#define IOEVENT_WRITE      0x0002
#define IOEVENT_ACCEPT     0x0004
#define IOEVENT_CONNECT    0x0008
#define IOEVENT_CLOSE      0x0010
#define IOEVENT_TIMEOUT    0x0020
#define IOEVENT_CANCEL     0x0040
#define IOEVENT_FREE       0x0080


#define IOEVENT_CUSTOM     0x0002FFFF 
#define IOEVENT_ACTIVE     0xFFFFFFFF 
    DWORD dwEvent;
    struct __IOCPFILE * hObject;//指向自己
} *HIOEVENT;


typedef struct __WATERMARK
{
    unsigned long uLow;//当缓冲达到这个值就产生回调通知
    unsigned long uHigh;//最大允许多少缓冲
}WATERMARK;

typedef struct __IOEVENT2
{
    OVERLAPPED pending;
//#define IOEVENT_NON        0x0000
//#define IOEVENT_READ       0x0001
//#define IOEVENT_WRITE      0x0002
//#define IOEVENT_ACCEPT     0x0004
//#define IOEVENT_CONNECT    0x0008
//#define IOEVENT_CLOSE      0x0010
//#define IOEVENT_TIMEOUT    0x0020
//#define IOEVENT_CANCEL     0x0040
//#define IOEVENT_FREE       0x0080
//
//
//#define IOEVENT_CUSTOM     0x0002FFFF 
//#define IOEVENT_ACTIVE     0xFFFFFFFF 
    DWORD dwEvent;
    struct __IOCPFILE * hObject;//指向自己

    LIST_ENTRY(__IOCHANNEL) ActiveList;//当事件激活后,加入优先级列表
    DWORD dwPriorityLevel;//优先级别
    DWORD dwEvent;//实际发生的事件,根据此来选择回调函数
    void * pfnEventProc;//回调
    void * pfnTimerProc;//超时回调

//#define CHANNEL_LOCK_STATE(pcl)   ((pcl)->uIoWorking ? (pcl)->uIoWorking : FALSE)
//#define CHANNEL_TRYLOCK(pcl)      (!InterlockedCompareExchange(&(pcl)->uIoWorking, TRUE, FALSE))
//#define CHANNEL_UNLOCK(pcl)       (InterlockedExchange(&(pcl)->uIoWorking, FALSE))
//
    volatile ULONG uIoWorking;//正在进行AIO工作, 此时异步尚未完成

    BOOL bTimeOut;//已经成为超时事件
    DWORD dwTimeOut;//超时时间,毫秒
    time_t tiAbsoluteTimeOut;//绝对超时时间
    volatile ULONG uBitHeapIndex;//在小根堆的位置
}IOEVENT2;

typedef struct __IOCHANNEL
{
    struct __IOEVENT Event;//事件

    LIST_ENTRY(__IOCHANNEL) ActiveList;//当事件激活后,加入优先级列表
    DWORD dwPriorityLevel;//优先级别
    DWORD dwEvent;//实际发生的事件,根据此来选择回调函数
    void * pfnEventProc;//回调
    void * pfnTimerProc;//超时回调

#define CHANNEL_LOCK_STATE(pcl)   ((pcl)->uIoWorking ? (pcl)->uIoWorking : FALSE)
#define CHANNEL_TRYLOCK(pcl)      (!InterlockedCompareExchange(&(pcl)->uIoWorking, TRUE, FALSE))
#define CHANNEL_UNLOCK(pcl)       (InterlockedExchange(&(pcl)->uIoWorking, FALSE))

    volatile ULONG uIoWorking;//正在进行AIO工作, 此时异步尚未完成

    BOOL bTimeOut;//已经成为超时事件
    DWORD dwTimeOut;//超时时间,毫秒
    time_t tiAbsoluteTimeOut;//绝对超时时间
    volatile ULONG uBitHeapIndex;//在小根堆的位置
}IOCHANNEL;





typedef void(*PFN_CUSTOMEVENTPROC)(struct __IOCPBASE * hIocp, DWORD dwEvent, void * pUserData);
typedef void(*PFN_IOCPOBJECTIOPROC)(struct __IOCPFILE * hObject, struct __IOCHANNEL * pChannel);

typedef void(*PFN_TCPCONNECTPROC)(struct __IOCPFILE *hObject, void *pUserData);
typedef void(*PFN_TCPLISTENPROC)(struct __IOCPFILE * hListen, struct __IOCPFILE * hClient);
typedef void(*PFN_TCPWRITEPROC)(struct __IOCPFILE * hObject, void * pUserData);
typedef void(*PFN_TCPREADPROC)(struct __IOCPFILE * hObject, void * pUserData);

typedef void(*PFN_SIGNALPROC)(struct __IOCPFILE * hObject, struct __IOCHANNEL * pChannel, DWORD dwError);
typedef void(*PFN_TIMERPROC)(struct __IOCPFILE * hObject, struct __IOCHANNEL * pChannel, void * pUserData);


typedef struct __IOCPFILE *(*PFNREQUESTOBJECTPROC)(struct __IOCPFILE * hSrc,DWORD dwObject, DWORD dwFlag);



//每个线程一个 iocpbase
typedef struct __IOCPBASE
{
    HANDLE hFile;//iocp句柄

    HANDLE hThread;//记录所在线程
    DWORD dwThreadId;//所在线程id:

    BOOL bAlertable;//是否启用APC异步

#define IOCP_STATE_NONE        MAXDWORD//初始状态
#define IOCP_STATE_RUN         0x0000//正常运行
#define IOCP_STATE_QUIT        0x0001//正常退出
    DWORD dwState;//当前状态

    time_t tiCache;//当前时间

    volatile ULONG uActive;//是否在执行任务
    struct __IOEVENT ioeActive;//激活一下hiocp,让waiting的hiocp进行新一轮loop
    struct __IOEVENT ioeCustom;//自定义消息
    PFN_CUSTOMEVENTPROC pfnCustomEventCall;//自定义消息回调

    BOOL bLock;//产生的套接字是否使用锁? 当在多线程使用该iocp时, 建议开启
    void * pLock;//指向CriticalSection
    CRITICAL_SECTION CriticalSection;//锁住对象

    DWORD dwPriorityCount;//优先级队列数量,默认为1
    LIST_HEAD(PRIORITYLISTHEAD, __IOCHANNEL) * PriorityListHead;//优先队列头部

    DWORD dwOverLappedCount;//最大接收OverLapped个数

    DWORD dwFileCount;//文件个数

    BITHEAP * pTimerHeap;//定时器小根堆

    HANDLE hSuspendEvent;//用来挂起IOCP
    DWORD dwSuspendMillisecond;//挂起毫秒数，如果为INFINITE则一直挂起
    void * pUserData;//用户自定义数据
}*HIOCPBASE;



typedef struct __IOCPFILE
{
    HANDLE hFile;//对象句柄
#ifdef  _DEBUG
    TCHAR strTag[64];//为了方便测试的时候识别这是哪个套接字
#endif

#define IOCP_OBJECT_NON      0x00
#define IOCP_OBJECT_BASE     0x01
#define IOCP_OBJECT_TIMER    0x02
#define IOCP_OBJECT_TCP      0x03
#define IOCP_OBJECT_UDP      0x04
#define IOCP_OBJECT_FILE     0x05
#define IOCP_OBJECT_PIPE     0x06
    DWORD dwObject;//对象类型
    long nRecursionCount;//引用计数,为0则释放
    PFN_IOCPOBJECTIOPROC pfnIoCall;//IO通知回调函数
    
    void * pfnSignalProc;//错误回调

    void * pUserData;//自定义数据

    HIOCPBASE hIocp;//所绑定的对象源
    CRITICAL_SECTION * pLock;//指向CriticalSection
    CRITICAL_SECTION CriticalSection;

    ULONG uSpinLock;
#define IOCPFILE_FLAG_SUSPEND_READ    0x00000001//挂起读
#define IOCPFILE_FLAG_SUSPEND_WRITE   0x00000002//挂起写
#define IOCPFILE_FLAG_ENABLE_READ     0x00000004//开启读
#define IOCPFILE_FLAG_ENABLE_WRITE    0x00000008//开启写

#define IOCPFILE_FLAG_READBUF         0x01000000//开启读缓冲，默认开启
#define IOCPFILE_FLAG_WRITEBUF        0x02000000//开启写缓冲，必须开启
#define IOCPFILE_FLAG_KEEPREAD        0x04000000//保持一直读，默认开启
#define IOCPFILE_FLAG_KEEPWRITE       0x08000000//保持一直写，这个设置是被忽略的

#define IOCPFILE_FLAG_KEEPTIMER       0x00100000//定时器持续执行

#define IOCPFILE_FLAG_CLOSE           0x80000000//关闭
    DWORD dwFlag;


#define IOCPFILE_READ_LOCKED_STATE(obj) CHANNEL_LOCK_STATE((obj)->pReadChannel)
#define IOCPFILE_TRYLOCK_READ(obj) CHANNEL_TRYLOCK((obj)->pReadChannel)
#define IOCPFILE_UNLOCK_READ(obj) CHANNEL_UNLOCK((obj)->pReadChannel)
    union{
        struct __IOCHANNEL * pReadChannel;
        struct __IOCHANNEL * pListenChannel;
        struct __IOCHANNEL * pTimerChannel;
    };

#define IOCPFILE_WRITE_LOCKED_STATE(obj) CHANNEL_LOCK_STATE((obj)->pWriteChannel)
#define IOCPFILE_TRYLOCK_WRITE(obj) CHANNEL_TRYLOCK((obj)->pWriteChannel)
#define IOCPFILE_UNLOCK_WRITE(obj) CHANNEL_UNLOCK((obj)->pWriteChannel)
    union{
        struct __IOCHANNEL * pWriteChannel;
        struct __IOCHANNEL * pConnectChannel;
    };

    struct __IOCHANNEL * pCloseChannel;//延时关闭时用到

    union
    {
        struct __IOCPFILE * hParent;//一般是监听套接字
        PFN_TCPCONNECTPROC pfnConnectProc;
    };

    SOCKADDR_IN siRemote;//远程地址
    SOCKADDR_IN siLocal;//本地地址

    time_t tiLastWrite;//最后一次写入时间
    time_t tiLastRead;//最后一次读取时间

    STREAMBUFFER * pOutput;//输出流缓冲
    STREAMBUFFER * pInput;//输入流缓冲

#define FILEBUFFER_SIZE   4096
    CACHE * pOutpack;//输出包缓冲,一般用于UDP
    CACHE * pInpack;//输入包缓冲,一般用于UDP
    
    ULONG uReadWaterMark;//读水位
    ULONG uWriteWaterMark;//写水位
    ULONG uMaxRead;
}* HIOCPFILE;


#endif