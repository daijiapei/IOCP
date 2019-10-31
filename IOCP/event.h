
#ifndef __IOCP_EVENT_H__
#define __IOCP_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

    //IOCHANNEL * channel_new();
    //void channel_free(IOCHANNEL * pChannel);
    ////提供给小根堆的回调函数
    //int __stdcall channel_compare_object(void * pObject1, void * pObject2);
    //void __stdcall channel_update_index(void * pObject, unsigned int uIndex);
    //BOOL channel_check_timeout(IOCHANNEL * pChannel, time_t tiVal);

    //void channel_set_keepactive(IOCHANNEL * pChannel, BOOL bVal);
    //BOOL channel_get_keepactive(IOCHANNEL * pChannel);
    //DWORD channel_k2u_error(IOCHANNEL * pChannel);

    //void channel_set_timeout(IOCHANNEL * pChannel, DWORD dwTimeOut);
    //DWORD channel_get_timeout(IOCHANNEL * pChannel);

    //BOOL channel_timer_add(HIOCPBASE hIocp, IOCHANNEL * pChannel, DWORD dwTimeOut);
    //BOOL channel_timer_del(HIOCPBASE hIocp, IOCHANNEL * pChannel);
    //BOOL channel_timer_reset(HIOCPBASE hIocp, IOCHANNEL * pChannel);
#ifdef __cplusplus
};
#endif

#endif 