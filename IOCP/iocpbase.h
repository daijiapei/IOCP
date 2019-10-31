
#ifndef __IOCP_SOCKET_BASE_H__
#define __IOCP_SOCKET_BASE_H__

#include "iocpdef.h"

#define OVERLAPPED_COUNT   1024

#ifdef __cplusplus
extern "C" {
#endif

#define iocp_base_handle(hIocp) ((hIocp)->hFile)

    //HIOCPBASE��һ������, ����װ�ظ����׽����¼�
    HIOCPBASE iocp_base_new();
    void iocp_base_free(HIOCPBASE hIocp);

    void iocp_base_set_custom_cb(HIOCPBASE hIocp, PFN_CUSTOMEVENTPROC custom_event_cb);
    //post һ���Զ����¼�
    BOOL iocp_base_custom_event_call(HIOCPBASE hIocp, DWORD dwEvent, void * arg);
    BOOL iocp_base_active(HIOCPBASE hIocp);
    //iocp_base_dispatch�������ܹ�����CreateThread
    unsigned int __stdcall iocp_base_dispatch(HIOCPBASE hIocp);

    //Get  Set
    DWORD iocp_base_get_threadid(HIOCPBASE hIocp);
    void iocp_base_set_priority_count(HIOCPBASE hIocp, DWORD dwCount);
    DWORD iocp_base_get_priority_count(HIOCPBASE hIocp);

    void iocp_base_set_lapped_count(HIOCPBASE hIocp, DWORD dwCount);
    DWORD iocp_base_get_lapped_count(HIOCPBASE hIocp);

    //iocp������socket�Ƿ�Ҫʹ������
    void iocp_base_enable_lock(HIOCPBASE hIocp);
    void iocp_base_disable_lock(HIOCPBASE hIocp);
    BOOL iocp_base_is_enble_lock(HIOCPBASE hIocp);

    void iocp_base_enable_apc(HIOCPBASE hIocp);
    void iocp_base_disable_apc(HIOCPBASE hIocp);
    BOOL iocp_base_is_enble_apc(HIOCPBASE hIocp);

    DWORD iocp_base_set_state(HIOCPBASE hIocp, DWORD dwState);
    DWORD iocp_base_get_state(HIOCPBASE hIocp);
    void iocp_base_loopexit(HIOCPBASE hIocp);

    void iocp_base_set_userdata(HIOCPBASE hIocp, void * pUserData);
    void * iocp_base_get_userdata(HIOCPBASE hIocp);


    BOOL iocp_base_is_suspend(HIOCPBASE hIocp);
    BOOL iocp_base_suspend(HIOCPBASE hIocp, DWORD dwMilliSecond);//��iocpbase�������
    BOOL iocp_base_resume(HIOCPBASE hIocp);//unsuspend, ȡ��iocpbase�������

#ifdef __cplusplus
};
#endif

#endif

