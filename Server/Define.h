#pragma once

#include<WinSock2.h>
#include<Windows.h>

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
	IOOperation		type;
};

// 클라이언트 정보
struct ClientInfo
{
	SOCKET				clntSock = INVALID_SOCKET;
	unsigned __int64	sessionId;
	char				recvBuffer[MAX_SOCKBUF_SIZE];
	char				sendBuffer[MAX_SOCKBUF_SIZE];

	OverlappedEx		RecvOverlappedEx = {};
	OverlappedEx		SendOverlappedEx = {};
};