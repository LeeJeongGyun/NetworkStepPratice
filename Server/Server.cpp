
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
    Session(SOCKET s) { sock = s; }
    WSAOVERLAPPED overlapped = { 0 };
    SOCKET sock;
    char recvBuffer[BUFF_SIZE] = { 0 };
    int recvBytes = 0;
};

void HandleError(const char* cause)
{
    std::cout << cause << std::endl;
    std::cout << ::WSAGetLastError() << std::endl;
}

void CompletionRoutine(DWORD errCode, DWORD numOfBytes, LPOVERLAPPED overlapped, DWORD flags)
{
    // 위와 같은 방법으로 세션 정보 가져올 수 있다.
    Session* session = reinterpret_cast<Session*>(overlapped);

    std::cout << "Socket: " << session->sock << std::endl;
    std::cout << "Completion Routine Success!!" << std::endl;
    std::cout << "Recv Data: " << numOfBytes << std::endl;

    // Echo Server 라면 여기서 WSASend 호출 필요.
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
    
    // Overlapped IO (비동기 + 논블로킹) (콜백 기반)
    // 비동기 함수(WSASend, WSARecv)를 호출하여 Callback 함수를 전달하면
    // 비동기 함수가 완료되었을 때 요청한 스레드의 APC Queue에 인자 전달
    // Alertable Wait 상태로 변경시키면 APC Queue에 존재하는 일감 처리
    // 한번 호출하면 모든 일감 처리하는 구조
    
    // 특징
    // 모든 비동기 소켓 함수에서 사용 가능하지 않음 ( AcceptEx, ConnectEx .. )
    // 빈번한 Alertable Wait 상태 변경으로 인한 성능 저하
    // ApcQueue가 스레드 별로 존재하기 때문에 일감 분배가 효율적이지 않음. 

    // Reactor Pattern ( ~뒤늦게. 논블로킹 소켓 -> 뒤늣게 recv, send 호출)
    // Proactor Pattern ( ~미리. WSASend, WSARecv)

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

        Session session = Session(clntSock);

        std::cout << "Client Connected" << std::endl;
        while (true)
        {
            WSABUF buf;
            buf.buf = session.recvBuffer;
            buf.len = BUFF_SIZE;

            DWORD recvSize = 0;
            DWORD flags = 0;    // 0 세팅안하면 오류발생
            // WSABUF 변수는 지워져도 상관x
            // But, recvBuffer는 지워지면 안된다.
            if (SOCKET_ERROR == ::WSARecv(session.sock, &buf, 1, &recvSize, &flags, &session.overlapped, CompletionRoutine))
            {
                if (WSAGetLastError() == WSA_IO_PENDING)
                {
                    // 원한는 시점에 Alertable Wait 상태로 만들어서 CALLBACK 함수 호출
                    SleepEx(WSA_INFINITE, TRUE);
                }
                else
                {
                    std::cout << "Recv Data Size: " << recvSize << std::endl;
                    std::cout << "Recv Data: " << session.recvBuffer << std::endl;
                }
            }
        }
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
