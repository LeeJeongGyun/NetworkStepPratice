
#include <iostream>
#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <vector>
#include<thread>
#pragma comment(lib, "ws2_32")

const int BUFF_SIZE = 1024;
struct Session
{
    Session(SOCKET s) { sock = s; }
    SOCKET sock;
    char recvBuffer[BUFF_SIZE] = { 0 };
    int recvBytes = 0;
};

struct OverlappedEx
{
    enum IO_TYPE { READ, WRITE, ACCEPT, CONNECT };
    WSAOVERLAPPED overlapped = {};
    int type;
};

void HandleError(const char* cause)
{
    std::cout << cause << std::endl;
    std::cout << ::WSAGetLastError() << std::endl;
}

void WokerMain(HANDLE iocpHandle)
{
    while (true)
    {
        DWORD numOfBytes;
        Session* session;
        OverlappedEx* overlappedEx;
        BOOL ret = ::GetQueuedCompletionStatus(iocpHandle, &numOfBytes, (PULONG_PTR)&session,  reinterpret_cast<LPOVERLAPPED*>(&overlappedEx), INFINITE);

        if (ret == FALSE || numOfBytes == 0)
        {
            // 클라이언트 해제
        }

        if (overlappedEx->type == OverlappedEx::IO_TYPE::READ)
        {
            std::cout << "Iocp Recv Size: " << numOfBytes << std::endl;
            std::cout << "Iocp Recv Data: " << session->recvBuffer << std::endl;

            WSABUF buf;
            buf.buf = session->recvBuffer;
            buf.len = BUFF_SIZE;

            DWORD recvSize = 0;
            DWORD flags = 0;    // 0 세팅안하면 오류발생
            WSARecv(session->sock, &buf, 1, &recvSize, &flags, &overlappedEx->overlapped, nullptr);
        }
    }
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
    
    std::vector<Session*> sessionManager;

    // Compleiton Port 생성
    HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

    std::thread t1(WokerMain, iocpHandle);
    std::thread t2(WokerMain, iocpHandle);


    while (true)
    {
        SOCKET clntSock;
        SOCKADDR_IN clntAdr;
        int clntAdrSize = sizeof(clntAdr);

        clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
        if (clntSock == INVALID_SOCKET)
        {
            HandleError("::accept error");
            return 0;
        }
        std::cout << "Client Connected" << std::endl;

        Session* session = new Session(clntSock);
        sessionManager.push_back(session);

        // 소켓 Completion Port에 등록
        ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(session->sock), iocpHandle, reinterpret_cast<ULONG_PTR>(session), 0);
        
        WSABUF buf;
        buf.buf = session->recvBuffer;
        buf.len = BUFF_SIZE;

        OverlappedEx* overlappedEx = new OverlappedEx;
        overlappedEx->type = OverlappedEx::IO_TYPE::READ;

        DWORD recvSize = 0;
        DWORD flags = 0;    // 0 세팅안하면 오류발생
        WSARecv(session->sock, &buf, 1, &recvSize, &flags, &overlappedEx->overlapped, nullptr);
    }

    t1.join();
    t2.join();
    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
