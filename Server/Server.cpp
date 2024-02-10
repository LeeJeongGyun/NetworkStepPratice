
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
    
    // WSAEventSelect
    // 64개 제한

    // 생성 : WSACreateEvent( 수동 리셋(Manual Reset), Non-Signaled )
    // 삭제 : WSACloseEvent
    // 신호 상태 감지 : WSAWaitForMutipleEvents
    // 구체적인 네트워크 이벤트 확인 : WSAEnumNetworkEvents

    // 소켓 <-> 이벤트 객체 연동 필요
    // FD_ACCEPT, FD_READ, FD_WRITE, FD_CLOSE, FD_CONNECT

    // 주의
    // WSAEventSelect 함수 호출 시 해당 소켓 자동으로 넌블로킹 전환
    // accept() 함수 리턴하는 소켓은 listenSock과 동일한 속성
    // 따라서, 리턴 소켓 FD_READ, RD_WRITE 등록  필요
    // WSAEWOULDBLOCK 예외 처리 필요
    // 이벤트 발생 시 적절한 함수 호출 필요 -> 그렇지 않다면 네트워크 이벤트 발생하지 않음
    // ex) FD_READ 발생히 recv 함수 무조건 호출 필요

    // WSAEnumNetworkEvents
    // 인자로 들어온 event 객체를 non-signaled 상태로 변경

    std::vector<WSAEVENT> events;
    std::vector<Session> sessions;

    WSAEVENT listenEvent = ::WSACreateEvent();
    events.push_back(listenEvent);
    sessions.push_back(Session{ listenSock });
    if (SOCKET_ERROR == ::WSAEventSelect(listenSock, listenEvent, FD_ACCEPT | FD_CLOSE))
    {
        HandleError("::WSAEventSelect error");
        return 0;
    }

    while (true)
    {
        int retIdx = ::WSAWaitForMultipleEvents(static_cast<DWORD>(events.size()), &events[0], FALSE, INFINITE, FALSE);
        if (retIdx == WSA_WAIT_FAILED) continue;

        retIdx -= WSA_WAIT_EVENT_0;

        WSANETWORKEVENTS netEvents;
        // 자동으로 non-signaled로 변경해줌
        if (SOCKET_ERROR == ::WSAEnumNetworkEvents(sessions[retIdx].sock, events[retIdx], &netEvents)) continue;
        
        if (netEvents.lNetworkEvents & FD_ACCEPT)
        {
            if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) continue;

            SOCKADDR_IN clntAdr;
            int clntAdrSize = sizeof(clntAdr);

            SOCKET clntSock = ::accept(listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrSize);
            if (clntSock != INVALID_SOCKET)
            {
                std::cout << "Client Success" << std::endl;

                WSAEVENT clntEvent = ::WSACreateEvent();
                events.push_back(clntEvent);
                sessions.push_back(Session{ clntSock });

                if (SOCKET_ERROR == ::WSAEventSelect(clntSock, clntEvent, FD_READ | FD_WRITE | FD_CLOSE)) continue;
            }
        }

        if (netEvents.lNetworkEvents & FD_READ || netEvents.lNetworkEvents & FD_WRITE)
        {
            if ((netEvents.lNetworkEvents & FD_READ) && (netEvents.iErrorCode[FD_READ_BIT] != 0)) continue;

            if ((netEvents.lNetworkEvents & FD_WRITE) && (netEvents.iErrorCode[FD_WRITE_BIT] != 0)) continue;

            Session s = sessions[retIdx];
            if (s.recvBytes <= s.sendBytes)
            {
                int recvSize = ::recv(s.sock, s.recvBuffer, BUFF_SIZE, 0);
                if (recvSize == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    // 에러 처리 필요
                    continue;
                }

                s.recvBytes     = recvSize;
                std::cout << "Recv Size: " << recvSize << std::endl;
                std::cout << "Recv Data: " << s.recvBuffer << std::endl;
            }
            
            if(s.recvBytes > s.sendBytes)
            {
                int sendSize = ::send(s.sock, &s.recvBuffer[s.recvBytes - s.sendBytes], s.recvBytes - s.sendBytes, 0);
                if (sendSize == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    // 에러 처리 필요
                    continue;
                }
                
                std::cout << "Send Success!!" << std::endl;
                s.sendBytes += sendSize;
                if (s.recvBytes == s.sendBytes)
                {
                    s.recvBytes = 0;
                    s.sendBytes = 0;
                }
            }
        }
        
        if (netEvents.lNetworkEvents & FD_CLOSE)
        {
            // 삭제 필요
        }
    }

    ::closesocket(listenSock);
    ::WSACleanup();
    return 0;
}
