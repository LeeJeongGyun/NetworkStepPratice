
#include "IOCompletionPort.h"

#define MAX_CLIENT	1000
#define	SERVER_PORT	9999

int main()
{
	IOCompletionPort ioCompletionPort;

	//소켓을 초기화
	ioCompletionPort.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	ioCompletionPort.BindAndListen(SERVER_PORT);

	ioCompletionPort.StartServer(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");
	getchar();

	ioCompletionPort.DestroyThread();
	return 0;

	return 0;
}
