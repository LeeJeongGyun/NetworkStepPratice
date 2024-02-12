#pragma once

#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <vector>
#include <thread>
#pragma comment(lib, "ws2_32")

#define MAX_SOCKBUF_SIZE		1024	// 패킷 크리
#define	MAX_WORKER_THREAD_CNT	4		// 쓰레드 풀에 넣을 쓰레드 수

enum class IOOperation
{
	RECV,
	SEND
};

// WSAOVERLAPPED 구조체 확장
struct OverlappedEx
{
	WSAOVERLAPPED	wsaOverlapped;
	SOCKET			clntSock;
	WSABUF			wsaBuf;
	char			buffer[MAX_SOCKBUF_SIZE];
	IOOperation		type;
};

// 클라이언트 정보
struct ClientInfo
{
	SOCKET			clntSock = INVALID_SOCKET;
	OverlappedEx	RecvOverlappedEx = {};
	OverlappedEx	SendOverlappedEx = {};
};

class IOCompletionPort
{
public:
	IOCompletionPort();
	~IOCompletionPort();

	// 나중에 적당한 곳으로 빼자.
	void HandleError(const char* cause);

	// 소켓 초기화 함수
	bool InitSocket();

	// 서버 주소 정보 socket과 연동
	// listen 등록
	bool BindAndListen(int bindPort);

	//접속 요청을 수락하고 메세지를 받아서 처리하는 함수
	bool StartServer(const unsigned int maxClientCount);

	// 생성되어있는 쓰레드 파괴
	void DestroyThread();

private:
	void CreateClient(const unsigned int maxClientCount);

	//WaitingThread Queue에서 대기할 쓰레드들을 생성
	bool CreateWorkerThread();

	// accept 요청 처리하는 쓰레드 생성
	bool CreateAccepterThread();

	//사용하지 않는 클라이언트 정보 구조체를 반환
	ClientInfo* GetEmptyClientInfo();

	// 사용자 접속 처리 쓰레드
	void AccepterThread();

	//Overlapped I/O작업에 대한 완료 통보를 받아 
	//그에 해당하는 처리를 하는 함수
	void WorkerThread();
	
	//CompletionPort객체와 소켓과 CompletionKey를 연결시키는 역할을 한다.
	bool BindIOCompletionPort(ClientInfo* pClientInfo);

	bool BindRecv(ClientInfo* pClientInfo);

	bool SendMsg(ClientInfo* pClientInfo, const char* pMsg, DWORD sendByte);

	// 소켓 연결 종료
	void CloseSocket(ClientInfo* pClientInfo, bool isForce = false);

private:
	SOCKET	_listenSock;
	HANDLE	_iocpHandle;
	bool	_isWorkerThreadRun = true;
	bool	_isAccepterRun = true;
	int		_clientCount = 0;

	std::thread					_accepterThread;
	std::vector<std::thread>	_vWorkerThread;
	std::vector<ClientInfo>		_vClientInfo;
};

