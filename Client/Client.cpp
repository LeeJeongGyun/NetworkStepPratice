
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
    Sleep(1000);
    WSAData wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clntSock = ::socket(AF_INET, SOCK_DGRAM, 0);
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

    // Connected UDP
    // 실제로 연결되는 것은 아님. 소켓에 등록만 한다는 개념
    if (SOCKET_ERROR == ::connect(clntSock, reinterpret_cast<SOCKADDR*>(&servAdr), sizeof(SOCKADDR_IN)))
    {
        HandleError("Connected UDP Connect Fail");
        return 0;
    }
   
    char buffer[1000] = "Hello, Server";
    while (true)
    {
        // 나의 IP 주소 + 포트 번호 설정
        // Unconnected UDP
        /*int sendSize = ::sendto(clntSock, buffer, sizeof(buffer), 0, reinterpret_cast<const SOCKADDR*>(&servAdr), sizeof(SOCKADDR_IN));
        if (sendSize <= 0)
        {
            HandleError("::sendto return error");
            return 0;
        }*/

        // Connected UDP
        int sendSize = ::send(clntSock, buffer, sizeof(buffer), 0);
        if (sendSize <= 0)
        {
            HandleError("::sendto return error");
            return 0;
        }

        std::cout << "Send Size " << sendSize << std::endl;
        std::cout << "Send Data: " << buffer << std::endl;

        //SOCKADDR_IN recvAdr;
        //int recvAdrSize = sizeof(recvAdr);
        //// Unconnected UDP
        //int recvSize = ::recvfrom(clntSock, buffer, sizeof(buffer), 0, reinterpret_cast<SOCKADDR*>(&recvAdr), &recvAdrSize);
        //if (recvSize <= 0)
        //{
        //    HandleError("::recvfrom return error");
        //    return 0;
        //}

        // Connected UDP
        int recvSize = ::recv(clntSock, buffer, sizeof(buffer), 0);
        if (recvSize <= 0)
        {
            HandleError("::recvfrom return error");
            return 0;
        }


        Sleep(1000);
    }
   
    ::WSACleanup();
    return 0;
}
