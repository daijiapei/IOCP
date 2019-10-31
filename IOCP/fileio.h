
#ifndef __IOCP_FILE_IO_H__
#define __IOCP_FILE_IO_H__

#include "iocpbase.h"


#ifdef __cplusplus
extern "C" {
#endif

    long iocp_file_increment(HIOCPFILE hObject);
    long iocp_file_decrement(HIOCPFILE hObject);
    void iocp_file_free(HIOCPFILE hObject);

    BOOL iocp_file_work_in_loop(HIOCPFILE hObject);
    HIOCPFILE iocp_file_reference(HIOCPFILE hObject);
    HIOCPBASE iocp_file_get_iocpbase(HIOCPFILE hObject);
    DWORD iocp_file_get_state(HIOCPFILE hObject);
    BOOL iocp_file_timeout(HIOCPFILE hObject, time_t);

    void iocp_file_set_tag(HIOCPFILE hObject, LPCTSTR);

    void iocp_file_set_read_watermark(HIOCPFILE hObject, unsigned int uVal);
    unsigned int iocp_file_get_read_watermark(HIOCPFILE hObject);
    void iocp_file_set_write_watermark(HIOCPFILE hObject, unsigned int uVal);
    unsigned int iocp_file_get_write_watermark(HIOCPFILE hObject);

    unsigned int iocp_file_get_output_lenght(HIOCPFILE hObject);
    unsigned int iocp_file_get_input_lenght(HIOCPFILE hObject);

    void iocp_file_set_readtime(HIOCPFILE hObject, time_t tiRead);
    time_t iocp_file_get_readtime(HIOCPFILE hObject);
    void iocp_file_set_writetime(HIOCPFILE hObject, time_t tiWrite);
    time_t iocp_file_get_writetime(HIOCPFILE hObject);

    BOOL iocp_file_set_read_timeout(HIOCPFILE hObject, DWORD dwTimeOut);
    BOOL iocp_file_set_write_timeout(HIOCPFILE hObject, DWORD dwTimeOut);
    DWORD iocp_file_get_read_timeout(HIOCPFILE hObject);
    DWORD iocp_file_get_write_timeout(HIOCPFILE hObject);
    void iocp_file_set_read_timeoutcb(HIOCPFILE hObject, PFN_TIMERPROC pfnTimerProc);
    void iocp_file_set_write_timeoutcb(HIOCPFILE hObject, PFN_TIMERPROC pfnTimerProc);
    
    void iocp_file_set_userdata(HIOCPFILE hSocket, void * pUserData);
    void *iocp_file_get_userdata(HIOCPFILE hSocket);

#ifdef __cplusplus
};
#endif


#endif