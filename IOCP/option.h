
#ifndef __IOCP_FILE_OPTION_H__
#define __IOCP_FILE_OPTION_H__

#include "iocpdef.h"


typedef struct __FIOCALL_TABLE{
    void * pfnIOCompleteCall;
    void * pfnIOListen;
    void * pfnIOConnect;
    void * pfnIOSend;
    void * pfnIORecv;
    void * pfnIOSendTo;
    void * pfnIORecvFrom;
    void * pfnIOEnableSend;
    void * pfnIOEnableRecv;
    void * pfnIOShutdown;
    void * pfnIOClose;
}FIOCALL_TABLE;


HIOCPFILE fio_create_tcp();
HIOCPFILE fio_create_udp();
HIOCPFILE fio_create_pipe();

void fio_listen();
void fio_connect();
void fio_send();
void fio_recv();
void fio_sendto();
void fio_recvfrom();
void fio_shutdown();
void fio_close();

void fio_free(HIOCPFILE hObject);


#endif