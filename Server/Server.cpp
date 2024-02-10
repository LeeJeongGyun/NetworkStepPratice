
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
    SOCKADDR_IN addr;

    char recvBuffer[BUFF_SIZE] = { 0 };
    char sendBuffer[BUFF_SIZE] = { 0 };

    int recvBytes = 0;
    int sendBytes = 0;
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
    
    // select 모델
    // 리눅스, 윈도우 호환
    // 64개 제한.
    // 소켓 함수 호출 성공 시점 미리 알 수 있음.
    // 문제 상황)
    // 수신 버퍼 데이터 없는데 read
    // 송신 버퍼가 꽉 찼는데, write
    // - 블로킷 소켓: 조건이 만족되지 않아 블로킹 되는 상황 예방
    // - 논블로킹 소켓: 조건이 만족되지 않아 불필요하게 반복 체크 예방ㄴ

    // FD_ZERO : 초기화
    // FD_SET  : 소켓 설정
    // FD_CLR  : 소켓 삭제
    // FD_ISSET : 소켓 확인

    std::vector<Session> sessions;
    sessions.reserve(100);

    fd_set readSet, writeSet;
    while (true)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        FD_SET(listenSock, &readSet);
        for (Session& s : sessions)
        {
            if (s.recvBytes <= s.sendBytes) FD_SET(s.sock, &readSet);
            else FD_SET(s.sock, &writeSet); 
        }
        
        // 적어도 하나의 소켓이 준비되면 리턴 -> 준비되지 않은 소켓들 삭제.
        int retVal = ::select(0, &readSet, &writeSet, nullptr, nullptr);
        if (retVal == SOCKET_ERROR)
        {
            HandleError("::select error");
            break;
        }

        if (FD_ISSET(listenSock, &readSet))
        {
            SOCKADDR_IN clntAdr;
            int clntAdrSize = sizeof(clntAdr);
            SOCKET clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
            if (clntSock == INVALID_SOCKET)
            {
                HandleError("::accept error");
                return 0;
            }
            else
            {
                sessions.push_back(Session{clntSock});
            }

            std::cout << "Accept Success!!" << std::endl;
        }

        for (Session& s : sessions)
        {
            if (FD_ISSET(s.sock, &readSet))
            {
                int recvSize = ::recv(s.sock, s.recvBuffer, BUFF_SIZE, 0);
                if (recvSize <= 0)
                {
                    // 제거 필요
                    continue;
                }

                s.recvBytes = recvSize;
                std::cout << "Recv Size: " << recvSize << std::endl;
                std::cout << "Recv Data: " << s.recvBuffer << std::endl;
            }
            
            if (FD_ISSET(s.sock, &writeSet))
            {
                // 블로킹 모드 -> 모든 데이터 보낼 때 까지 대기.
                // 논블로킹 모드 -> 일부만 보낼 경우 존재.
                int sendSize = ::send(s.sock, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
                if (sendSize <= 0)
                {
                    // 제거 필요
                    continue;
                }

                s.sendBytes += sendSize;
                std::cout << "Send Size: " << sendSize << std::endl;

                if (s.sendBytes == s.recvBytes)
                {
                    s.recvBytes = 0;
                    s.sendBytes = 0;
                }
            }
        }
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
