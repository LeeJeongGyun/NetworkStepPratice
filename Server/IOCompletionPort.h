#pragma once

#include<WinSock2.h>
#include<MSWSock.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <vector>
#include <thread>
#pragma comment(lib, "ws2_32")

#define MAX_SOCKBUF_SIZE		1024	// ��Ŷ ũ��
#define	MAX_WORKER_THREAD_CNT	4		// ������ Ǯ�� ���� ������ ��

enum class IOOperation
{
	RECV,
	SEND
};

// WSAOVERLAPPED ����ü Ȯ��
struct OverlappedEx
{
	WSAOVERLAPPED	wsaOverlapped;
	SOCKET			clntSock;
	WSABUF			wsaBuf;
	char			buffer[MAX_SOCKBUF_SIZE];
	IOOperation		type;
};

// Ŭ���̾�Ʈ ����
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

	// ���߿� ������ ������ ����.
	void HandleError(const char* cause);

	// ���� �ʱ�ȭ �Լ�
	bool InitSocket();

	// ���� �ּ� ���� socket�� ����
	// listen ���
	bool BindAndListen(int bindPort);

	//���� ��û�� �����ϰ� �޼����� �޾Ƽ� ó���ϴ� �Լ�
	bool StartServer(const unsigned int maxClientCount);

	// �����Ǿ��ִ� ������ �ı�
	void DestroyThread();

private:
	void CreateClient(const unsigned int maxClientCount);

	//WaitingThread Queue���� ����� ��������� ����
	bool CreateWorkerThread();

	// accept ��û ó���ϴ� ������ ����
	bool CreateAccepterThread();

	//������� �ʴ� Ŭ���̾�Ʈ ���� ����ü�� ��ȯ
	ClientInfo* GetEmptyClientInfo();

	// ����� ���� ó�� ������
	void AccepterThread();

	//Overlapped I/O�۾��� ���� �Ϸ� �뺸�� �޾� 
	//�׿� �ش��ϴ� ó���� �ϴ� �Լ�
	void WorkerThread();
	
	//CompletionPort��ü�� ���ϰ� CompletionKey�� �����Ű�� ������ �Ѵ�.
	bool BindIOCompletionPort(ClientInfo* pClientInfo);

	bool BindRecv(ClientInfo* pClientInfo);

	bool SendMsg(ClientInfo* pClientInfo, const char* pMsg, DWORD sendByte);

	// ���� ���� ����
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

