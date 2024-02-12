#include "IOCompletionPort.h"
#include<iostream>

IOCompletionPort::IOCompletionPort()
{
}

IOCompletionPort::~IOCompletionPort()
{
    ::WSACleanup();
}

void IOCompletionPort::HandleError(const char* cause)
{
    std::cout << cause << std::endl;
    std::cout << "Error Code: " << ::WSAGetLastError() << std::endl;
}

bool IOCompletionPort::InitSocket()
{
    WSADATA wsaData;
    int ret = ::WSAStartup(MAKEWORD(2,2), &wsaData);
    if (ret != 0)
    {
        HandleError("::WSAStartup Error");
        return false;
    }

    _listenSock = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (_listenSock == INVALID_SOCKET)
    {
        HandleError("::WSASocket Error");
        return false;
    }

    // TODO: 옵션 세팅 필요!!
    // LINGER, REUSEADDR 등등

    std::cout << "Socket Init Success!!" << std::endl;
    return true;
}

bool IOCompletionPort::BindAndListen(int bindPort)
{
    SOCKADDR_IN servAdr = {};
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    servAdr.sin_port = ::htons(bindPort);
    if (SOCKET_ERROR == ::bind(_listenSock, reinterpret_cast<const SOCKADDR*>(&servAdr), sizeof(servAdr)))
    {
        HandleError("::bind Error");
        return false;
    }

    if (SOCKET_ERROR == ::listen(_listenSock, 100))
    {
        HandleError("::listen Error");
        return false;
    }

    std::cout << "Bind And Listen Success" << std::endl;
    return true;
}

bool IOCompletionPort::StartServer(const unsigned int maxClientCount)
{
    CreateClient(maxClientCount);

    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKER_THREAD_CNT);
    if (_iocpHandle == NULL)
    {
        HandleError("::CreateIoCompletionPort Error");
        return false;
    }
    
    if (false == CreateWorkerThread())
    {
        HandleError("CreateWorkerThread Fail");
        return false;
    }

    if (false == CreateAccepterThread())
    {
        HandleError("CreateAccepterThread Fail");
        return false;
    }

    std::cout << "Service Start!!" << std::endl;
    return true;
}

void IOCompletionPort::CreateClient(const unsigned int maxClientCount)
{
    for (unsigned int i = 0; i < maxClientCount; ++i)
    {
        _vClientInfo.emplace_back();
    }
}

bool IOCompletionPort::CreateWorkerThread()
{
    for (int i = 0; i < MAX_WORKER_THREAD_CNT; ++i)
    {
        _vWorkerThread.emplace_back(std::thread([this]() {WorkerThread(); }));
    }

    std::cout << "Worker Thread Run!!" << std::endl;
    return true;
}

bool IOCompletionPort::CreateAccepterThread()
{
    _accepterThread = std::thread([this]() { AccepterThread(); });

    std::cout << "Accepter Thread Run!!" << std::endl;
    return true;
}

ClientInfo* IOCompletionPort::GetEmptyClientInfo()
{
    for (auto& clientInfo : _vClientInfo)
    {
        if (clientInfo.clntSock == INVALID_SOCKET)
        {
            return &clientInfo;
        }
    }

    return nullptr;
}

void IOCompletionPort::AccepterThread()
{
    SOCKADDR_IN clntAdr;
    int clntAdrLen = sizeof(clntAdr);

    while (_isAccepterRun)
    {
        ClientInfo* pClientInfo = GetEmptyClientInfo();
        if (pClientInfo == nullptr)
        {
            std::cout << "Client Full!!" << std::endl;
            return;
        }

        pClientInfo->clntSock = ::accept(_listenSock, reinterpret_cast<SOCKADDR*>(&clntAdr), &clntAdrLen);
        if (pClientInfo->clntSock == INVALID_SOCKET)
        {
            HandleError("::accept Error");
            continue;
        }

        if (false == BindIOCompletionPort(pClientInfo))
        {
            HandleError("BindIOCompletionPort Error");
            return;
        }

        if (false == BindRecv(pClientInfo))
        {
            // TODO 수정 필요.
            return;
        }

        char clientIp[16] = {};
        ::InetNtopA(AF_INET, &clntAdr.sin_addr, reinterpret_cast<PSTR>(clientIp), 16);
        std::cout << clientIp << std::endl;

        _clientCount++;
    }
}

void IOCompletionPort::WorkerThread()
{
    DWORD bytes;
    ClientInfo* pClientInfo;
    LPOVERLAPPED lpOverlapped = NULL;
    BOOL bSuccess;

    while (_isWorkerThreadRun)
    {
        bSuccess = ::GetQueuedCompletionStatus(_iocpHandle, &bytes, reinterpret_cast<PULONG_PTR>(&pClientInfo), &lpOverlapped, INFINITE);
        if (bSuccess == TRUE && bytes == 0 && lpOverlapped == NULL)
        {
            _isWorkerThreadRun = false;
            continue;
        }

        if (lpOverlapped == NULL)
        {
            continue;
        }

        // Client 접속 종료
        if (bytes == 0)
        {
            std::cout << "Client Disconnect, Socket: " << pClientInfo->clntSock << std::endl;
            CloseSocket(pClientInfo);
            continue;
        }

        OverlappedEx* pOverlappedEx = reinterpret_cast<OverlappedEx*>(lpOverlapped);
        if (pOverlappedEx->type == IOOperation::RECV)
        {
            pOverlappedEx->buffer[bytes] = NULL;
            std::cout << "Recv Bytes: " << bytes << ", Msg: " << pOverlappedEx->buffer << std::endl;

            // 클라이언트 Echo
            SendMsg(pClientInfo, pOverlappedEx->buffer, bytes);
            BindRecv(pClientInfo);
        }
        else if (pOverlappedEx->type == IOOperation::SEND)
        {
            pOverlappedEx->buffer[bytes] = NULL;
            std::cout << "Send Bytes: " << bytes << ", Msg: " << pOverlappedEx->buffer << std::endl;
        }
        else
        {
            // TODO : 예외처리
            std::cout << "예외 처리 필요!!" << std::endl;
        }
    }
}

bool IOCompletionPort::BindIOCompletionPort(ClientInfo* pClientInfo)
{
    if (NULL == ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(pClientInfo->clntSock), reinterpret_cast<HANDLE>(_iocpHandle), reinterpret_cast<ULONG_PTR>(pClientInfo), 0))
    {
        HandleError("BindIOCompltionPort -> ::CreateIOCompletionPort Error");
        return false;
    }

    return true;
}

bool IOCompletionPort::BindRecv(ClientInfo* pClientInfo)
{
    DWORD bytes = 0;
    DWORD flag = 0;
    WSABUF& wsaBuf = pClientInfo->RecvOverlappedEx.wsaBuf;
    wsaBuf.buf = pClientInfo->RecvOverlappedEx.buffer;
    wsaBuf.len = MAX_SOCKBUF_SIZE;
    pClientInfo->RecvOverlappedEx.type = IOOperation::RECV;

    int ret = ::WSARecv(pClientInfo->clntSock, &wsaBuf, 1, &bytes, &flag, reinterpret_cast<LPWSAOVERLAPPED>(&pClientInfo->RecvOverlappedEx), NULL);
    if (ret == SOCKET_ERROR && (::WSAGetLastError() != WSA_IO_PENDING))
    {
        HandleError("BindRecv - ::WSARecv Error");
        return false;
    }
    return true;
}

bool IOCompletionPort::SendMsg(ClientInfo* pClientInfo, const char * pMsg, DWORD sendByte)
{
    DWORD bytes = 0;
    CopyMemory(pClientInfo->SendOverlappedEx.buffer, pClientInfo->RecvOverlappedEx.buffer, sendByte);

    WSABUF& wsaBuf = pClientInfo->SendOverlappedEx.wsaBuf;
    wsaBuf.buf = pClientInfo->SendOverlappedEx.buffer;
    wsaBuf.len = sendByte;
    pClientInfo->SendOverlappedEx.type = IOOperation::SEND;


    // Error면 끊어진 것으로 처리
    int ret = ::WSASend(pClientInfo->clntSock, &wsaBuf, 1, &bytes, 0, reinterpret_cast<LPWSAOVERLAPPED>(&pClientInfo->SendOverlappedEx), NULL);
    if (ret == SOCKET_ERROR && (::WSAGetLastError() != WSA_IO_PENDING))
    {
        HandleError("SendMsg - ::WSASend Error");
        return false;
    }
    
    return true;
}

void IOCompletionPort::DestroyThread()
{
    _isWorkerThreadRun = false;
    CloseHandle(_iocpHandle);

    for (auto& t : _vWorkerThread)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    _isAccepterRun = false;
    ::closesocket(_listenSock);

    if (_accepterThread.joinable())
    {
        _accepterThread.join();
    }

}

void IOCompletionPort::CloseSocket(ClientInfo* pClientInfo, bool isForce)
{
    ::closesocket(pClientInfo->clntSock);
    pClientInfo->clntSock = INVALID_SOCKET;
}
