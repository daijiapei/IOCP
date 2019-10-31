
#include <WinSock2.h>
#include <mswsock.h>
#include <stdio.h>
#include "iocpdef.h"

#define IOCP_OUTPUT_STACK 1024

int __cdecl _iocp_output(const char * funcname, const char * format, ...)
{
    int len = 0;
    char buffer[IOCP_OUTPUT_STACK] = { 0 };
    va_list args;

    va_start(args, format);

    len += sprintf_s(&buffer[len], sizeof(buffer), "iocp_output [%05] -> ", GetCurrentThreadId());

    len += vsprintf_s(&buffer[len], sizeof(buffer) - len - 64, format, args);

    len += sprintf_s(&buffer[len], sizeof(buffer) - len, " - %s", funcname);

    va_end(args);

    buffer[len++] = '\n';
    buffer[len] = '\0';

    OutputDebugStringA(buffer);

    return len;
}

extern HIOCPGLOBALENV IocpEnv = { 0 };

BOOL iocp_init_env()
{
    BOOL bRet = FALSE;
    WSADATA WsaData;
    GUID GuidConnectEx = WSAID_CONNECTEX;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    GUID GuidDisconnEx = WSAID_DISCONNECTEX;
    SOCKET Socket = SOCKET_ERROR;
    DWORD dwBytes = 0;

    memset(&IocpEnv, 0, sizeof(HIOCPGLOBALENV));

    WSAStartup(0x0201, &WsaData);

    IocpEnv._iocp_mallo = malloc;
    IocpEnv._iocp_realloc = realloc;
    IocpEnv._iocp_calloc = calloc;
    IocpEnv._iocp_free = free;

    do{
        Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0x01);

        if (SOCKET_ERROR == Socket) break;

        if (SOCKET_ERROR == WSAIoctl(Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &GuidConnectEx, sizeof(GuidConnectEx),
            &IocpEnv.lpfnConnectEx, sizeof(IocpEnv.lpfnConnectEx), &dwBytes, 0, 0))
        {
            break;
        }

        if (SOCKET_ERROR == WSAIoctl(Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &GuidAcceptEx, sizeof(GuidAcceptEx),
            &IocpEnv.lpfnAcceptEx, sizeof(&IocpEnv.lpfnAcceptEx), &dwBytes, 0, 0))
        {
            break;
        }

        if (SOCKET_ERROR == WSAIoctl(Socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &GuidDisconnEx, sizeof(GuidDisconnEx),
            &IocpEnv.lpfnDisconnectEx, sizeof(&IocpEnv.lpfnDisconnectEx), &dwBytes, 0, 0))
        {
            break;
        }

        bRet = TRUE;
    }while(0);
    
    if (SOCKET_ERROR != Socket)
    {
        closesocket(Socket);
    }

    return bRet;
}


void iocp_uninit_env()
{
    WSACleanup();
    memset(&IocpEnv, 0, sizeof(HIOCPGLOBALENV));
}

time_t iocp_get_time()
{
    time_t tiCurrent = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&tiCurrent);
    tiCurrent = tiCurrent / 1000; 
    return tiCurrent;
}
