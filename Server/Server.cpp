
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32")

void HandleError(const char* reason)
{
    std::cout << ::WSAGetLastError() << std::endl;
}

int main()
{
    WSAData wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
    {
        HandleError("::socket return error");
        return 0;
    }

    SOCKADDR_IN sockAdr;
    ::memset(&sockAdr, 0, sizeof(sockAdr));
    sockAdr.sin_family      = AF_INET;
    sockAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    sockAdr.sin_port        = ::htons(9999);

    if (SOCKET_ERROR == ::bind(listenSock, reinterpret_cast<const SOCKADDR*>(&sockAdr), sizeof(sockAdr)))
    {
        HandleError("::bind return error");
        return 0;
    }

    if (SOCKET_ERROR == ::listen(listenSock, SOMAXCONN))
    {
        HandleError("::listen return error");
        return 0;
    }

    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);
    
    std::cout << "Accept Wait" << std::endl;
    SOCKET clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
    if (clntSock == INVALID_SOCKET)
    {
        HandleError("::accept return error");
        return 0;
    }

    char clntIp[16];
    if (NULL != ::InetNtopA(AF_INET, &clntAdr.sin_addr, clntIp, 32))
    {
        std::cout << "Ip: " << clntIp << std::endl;
        std::cout << "Port: " << ::ntohs(clntAdr.sin_port) << std::endl;
    }

    char buffer[1024];
    while (true)
    {
        int recvSize = ::recv(clntSock, buffer, sizeof(buffer), 0);
        if (recvSize == 0 || recvSize == SOCKET_ERROR)
        {
            HandleError("::recv return error");
            return 0;
        }

        buffer[recvSize] = 0;
        std::cout << "RecvSize: " << recvSize << std::endl;
        std::cout << buffer << std::endl;
        int sendSize = ::send(clntSock, buffer, recvSize, 0);
        if (sendSize == 0 || sendSize == SOCKET_ERROR)
        {
            HandleError("::send return error");
            return 0;
        }

        ::Sleep(1000);
    }

    ::WSACleanup();
    return 0;
}
