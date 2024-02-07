
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32")

void HandleError(const char* reason)
{
    std::cout << reason << std::endl;
    std::cout << ::WSAGetLastError() << std::endl;
}

int main()
{
    Sleep(1000);
    WSAData wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clntSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (clntSock == INVALID_SOCKET)
    {
        HandleError("::socket return error");
        return 0;
    }

    SOCKADDR_IN sockAdr;
    ::memset(&sockAdr, 0, sizeof(sockAdr));
    sockAdr.sin_family = AF_INET;
    ::InetPtonA(AF_INET, "127.0.0.1", &sockAdr.sin_addr);
    sockAdr.sin_port = ::htons(9999);

    if (SOCKET_ERROR == ::connect(clntSock, reinterpret_cast<const SOCKADDR*>(&sockAdr), sizeof(sockAdr)))
    {
        HandleError("::connect return error");
        return 0;
    }

    std::cout << "Connect Success" << std::endl;

    char buffer[1000] = "Hello, Server";
    while (true)
    {
        int sendSize = ::send(clntSock, buffer, sizeof(buffer), 0);
        if (sendSize == 0 || sendSize == SOCKET_ERROR)
        {
            HandleError("::send return error");
            return 0;
        }

        int recvSize = ::recv(clntSock, buffer, sizeof(buffer), 0);
        if (recvSize == 0 || recvSize == SOCKET_ERROR)
        {
            HandleError("::recv return error");
            return 0;
        }

        std::cout << "RecvSize: " << recvSize << std::endl;
        std::cout << buffer << std::endl;
        Sleep(1000);
    }
   
    ::WSACleanup();
    return 0;
}
