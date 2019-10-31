
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



//ȫ�ֻ���
typedef struct __IOCPGLOBALENV
{
    void * (*_iocp_mallo)(unsigned int);
    void * (*_iocp_realloc)(void *, unsigned int);
    void * (*_iocp_calloc)(unsigned int, unsigned int);
    void(*_iocp_free)(void *);

    CONNECTEXPTR lpfnConnectEx;
    ACCEPTEXPTR  lpfnAcceptEx;
    DISCONNECTEXPTR lpfnDisconnectEx;//���Ƽ�ʹ��
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

//�Ƿ���Ч���
#define INVALID_HANDLE(obj)  (INVALID_HANDLE_VALUE == (obj)->hFile)


#ifdef __cplusplus
extern "C" {
#endif

    BOOL iocp_init_env();//main��ʼʱ, ��ʼ������
    void iocp_uninit_env();//main����ʱ, �ͷŻ���
    time_t iocp_get_time();//ȡ�õ�ǰʱ��, ����
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
    struct __IOCPFILE * hObject;//ָ���Լ�
} *HIOEVENT;


typedef struct __WATERMARK
{
    unsigned long uLow;//������ﵽ���ֵ�Ͳ����ص�֪ͨ
    unsigned long uHigh;//���������ٻ���
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
    struct __IOCPFILE * hObject;//ָ���Լ�

    LIST_ENTRY(__IOCHANNEL) ActiveList;//���¼������,�������ȼ��б�
    DWORD dwPriorityLevel;//���ȼ���
    DWORD dwEvent;//ʵ�ʷ������¼�,���ݴ���ѡ��ص�����
    void * pfnEventProc;//�ص�
    void * pfnTimerProc;//��ʱ�ص�

//#define CHANNEL_LOCK_STATE(pcl)   ((pcl)->uIoWorking ? (pcl)->uIoWorking : FALSE)
//#define CHANNEL_TRYLOCK(pcl)      (!InterlockedCompareExchange(&(pcl)->uIoWorking, TRUE, FALSE))
//#define CHANNEL_UNLOCK(pcl)       (InterlockedExchange(&(pcl)->uIoWorking, FALSE))
//
    volatile ULONG uIoWorking;//���ڽ���AIO����, ��ʱ�첽��δ���

    BOOL bTimeOut;//�Ѿ���Ϊ��ʱ�¼�
    DWORD dwTimeOut;//��ʱʱ��,����
    time_t tiAbsoluteTimeOut;//���Գ�ʱʱ��
    volatile ULONG uBitHeapIndex;//��С���ѵ�λ��
}IOEVENT2;

typedef struct __IOCHANNEL
{
    struct __IOEVENT Event;//�¼�

    LIST_ENTRY(__IOCHANNEL) ActiveList;//���¼������,�������ȼ��б�
    DWORD dwPriorityLevel;//���ȼ���
    DWORD dwEvent;//ʵ�ʷ������¼�,���ݴ���ѡ��ص�����
    void * pfnEventProc;//�ص�
    void * pfnTimerProc;//��ʱ�ص�

#define CHANNEL_LOCK_STATE(pcl)   ((pcl)->uIoWorking ? (pcl)->uIoWorking : FALSE)
#define CHANNEL_TRYLOCK(pcl)      (!InterlockedCompareExchange(&(pcl)->uIoWorking, TRUE, FALSE))
#define CHANNEL_UNLOCK(pcl)       (InterlockedExchange(&(pcl)->uIoWorking, FALSE))

    volatile ULONG uIoWorking;//���ڽ���AIO����, ��ʱ�첽��δ���

    BOOL bTimeOut;//�Ѿ���Ϊ��ʱ�¼�
    DWORD dwTimeOut;//��ʱʱ��,����
    time_t tiAbsoluteTimeOut;//���Գ�ʱʱ��
    volatile ULONG uBitHeapIndex;//��С���ѵ�λ��
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



//ÿ���߳�һ�� iocpbase
typedef struct __IOCPBASE
{
    HANDLE hFile;//iocp���

    HANDLE hThread;//��¼�����߳�
    DWORD dwThreadId;//�����߳�id:

    BOOL bAlertable;//�Ƿ�����APC�첽

#define IOCP_STATE_NONE        MAXDWORD//��ʼ״̬
#define IOCP_STATE_RUN         0x0000//��������
#define IOCP_STATE_QUIT        0x0001//�����˳�
    DWORD dwState;//��ǰ״̬

    time_t tiCache;//��ǰʱ��

    volatile ULONG uActive;//�Ƿ���ִ������
    struct __IOEVENT ioeActive;//����һ��hiocp,��waiting��hiocp������һ��loop
    struct __IOEVENT ioeCustom;//�Զ�����Ϣ
    PFN_CUSTOMEVENTPROC pfnCustomEventCall;//�Զ�����Ϣ�ص�

    BOOL bLock;//�������׽����Ƿ�ʹ����? ���ڶ��߳�ʹ�ø�iocpʱ, ���鿪��
    void * pLock;//ָ��CriticalSection
    CRITICAL_SECTION CriticalSection;//��ס����

    DWORD dwPriorityCount;//���ȼ���������,Ĭ��Ϊ1
    LIST_HEAD(PRIORITYLISTHEAD, __IOCHANNEL) * PriorityListHead;//���ȶ���ͷ��

    DWORD dwOverLappedCount;//������OverLapped����

    DWORD dwFileCount;//�ļ�����

    BITHEAP * pTimerHeap;//��ʱ��С����

    HANDLE hSuspendEvent;//��������IOCP
    DWORD dwSuspendMillisecond;//��������������ΪINFINITE��һֱ����
    void * pUserData;//�û��Զ�������
}*HIOCPBASE;



typedef struct __IOCPFILE
{
    HANDLE hFile;//������
#ifdef  _DEBUG
    TCHAR strTag[64];//Ϊ�˷�����Ե�ʱ��ʶ�������ĸ��׽���
#endif

#define IOCP_OBJECT_NON      0x00
#define IOCP_OBJECT_BASE     0x01
#define IOCP_OBJECT_TIMER    0x02
#define IOCP_OBJECT_TCP      0x03
#define IOCP_OBJECT_UDP      0x04
#define IOCP_OBJECT_FILE     0x05
#define IOCP_OBJECT_PIPE     0x06
    DWORD dwObject;//��������
    long nRecursionCount;//���ü���,Ϊ0���ͷ�
    PFN_IOCPOBJECTIOPROC pfnIoCall;//IO֪ͨ�ص�����
    
    void * pfnSignalProc;//����ص�

    void * pUserData;//�Զ�������

    HIOCPBASE hIocp;//���󶨵Ķ���Դ
    CRITICAL_SECTION * pLock;//ָ��CriticalSection
    CRITICAL_SECTION CriticalSection;

    ULONG uSpinLock;
#define IOCPFILE_FLAG_SUSPEND_READ    0x00000001//�����
#define IOCPFILE_FLAG_SUSPEND_WRITE   0x00000002//����д
#define IOCPFILE_FLAG_ENABLE_READ     0x00000004//������
#define IOCPFILE_FLAG_ENABLE_WRITE    0x00000008//����д

#define IOCPFILE_FLAG_READBUF         0x01000000//���������壬Ĭ�Ͽ���
#define IOCPFILE_FLAG_WRITEBUF        0x02000000//����д���壬���뿪��
#define IOCPFILE_FLAG_KEEPREAD        0x04000000//����һֱ����Ĭ�Ͽ���
#define IOCPFILE_FLAG_KEEPWRITE       0x08000000//����һֱд����������Ǳ����Ե�

#define IOCPFILE_FLAG_KEEPTIMER       0x00100000//��ʱ������ִ��

#define IOCPFILE_FLAG_CLOSE           0x80000000//�ر�
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

    struct __IOCHANNEL * pCloseChannel;//��ʱ�ر�ʱ�õ�

    union
    {
        struct __IOCPFILE * hParent;//һ���Ǽ����׽���
        PFN_TCPCONNECTPROC pfnConnectProc;
    };

    SOCKADDR_IN siRemote;//Զ�̵�ַ
    SOCKADDR_IN siLocal;//���ص�ַ

    time_t tiLastWrite;//���һ��д��ʱ��
    time_t tiLastRead;//���һ�ζ�ȡʱ��

    STREAMBUFFER * pOutput;//���������
    STREAMBUFFER * pInput;//����������

#define FILEBUFFER_SIZE   4096
    CACHE * pOutpack;//���������,һ������UDP
    CACHE * pInpack;//���������,һ������UDP
    
    ULONG uReadWaterMark;//��ˮλ
    ULONG uWriteWaterMark;//дˮλ
    ULONG uMaxRead;
}* HIOCPFILE;


#endif