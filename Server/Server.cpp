
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

    // SO_KEEPALIVE : 주기적으로 연결 상태 확인하여 끊어짐 감지(TCP)
    bool enable = true;
    ::setsockopt(servSock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&enable), sizeof(enable));

    // SO_LINER 
    // onoff = 1 linger 옵션 활성화, 0 = default
    // RST 보낼 때 사용
    // 상대방과 4way handshaking 하지 않겠다. ( onoff = 1, linger = 0 )
    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    ::setsockopt(servSock, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&linger), sizeof(linger));

    // SO_SNDBUF = 송신 버퍼 크기
    // SO_RCVBUF = 수신 버퍼 크기
    int sndBufSize;
    int optionLen = sizeof(sndBufSize);
    ::getsockopt(servSock,SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&sndBufSize),&optionLen);
    std::cout << "송신 버퍼 크기: " << sndBufSize << std::endl;

    int rcvdBufSize;
    optionLen = sizeof(rcvdBufSize);
    ::getsockopt(servSock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&rcvdBufSize), &optionLen);
    std::cout << "수신 버퍼 크기: " << rcvdBufSize << std::endl;

    // SO_REUSEADDR
    // IP 주소 및 PORT 재사용
    ::setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&enable), sizeof(enable));
    
    // IPPROTO_TCP
    // TCP_NODELAY = Nagle 알고리즘
    // 장점: 작은 패킷 불필요하게 많이 생성 x
    // 단점: 반응 시간 손해
    {
        bool enable = true;
        ::setsockopt(servSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&enable), sizeof(enable));
    }


    ::closesocket(servSock);
    ::WSACleanup();
    return 0;
}
