

#include "tcpsocket.h"
#include "iocpbase.h"
#include <stdio.h>
#include "fileio.h"
#pragma comment(lib,"ws2_32.lib")

#ifndef __IOCP_DEMO__
#define demo  main
#endif

void tcp_event_call(HIOCPFILE hSocket, DWORD dwEvent, void * pUserData);
void tcp_signal_call(HIOCPFILE hSocket, DWORD dwEvent, DWORD dwError, void * pUserData);


void custom_event_call(HIOCPBASE hIocp, DWORD dwEvent, void * pUserData);


void tcp_listen_cb(struct __IOCPFILE * hListen, struct __IOCPFILE * hClient);
void tcp_read_cb(HIOCPFILE hSocket, void * pUserData);

int demo(int argc, char ** argv)
{
    iocp_init_env();

    HIOCPBASE hIocp = iocp_base_new();
    iocp_base_set_custom_cb(hIocp, custom_event_call);

    SOCKADDR_IN SockAddr = { 0 };
    int nAddrSize = sizeof(SOCKADDR_IN);
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(8080);

    //TCP
    HIOCPFILE hListen = iocp_tcp_new(hIocp, NULL, NULL);
    iocp_file_set_tag(hListen, TEXT("hListen"));
    iocp_tcp_listen(hListen, &SockAddr, tcp_listen_cb);

    HIOCPFILE hAccept = iocp_tcp_new(hIocp, NULL, NULL);
    iocp_file_set_tag(hAccept, TEXT("hAccept"));
    iocp_tcp_setcb(hAccept, tcp_read_cb, NULL, NULL, NULL);
    iocp_tcp_accept(hListen, hAccept);

    iocp_base_dispatch(hIocp);//消息循环

    iocp_base_free(hIocp);
    iocp_tcp_close(hListen, 0);
    iocp_uninit_env();

    printf("main: process is over!\n");
    getchar();

    return 0;
}

void tcp_listen_cb(struct __IOCPFILE * hListen, struct __IOCPFILE * hClient)
{
    iocp_tcp_enable_read(hClient, TRUE);

    HIOCPFILE hAccept = iocp_tcp_new(iocp_tcp_get_iocpbase(hListen), NULL, NULL);
    iocp_file_set_tag(hAccept, TEXT("hAccept"));
    iocp_tcp_setcb(hAccept, tcp_read_cb, NULL, NULL, NULL);
    iocp_tcp_accept(hListen, hAccept);
}

void tcp_read_cb(HIOCPFILE hSocket, void * pUserData)
{
    char msg[64] = { 0 };
    int size = sizeof(msg);

    size = iocp_tcp_read(hSocket, msg, size - 1);

    msg[size] = 0;
    printf("tcp_read_cb = %s\n", msg);
    //iocp_tcp_close(hSocket);
    iocp_tcp_write(hSocket, msg, size);
}

void tcp_write_cb(HIOCPFILE hSocket, void * pUserData)
{
    printf("tcp_write_cb %d \n", hSocket, iocp_tcp_get_output_lenght(hSocket));
}

void tcp_event_call(HIOCPFILE hSocket, DWORD dwEvent, void * pUserData)
{
    char msg[64] = { 0 };
    int size = sizeof(msg);
    HIOCPFILE client = hSocket;

    switch (dwEvent)
    {
    case IOEVENT_ACCEPT:
    {

    }//流入IOEVENT_CONNECT
    case IOEVENT_CONNECT:
    {
        //激活写事件
    }break;
    case IOEVENT_READ:
    {
        size = iocp_tcp_read(hSocket, msg, size - 1);

        msg[size] = 0;

        iocp_tcp_write(hSocket, msg, size);
    }break;
    case IOEVENT_WRITE:
    {
        if (0 == iocp_tcp_get_output_lenght(hSocket))
        {
            //printf("IOEVENT_WRITE: %ld count = %u flushed msg!\n", hSocket, uWriteCount);
            //iocp_tcp_close(hSocket);
        }
    }break;
    default:
    {
        printf("socket_event_call: 未处理的事件 = %ld\n", dwEvent);
    }break;
    }

    return;
}

void tcp_signal_call(HIOCPFILE hSocket, DWORD dwEvent, DWORD dwError, void * pUserData)
{
    //printf("socket_signal_call:hSocket = %ld\t dwEvent = %ld\tdwError=%ld\n", hSocket, dwEvent, dwError);

    HIOCPBASE hIocp = iocp_tcp_get_iocpbase(hSocket);
    int nCount = 0;
    if (IOEVENT_CLOSE == dwEvent)
    {
    }
    else
    {
        iocp_tcp_close(hSocket, 0);
    }


    return;
}

void custom_event_call(HIOCPBASE hIocp, DWORD dwEvent, void * pUserData)
{
    printf("dwEvent = %ld\t arg=%s\n", dwEvent, pUserData);
    iocp_base_loopexit(hIocp);
}
