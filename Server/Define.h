#pragma once

#include<WinSock2.h>
#include<Windows.h>

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
	IOOperation		type;
};

// Ŭ���̾�Ʈ ����
struct ClientInfo
{
	SOCKET				clntSock = INVALID_SOCKET;
	unsigned __int64	sessionId;
	char				recvBuffer[MAX_SOCKBUF_SIZE];
	char				sendBuffer[MAX_SOCKBUF_SIZE];

	OverlappedEx		RecvOverlappedEx = {};
	OverlappedEx		SendOverlappedEx = {};
};