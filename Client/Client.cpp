
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <vector>
#pragma comment(lib, "ws2_32")

void HandleError(const char* cause)
{
    std::cout << cause << std::endl;
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

    SOCKADDR_IN servAdr;
    ::memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    ::InetPtonA(AF_INET, "127.0.0.1", &servAdr.sin_addr);
    servAdr.sin_port = ::htons(9999);

    if (SOCKET_ERROR == ::connect(clntSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(SOCKADDR_IN)))
    {
        HandleError("Connect Fail");
        return 0;
    }

    char buffer[1000] = "Hello, Server";
    while (true)
    {
        int sendSize = ::send(clntSock, buffer, sizeof(buffer), 0);
        if (sendSize <= 0)
        {
            HandleError("::send return error");
            return 0;
        }
        
        std::cout << "Send Size " << sendSize << std::endl;
        std::cout << "Send Data: " << buffer << std::endl;
        int recvSize = ::recv(clntSock, buffer, sizeof(buffer), 0);
        if (recvSize <= 0)
        {
            HandleError("::recv return error");
            return 0;
        }
        std::cout << "Recv Success!!" << std::endl;
    
        Sleep(1000);
    }
   
    ::WSACleanup();
    return 0;
}
