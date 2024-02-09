
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

    SOCKET listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
    {
        HandleError("::socket error");
        return 0;
    }

    u_long nonBlockOn = 1;
    if (INVALID_SOCKET == ::ioctlsocket(listenSock, FIONBIO, &nonBlockOn))
    {
        HandleError("::ioctlsocket error");
        return 0;
    }

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    servAdr.sin_port = ::htons(9999);

    if (SOCKET_ERROR == ::bind(listenSock, reinterpret_cast<const SOCKADDR*>(&servAdr), sizeof(SOCKADDR_IN)))
    {
        HandleError("::bind Error");
        return 0;
    }

    if (SOCKET_ERROR == ::listen(listenSock, SOMAXCONN))
    {
        HandleError("::bind Error");
        return 0;
    }

    std::cout << "Accept" << std::endl;
    SOCKET clntSock;
    SOCKADDR_IN clntAdr;
    int clntAdrSize = sizeof(clntAdr);

    while (true)
    {
        clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
        if (clntSock == INVALID_SOCKET)
        {
            if (::WSAGetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            }
        }

        std::cout << "Client Connect Success" << std::endl;

        while (true)
        {
            char buffer[1000];
            int recvSize = ::recv(clntSock, buffer, sizeof(buffer), 0);
            if (recvSize <= 0)
            {
                if (::WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    continue;
                }
                else
                {
                    HandleError("::recv error");
                    return 0;
                }
            }

            std::cout << "Recv Size: " << recvSize << std::endl;
            std::cout << "Recv Data: " << buffer << std::endl;
            
            while (true)
            {
                int sendSize = ::send(clntSock, buffer, recvSize, 0);
                if (sendSize <= 0)
                {
                    if (::WSAGetLastError() == WSAEWOULDBLOCK)
                    {
                        continue;
                    }
                    else
                    {
                        HandleError("::send error");
                        return 0;
                    }
                }
                else
                {
                    std::cout << "Send Success!!" << std::endl;
                    break;
                }
            }
        }
    }


    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
