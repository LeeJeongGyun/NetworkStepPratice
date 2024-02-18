#include "Contents.h"
#include<iostream>

void Contents::OnAccept()
{
	std::cout << "Contents OnAccept" << std::endl;
}

void Contents::OnRecv(const unsigned __int64 contentsId, char* pBuffer, int recvBytes)
{
	// Echo Server
	std::cout << "Contents OnRecv" << std::endl;

	/*
		¿©±â¼­ ÄÁÅÙÃ÷ ÀÛ¾÷ ½ÇÇà.
	*/

	SendData(contentsId, pBuffer, recvBytes);
}

void Contents::OnSend(char* pBuffer, int sendBytes)
{
	std::cout << "Contents OnSend" << std::endl;
}

void Contents::OnDisconnect()
{
	std::cout << "Contents OnDisconnect" << std::endl;
}
