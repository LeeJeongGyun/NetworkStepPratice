
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32")

void HandleError(const char* cause)
{
    std::cout << cause << std::endl;
    std::cout << ::WSAGetLastError() << std::endl;
}

int main()
{
    WSAData wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET servSock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (servSock == INVALID_SOCKET)
    {
        HandleError("::socket error");
        return 0;
    }

    SOCKADDR_IN sockAdr;
    ::memset(&sockAdr, 0, sizeof(sockAdr));
    sockAdr.sin_family = AF_INET;
    sockAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    sockAdr.sin_port = ::htons(9999);

    if (SOCKET_ERROR == ::bind(servSock, reinterpret_cast<const SOCKADDR*>(&sockAdr), sizeof(sockAdr)))
    {
        HandleError("::bind error");
        return 0;
    }

    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);
    while (true)
    {
        char recvBuffer[1000];
        int recvSize = ::recvfrom(servSock, recvBuffer, sizeof(recvBuffer), 0, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
        if (recvSize <= 0)
        {
            HandleError("recvfrom return <= 0");
            return 0;
        }

        std::cout << "Recv Size: " << recvSize << std::endl;
        std::cout << "Recv Data: " << recvBuffer << std::endl;
        int sendSize = ::sendto(servSock, recvBuffer, recvSize, 0, reinterpret_cast<SOCKADDR*>(&clntAdr), sizeof(SOCKADDR_IN));
        if (sendSize <= 0)
        {
            HandleError("sendto return <= 0");
            return 0;
        }
    }

    ::WSACleanup();
    return 0;
}
