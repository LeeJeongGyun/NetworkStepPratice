#pragma once

#include "IOCompletionPort.h"

class Contents : public IOCompletionPort
{
public:
	virtual void OnAccept() override;
	virtual void OnRecv(const unsigned __int64 contentsId, char* pBuffer, int recvBytes) override;
	virtual void OnSend(char* pBuffer, int sendBytes) override;
	virtual void OnDisconnect() override;
};

