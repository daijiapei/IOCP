
#ifndef __IOCP_TCP_SOCKET_H__
#define __IOCP_TCP_SOCKET_H__


//sockwork负责socket连接的各种工作,比如创建封装好的套接字对象
//发送数据,接收数据, 监听, 连接服务器等

#include "iocpdef.h"
#include "fileio.h"


#define iocp_tcp_get_iocpbase          iocp_file_get_iocpbase

#define iocp_tcp_set_write_watermark   iocp_file_set_write_watermark
#define iocp_tcp_get_write_watermark   iocp_file_get_write_watermark
#define iocp_tcp_set_read_watermark    iocp_file_set_read_watermark
#define iocp_tcp_get_read_watermark    iocp_file_get_read_watermark

#define iocp_tcp_set_readtime          iocp_file_set_readtime
#define iocp_tcp_get_readtime          iocp_file_get_readtime
#define iocp_tcp_set_writetime         iocp_file_set_writetime
#define iocp_tcp_get_writetime         iocp_file_get_writetime
#define iocp_tcp_get_output_lenght     iocp_file_get_output_lenght
#define iocp_tcp_get_input_lenght      iocp_file_get_input_lenght

#define iocp_tcp_set_read_timeout      iocp_file_set_read_timeout
#define iocp_tcp_set_write_timeout     iocp_file_set_write_timeout
#define iocp_tcp_get_read_timeout      iocp_file_get_read_timeout
#define iocp_tcp_get_write_timeout     iocp_file_get_write_timeout
#define iocp_tcp_set_read_timeoutcb    iocp_file_set_read_timeoutcb
#define iocp_tcp_set_write_timeoutcb   iocp_file_set_write_timeoutcb

#define iocp_tcp_set_userdata          iocp_file_set_userdata
#define iocp_tcp_get_userdata          iocp_file_get_userdata
#ifdef __cplusplus
extern "C" {
#endif

    HIOCPFILE iocp_tcp_new(HIOCPBASE hIocp, SOCKET hFile, DWORD dwFlag);
    void iocp_tcp_setcb(HIOCPFILE hSocket, PFN_TCPREADPROC pfnReadProc,
        PFN_TCPWRITEPROC pfnWriteProc, PFN_SIGNALPROC pfnSignalProc, void * pUserData);
    

    BOOL iocp_tcp_listen(HIOCPFILE hSocket, SOCKADDR_IN * pBindAddr, PFN_TCPLISTENPROC pfnListenProc);
    BOOL iocp_tcp_accept(HIOCPFILE hListen, HIOCPFILE hAccept);
    BOOL iocp_tcp_connect(HIOCPFILE hSocket, SOCKADDR_IN * pRemote, PFN_TCPCONNECTPROC pfnConnectProc);
    BOOL iocp_tcp_reconnect(HIOCPFILE hSocket);
    BOOL iocp_tcp_close(HIOCPFILE hSocket, DWORD dwDelayTime);


    int iocp_tcp_write(HIOCPFILE hSocket, const char * pBuffer, unsigned int uSize);
    int iocp_tcp_read(HIOCPFILE hSocket, char * pBuffer, unsigned int uSize);
    BOOL iocp_tcp_enable_write(HIOCPFILE hSocket, BOOL bEnable);
    BOOL iocp_tcp_enable_read(HIOCPFILE hSocket, BOOL bEnable);

    
    //utils
    BOOL iocp_tcp_get_remote(HIOCPFILE hSocket, SOCKADDR_IN * pRemote);
    BOOL iocp_tcp_get_local(HIOCPFILE hSocket, SOCKADDR_IN * pLocal);
    BOOL iocp_tcp_bind(HIOCPFILE hSocket, SOCKADDR_IN * pBindAddr);
    BOOL iocp_tcp_shutdown(HIOCPFILE hSocket, int nHow);
    BOOL iocp_tcp_keepalive(HIOCPFILE hSocket, DWORD dwIdleMilliseconds,
        DWORD dwIntervalMilliseconds);
#ifdef __cplusplus
};
#endif

#endif