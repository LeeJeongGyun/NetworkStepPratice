
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <vector>
#pragma comment(lib, "ws2_32")

const int BUFF_SIZE = 1024;
struct Session
{
    SOCKET sock;
    char recvBuffer[BUFF_SIZE] = { 0 };
    int recvBytes = 0;
    WSAOVERLAPPED overlapped = { 0 };
};

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

    // 논블로킹 함수 세팅
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
    
    // Overlapped IO (비동기 + 논블로킹) (이벤트 기반)
    // WSARecv, WSASend, AcceptEx, ConnectEx
    
    while (true)
    {
        SOCKET clntSock;
        SOCKADDR_IN clntAdr;
        int clntAdrSize = sizeof(clntAdr);
        while (true)
        {
            clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
            if (clntSock != INVALID_SOCKET) break;
               
            if (::WSAGetLastError() == WSAEWOULDBLOCK) continue;

            // ERROR
            HandleError("accept error");
            return 0;
        }

        Session session = Session{ clntSock };
        WSAEVENT hEvent = ::WSACreateEvent();
        session.overlapped.hEvent = hEvent;

        std::cout << "Client Connected" << std::endl;
        while (true)
        {
            WSABUF buf;
            buf.buf = session.recvBuffer;
            buf.len = BUFF_SIZE;

            DWORD recvSize;
            DWORD flags = 0;    // 0 세팅안하면 오류발생
            // WSABUF 변수는 지워져도 상관x
            // But, recvBuffer는 지워지면 안된다.
            if (SOCKET_ERROR == ::WSARecv(session.sock, &buf, 1, &recvSize, &flags, &session.overlapped, NULL))
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    ::closesocket(clntSock);
                    ::WSACloseEvent(hEvent);
                    return 0; // ERROR
                }

                ::WSAWaitForMultipleEvents(1, &hEvent, TRUE, WSA_INFINITE, FALSE);
                ::WSAGetOverlappedResult(session.sock, &session.overlapped, &recvSize, FALSE, &flags);
            }

            std::cout << "Recv Data Size: " << recvSize << std::endl;
            std::cout << "Recv Data: " << session.recvBuffer << std::endl;
        }
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
