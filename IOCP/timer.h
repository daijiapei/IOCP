

#ifndef __IOCP_TIMER_H__
#define __IOCP_TIMER_H__

#include "iocpdef.h"

#define iocp_timer_set_userdata          iocp_file_set_userdata
#define iocp_timer_get_userdata          iocp_file_get_userdata

#ifdef __cplusplus
extern "C" {
#endif

    HIOCPFILE iocp_timer_new(HIOCPBASE hIocp, DWORD dwFlag);
    void iocp_timer_set_signalcb(HIOCPFILE hTimer, PFN_SIGNALPROC pfnSignalProc);
    BOOL iocp_timer_add(HIOCPFILE hTimer, DWORD dwTimeOut, PFN_TIMERPROC pfnTimerProc);
    BOOL iocp_timer_del(HIOCPFILE hTimer);
    BOOL iocp_timer_close(HIOCPFILE hTimer);

#ifdef __cplusplus
};
#endif

#endif
