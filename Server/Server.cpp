
#include "Contents.h"

#define MAX_CLIENT	1000
#define	SERVER_PORT	9999

int main()
{
	Contents contents;

	// 소켓 초기화
	contents.InitSocket();
	
	// 소켓 주소 연동
	contents.BindAndListen(SERVER_PORT);
	
	// 서버 시작
	contents.StartServer(MAX_CLIENT);

	std::cout << "종료를 원하면 quit를 입력하시오." << std::endl;
	while (true)
	{
		std::string inputCmd;
		std::cin >> inputCmd;
		if (inputCmd == "quit") break;
	}

	contents.DestroyThread();

	return 0;
}
